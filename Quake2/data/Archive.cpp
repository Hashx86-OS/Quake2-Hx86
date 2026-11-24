/*
 * MIT License
 *
 * Copyright (c) 2025 Malaka Gunawardana
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "Archive.h"

#include "HttpClient.h"
#include "StoreUtil.h"

#include <Hx86/Hsyscalls/syscalls.h>
#include <Hx86/debug.h>
#include <mbedtls/sha256.h>

#define MINIZ_NO_STDIO
#define MINIZ_NO_DEFLATE_APIS
#define MINIZ_NO_TIME
#define NDEBUG
#include <miniz.h>

namespace {

constexpr const char* kDemoUrl =
    "https://www.gamers.org/pub/idgames/idstuff/quake2/q2-314-demo-x86.exe";
constexpr uint32_t kDemoSize = 39015499;
constexpr const char* kDemoHash =
    "7ace5a43983f10d6bdc9d9b6e17a1032ba6223118d389bd170df89b945a04a1e";

constexpr const char* kPakMember = "Install/Data/baseq2/pak0.pak";
constexpr uint32_t kPakSize = 49951322;
constexpr const char* kPakHash =
    "cae257182f34d3913f3d663e1d7cf865d668feda6af393d4ecf3e9e408b48d09";

constexpr const char* kLicenseMember = "Install/Data/DOCS/license.txt";
constexpr uint32_t kLicenseSize = 8963;
constexpr const char* kLicenseHash =
    "5d23d6248e292d810d4925a6ec32ca5b179a20d4273a73638b11fbd12e4c11d0";

constexpr const char* kPlayersPrefix = "Install/Data/baseq2/players/";
constexpr uint32_t kPathCapacity = 320;

struct ArchivePaths {
    char baseDir[kPathCapacity];
    char sharewareDir[kPathCapacity];
    char licensesDir[kPathCapacity];
    char demoPath[kPathCapacity];
    char demoPartPath[kPathCapacity];
    char pakPath[kPathCapacity];
    char pakPartPath[kPathCapacity];
    char licensePath[kPathCapacity];
    char licensePartPath[kPathCapacity];
};

void JoinPath(char* output, uint32_t capacity, const char* base, const char* leaf) {
    uint32_t used = 0;
    if (base != nullptr) {
        while (*base != '\0' && used + 1 < capacity) output[used++] = *base++;
    }
    while (used > 0 && output[used - 1] == '/') used--;
    if (used + 1 < capacity) output[used++] = '/';
    if (leaf != nullptr) {
        while (*leaf != '\0' && used + 1 < capacity) output[used++] = *leaf++;
    }
    output[used] = '\0';
}

void BuildPaths(const char* baseDir, ArchivePaths* paths) {
    StoreStrCopy(paths->baseDir, sizeof(paths->baseDir), baseDir);
    JoinPath(paths->sharewareDir, sizeof(paths->sharewareDir), baseDir, "shareware");
    JoinPath(paths->licensesDir, sizeof(paths->licensesDir), baseDir, "licenses");
    JoinPath(paths->demoPath, sizeof(paths->demoPath), paths->sharewareDir,
             "q2-314-demo-x86.exe");
    JoinPath(paths->demoPartPath, sizeof(paths->demoPartPath), paths->sharewareDir,
             "q2-314-demo-x86.exe.part");
    JoinPath(paths->pakPath, sizeof(paths->pakPath), baseDir, "pak0.pak");
    JoinPath(paths->pakPartPath, sizeof(paths->pakPartPath), baseDir, "pak0.pak.part");
    JoinPath(paths->licensePath, sizeof(paths->licensePath), paths->licensesDir,
             "QUAKE2LIC.TXT");
    JoinPath(paths->licensePartPath, sizeof(paths->licensePartPath), paths->licensesDir,
             "QUAKE2LIC.TXT.part");
}

struct ShaSink {
    int32_t fd;
    mbedtls_sha256_context sha;
    uint32_t received;
    uint32_t expected;
    uint32_t nextProgress;
    ArchiveProgress progress;
    const char* label;
};

void SetError(char* error, uint32_t capacity, const char* text) {
    StoreStrCopy(error, capacity, text);
}

void Report(ArchiveProgress progress, const char* text) {
    if (progress != nullptr) progress(text);
}

void DigestToHex(const uint8_t digest[32], char output[65]) {
    static const char kHex[] = "0123456789abcdef";
    for (uint32_t i = 0; i < 32; i++) {
        output[i * 2] = kHex[digest[i] >> 4];
        output[i * 2 + 1] = kHex[digest[i] & 0x0F];
    }
    output[64] = '\0';
}

bool FinishSha(ShaSink* sink, char output[65]) {
    uint8_t digest[32] = {};
    int result = mbedtls_sha256_finish_ret(&sink->sha, digest);
    mbedtls_sha256_free(&sink->sha);
    if (result != 0) return false;
    DigestToHex(digest, output);
    return true;
}

void UpdateProgress(ShaSink* sink, const char* label) {
    if (sink->progress == nullptr) return;
    if (sink->expected == 0 || sink->received < sink->nextProgress) return;
    uint32_t percent = (sink->received * 100u) / sink->expected;
    char text[160];
    StoreFormat(text, sizeof(text), "%s: %u / %u KB (%u%%)", label, sink->received / 1024u,
                sink->expected / 1024u, percent);
    sink->progress(text);
    sink->nextProgress += 64u * 1024u;
}

bool DownloadSink(void* context, const uint8_t* data, uint32_t length) {
    ShaSink* sink = static_cast<ShaSink*>(context);
    if (!StoreWriteAll(sink->fd, data, length)) return false;
    if (mbedtls_sha256_update_ret(&sink->sha, data, length) != 0) return false;
    sink->received += length;
    UpdateProgress(sink, sink->label);
    return true;
}

bool ReplaceFile(const char* partPath, const char* finalPath) {
    syscall_unlink(finalPath);
    return syscall_rename(partPath, finalPath) == 0;
}

bool DownloadFile(const char* url, const char* partPath, const char* finalPath,
                  uint32_t expectedSize, const char* expectedHash, const char* label,
                  ArchiveProgress progress, char* error, uint32_t errorCapacity) {
    int32_t fd = syscall_open(partPath, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        StoreFormat(error, errorCapacity, "could not create %s", partPath);
        return false;
    }

    ShaSink sink = {};
    sink.fd = fd;
    sink.expected = expectedSize;
    sink.nextProgress = 64u * 1024u;
    sink.progress = progress;
    sink.label = label;
    mbedtls_sha256_init(&sink.sha);
    if (mbedtls_sha256_starts_ret(&sink.sha, 0) != 0) {
        syscall_close((uint32_t)fd);
        syscall_unlink(partPath);
        mbedtls_sha256_free(&sink.sha);
        SetError(error, errorCapacity, "SHA-256 initialization failed");
        return false;
    }

    StoreHttpResponse response = {};
    bool ok = StoreHttpGet(url, "*/*", DownloadSink, &sink, &response, error, errorCapacity);
    char digest[65];
    bool hashOk = FinishSha(&sink, digest);
    syscall_close((uint32_t)fd);

    if (!ok) {
        syscall_unlink(partPath);
        return false;
    }
    if (!hashOk || sink.received != expectedSize || !StoreHexEqual(digest, expectedHash)) {
        syscall_unlink(partPath);
        SetError(error, errorCapacity, "downloaded file failed its size or SHA-256 check");
        return false;
    }
    if (!ReplaceFile(partPath, finalPath)) {
        syscall_unlink(partPath);
        StoreFormat(error, errorCapacity, "could not move %s into place", finalPath);
        return false;
    }
    return true;
}

struct ZipSource {
    int32_t fd;
    uint32_t size;
};

size_t ZipRead(void* opaque, mz_uint64 offset, void* buffer, size_t length) {
    ZipSource* source = static_cast<ZipSource*>(opaque);
    if (offset >= source->size) return 0;
    uint32_t available = source->size - (uint32_t)offset;
    if (length > available) length = available;
    if (syscall_lseek((uint32_t)source->fd, (int32_t)offset, SEEK_SET) < 0) return 0;
    int32_t result = syscall_read((uint32_t)source->fd, (char*)buffer, (uint32_t)length);
    return result > 0 ? (size_t)result : 0;
}

size_t ZipWrite(void* opaque, mz_uint64 offset, const void* data, size_t length) {
    (void)offset;
    ShaSink* sink = static_cast<ShaSink*>(opaque);
    if (!StoreWriteAll(sink->fd, (const uint8_t*)data, (uint32_t)length)) return 0;
    if (mbedtls_sha256_update_ret(&sink->sha, (const uint8_t*)data, length) != 0) return 0;
    sink->received += (uint32_t)length;
    UpdateProgress(sink, sink->label);
    return length;
}

bool ExtractZipMember(mz_zip_archive* zip, const char* member, const char* partPath,
                      const char* finalPath, uint32_t expectedSize, const char* expectedHash,
                      const char* label, ArchiveProgress progress, char* error,
                      uint32_t errorCapacity) {
    int index = mz_zip_reader_locate_file(zip, member, nullptr, 0);
    mz_zip_archive_file_stat stat = {};
    if (index < 0 || !mz_zip_reader_file_stat(zip, (mz_uint)index, &stat) ||
        stat.m_uncomp_size != expectedSize) {
        StoreFormat(error, errorCapacity, "%s is missing from the demo archive", member);
        return false;
    }

    int32_t fd = syscall_open(partPath, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        StoreFormat(error, errorCapacity, "could not create %s", partPath);
        return false;
    }

    ShaSink sink = {};
    sink.fd = fd;
    sink.expected = expectedSize;
    sink.nextProgress = 64u * 1024u;
    sink.progress = progress;
    sink.label = label;
    mbedtls_sha256_init(&sink.sha);
    bool started = mbedtls_sha256_starts_ret(&sink.sha, 0) == 0;
    bool extracted =
        started && mz_zip_reader_extract_to_callback(zip, (mz_uint)index, ZipWrite, &sink, 0);
    char digest[65];
    bool hashOk = FinishSha(&sink, digest);
    syscall_close((uint32_t)fd);

    if (!started || !extracted || !hashOk || sink.received != expectedSize ||
        !StoreHexEqual(digest, expectedHash)) {
        syscall_unlink(partPath);
        StoreFormat(error, errorCapacity, "%s failed its size or SHA-256 check", member);
        return false;
    }
    if (!ReplaceFile(partPath, finalPath)) {
        syscall_unlink(partPath);
        StoreFormat(error, errorCapacity, "could not move %s into place", finalPath);
        return false;
    }
    return true;
}

bool EnsureDirTree(const char* path) {
    char buffer[kPathCapacity];
    if (!StoreStrCopy(buffer, sizeof(buffer), path)) return false;
    for (uint32_t i = 1; buffer[i] != '\0'; i++) {
        if (buffer[i] != '/') continue;
        buffer[i] = '\0';
        int32_t result = syscall_mkdir(buffer, 0777);
        buffer[i] = '/';
        if (result < 0) {
            struct stat st;
            if (syscall_stat(buffer, &st) != 0) return false;
        }
    }
    if (syscall_mkdir(buffer, 0777) < 0) {
        struct stat st;
        if (syscall_stat(buffer, &st) != 0) return false;
    }
    return true;
}

const char* SkipPrefix(const char* name, const char* prefix) {
    if (name == nullptr || prefix == nullptr) return nullptr;
    while (*prefix != '\0') {
        if (*name != *prefix) return nullptr;
        name++;
        prefix++;
    }
    return name;
}

bool ExtractPlayers(mz_zip_archive* zip, const ArchivePaths* paths, ArchiveProgress progress,
                    char* error, uint32_t errorCapacity) {
    Report(progress, "Extracting player models...");
    mz_uint count = mz_zip_reader_get_num_files(zip);
    uint32_t extracted = 0;
    for (mz_uint i = 0; i < count; i++) {
        mz_zip_archive_file_stat stat = {};
        if (!mz_zip_reader_file_stat(zip, i, &stat) || stat.m_filename == nullptr) continue;

        char normalized[kPathCapacity];
        if (!StoreStrCopy(normalized, sizeof(normalized), stat.m_filename)) continue;
        for (uint32_t c = 0; normalized[c] != '\0'; c++) {
            if (normalized[c] == '\\') normalized[c] = '/';
        }
        const char* relative = SkipPrefix(normalized, kPlayersPrefix);
        if (relative == nullptr || relative[0] == '\0') continue;
        if (StoreStrStartsWith(relative, "../") || StoreStrEqual(relative, "..")) {
            SetError(error, errorCapacity, "demo archive contains an unsafe player path");
            return false;
        }

        char playersDir[kPathCapacity];
        JoinPath(playersDir, sizeof(playersDir), paths->baseDir, "players");
        char playerPath[kPathCapacity];
        JoinPath(playerPath, sizeof(playerPath), playersDir, relative);
        char directory[kPathCapacity];
        StoreStrCopy(directory, sizeof(directory), playerPath);
        char* slash = nullptr;
        for (char* p = directory; *p != '\0'; p++) {
            if (*p == '/') slash = p;
        }
        if (slash == nullptr) continue;
        *slash = '\0';
        if (!EnsureDirTree(directory)) {
            StoreFormat(error, errorCapacity, "could not create %s", directory);
            return false;
        }
        *slash = '/';

        char partPath[kPathCapacity + 5];
        if (!StoreStrCopy(partPath, sizeof(partPath), playerPath) ||
            !StoreStrCopy(partPath + StoreStrLen(partPath), sizeof(partPath) - StoreStrLen(partPath),
                          ".part")) {
            SetError(error, errorCapacity, "player path is too long");
            return false;
        }
        int32_t fd = syscall_open(partPath, O_WRONLY | O_CREAT | O_TRUNC);
        if (fd < 0) {
            StoreFormat(error, errorCapacity, "could not create %s", partPath);
            return false;
        }
        ShaSink sink = {};
        sink.fd = fd;
        sink.expected = (uint32_t)stat.m_uncomp_size;
        sink.nextProgress = 64u * 1024u;
        sink.progress = nullptr;
        sink.label = "Extracting player models";
        mbedtls_sha256_init(&sink.sha);
        bool started = mbedtls_sha256_starts_ret(&sink.sha, 0) == 0;
        bool ok = started && mz_zip_reader_extract_to_callback(zip, i, ZipWrite, &sink, 0);
        char digest[65];
        FinishSha(&sink, digest);
        syscall_close((uint32_t)fd);
        if (!ok || sink.received != sink.expected) {
            syscall_unlink(partPath);
            StoreFormat(error, errorCapacity, "failed to extract %s", playerPath);
            return false;
        }
        if (!ReplaceFile(partPath, playerPath)) {
            syscall_unlink(partPath);
            StoreFormat(error, errorCapacity, "could not move %s into place", playerPath);
            return false;
        }
        extracted++;
    }
    if (extracted == 0) {
        SetError(error, errorCapacity, "player models are missing from the demo archive");
        return false;
    }
    return true;
}

}  // namespace

bool ArchiveInstallSharewareData(const char* baseDir, ArchiveProgress progress, char* error,
                                 uint32_t errorCapacity) {
    if (error != nullptr && errorCapacity != 0) error[0] = '\0';
    if (baseDir == nullptr || baseDir[0] == '\0') {
        SetError(error, errorCapacity, "no Quake II base directory was provided");
        return false;
    }

    ArchivePaths paths = {};
    BuildPaths(baseDir, &paths);

    if (StoreFileExists(paths.pakPath, nullptr)) {
        Report(progress, "Game data is already installed.");
        return true;
    }

    if (!StoreEnsureDir(paths.baseDir) || !StoreEnsureDir(paths.sharewareDir) ||
        !StoreEnsureDir(paths.licensesDir)) {
        SetError(error, errorCapacity, "could not create the Quake II data directories");
        return false;
    }

    Report(progress, "Downloading the Quake II demo archive...");
    if (!DownloadFile(kDemoUrl, paths.demoPartPath, paths.demoPath, kDemoSize, kDemoHash,
                      "Downloading demo archive", progress, error, errorCapacity)) {
        return false;
    }

    int32_t demoFd = syscall_open(paths.demoPath, O_RDONLY);
    if (demoFd < 0) {
        SetError(error, errorCapacity, "could not open the downloaded demo archive");
        return false;
    }
    ZipSource source = {demoFd, kDemoSize};
    mz_zip_archive zip = {};
    zip.m_pRead = ZipRead;
    zip.m_pIO_opaque = &source;
    if (!mz_zip_reader_init(&zip, kDemoSize, 0)) {
        syscall_close((uint32_t)demoFd);
        SetError(error, errorCapacity, "the demo archive is not a readable ZIP file");
        return false;
    }

    bool ok = ExtractZipMember(&zip, kPakMember, paths.pakPartPath, paths.pakPath, kPakSize,
                               kPakHash, "Extracting pak0.pak", progress, error, errorCapacity);
    if (ok) {
        ok = ExtractZipMember(&zip, kLicenseMember, paths.licensePartPath, paths.licensePath,
                              kLicenseSize, kLicenseHash, "Extracting license", progress, error,
                              errorCapacity);
    }
    if (ok) {
        ok = ExtractPlayers(&zip, &paths, progress, error, errorCapacity);
    }

    mz_zip_reader_end(&zip);
    syscall_close((uint32_t)demoFd);
    if (!ok) return false;

    Report(progress, "Quake II shareware data installed.");
    return true;
}
