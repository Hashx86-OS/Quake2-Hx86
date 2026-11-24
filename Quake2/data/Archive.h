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

#ifndef QUAKE2_ARCHIVE_H
#define QUAKE2_ARCHIVE_H

#include <stdint.h>

/** ArchiveProgress() - Report a human-readable data-install status line. */
typedef void (*ArchiveProgress)(const char* text);

/**
 * ArchiveInstallSharewareData() - Download and install Quake II demo data.
 *
 * @baseDir is the Quake II base directory (normally "/baseq2"). Downloads the
 * original id Software 3.14 shareware self-extracting ZIP from gamers.org,
 * verifies its pinned SHA-256, and installs:
 *   <baseDir>/pak0.pak
 *   <baseDir>/licenses/QUAKE2LIC.TXT
 * The original demo archive is kept under <baseDir>/shareware/.
 *
 * Return: true on success, false with @error filled on failure.
 */
bool ArchiveInstallSharewareData(const char* baseDir, ArchiveProgress progress, char* error,
                                 uint32_t errorCapacity);

#endif  // QUAKE2_ARCHIVE_H
