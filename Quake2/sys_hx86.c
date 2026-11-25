#include "engine/qcommon/qcommon.h"
#include "engine/game/game.h"
#include <Hx86/Hx86.h>
#include <stdarg.h>

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0x40
#define O_TRUNC 0x200

cvar_t *nostdout;
cvar_t *in_joystick;
unsigned sys_frame_time;
int curtime;

static double sys_time_base = 0.0;
static double sys_time_scale = 0.0;

// =======================================================================
// Statically linked game module
// =======================================================================
game_export_t *GetGameAPI(game_import_t *import);
extern game_export_t *ge;

static void *game_library = NULL;

void Sys_UnloadGame(void) {
    game_library = NULL;
}

void *Sys_GetGameAPI(void *parms) {
    if (game_library)
        Com_Error(ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadGame");
    game_library = (void*)1;
    return GetGameAPI((game_import_t*)parms);
}

// =======================================================================
// Statically linked refresh module
// =======================================================================
#include "../client/ref.h"

extern refexport_t GetRefAPI(refimport_t ri);
refexport_t re;
qboolean reflib_active = false;

void *KBD_Init_fp;
void *KBD_Update_fp;
void *KBD_Close_fp;

// =======================================================================

void Sys_ConsoleOutput(char *string) {
    if (nostdout && nostdout->value)
        return;
    printf("%s", string);
}

void Sys_Printf(char *fmt, ...) {
    va_list argptr;
    char text[1024];
    va_start(argptr, fmt);
    vsprintf(text, fmt, argptr);
    va_end(argptr);
    printf("%s", text);
}

void Sys_Error(char *error, ...) {
    va_list argptr;
    char string[1024];
    va_start(argptr, error);
    vsprintf(string, error, argptr);
    va_end(argptr);
    printf("Error: %s\n", string);
    Qcommon_Shutdown();
    syscall_exit_group(1);
}

void Sys_Warn(char *warning, ...) {
    va_list argptr;
    char string[1024];
    va_start(argptr, warning);
    vsprintf(string, warning, argptr);
    va_end(argptr);
    printf("Warning: %s", string);
}

int Sys_FileTime(char *path) {
    struct stat buf;
    if (syscall_stat(path, &buf) == -1) return -1;
    return 0;
}

void Sys_mkdir(char *path) {
    (void)path;
}

int Sys_FileOpenRead(char *path, int *handle) {
    int h = syscall_open(path, O_RDONLY);
    *handle = h;
    if (h == -1) return -1;
    struct stat fileinfo;
    syscall_stat(path, &fileinfo);
    return fileinfo.st_size;
}

int Sys_FileOpenWrite(char *path) {
    return syscall_open(path, O_RDWR | O_CREAT | O_TRUNC);
}

int Sys_FileWrite(int handle, void *src, int count) {
    return syscall_write(handle, (const char*)src, count);
}

void Sys_FileClose(int handle) {
    syscall_close(handle);
}

void Sys_FileSeek(int handle, int position) {
    int ret;
    __asm__ volatile ("int $0x80"
        : "=a" (ret)
        : "0" (19), "b" (handle), "c" (position), "d" (0)
        : "memory");
    (void)ret;
}

int Sys_FileRead(int handle, void *dest, int count) {
    return syscall_read(handle, (char*)dest, count);
}

void Sys_DebugLog(char *file, char *fmt, ...) {
    va_list argptr;
    static char data[1024];
    int fd;
    va_start(argptr, fmt);
    vsprintf(data, fmt, argptr);
    va_end(argptr);
    fd = syscall_open(file, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd >= 0) {
        syscall_write(fd, data, strlen(data));
        syscall_close(fd);
    }
}

void Sys_EditFile(char *filename) {
    (void)filename;
}

// Kernel PIT-based millisecond clock (Hsys 15). A raw TSC is scaled by a
// fixed constant in the generic code, but the TSC frequency is host-specific.
#define HX86_HSYS_GET_TIME_MS 15

static int32_t HxTimeMs(void) {
    int32_t result = 0;
    __asm__ __volatile__("int $0x80"
                         : "=a"(result)
                         : "a"(sys_Hcall), "b"(HX86_HSYS_GET_TIME_MS)
                         : "memory");
    return result;
}

int Sys_Milliseconds(void) {
    return HxTimeMs();
}

void Sys_Sleep(void) {
    syscall_sleep(1);
}

char *Sys_ConsoleInput(void) {
    return NULL;
}

void Sys_Init(void) {
}

void Sys_Quit(void) {
    Qcommon_Shutdown();
    syscall_exit_group(0);
}

void Sys_AppActivate(void) {
}

void Sys_SendKeyEvents(void) {
    sys_frame_time = Sys_Milliseconds();
}

char *Sys_GetClipboardData(void) {
    return NULL;
}

void Sys_CopyProtect(void) {
}

void Sys_MakeCodeWriteable(unsigned long startaddr, unsigned long length) {
    (void)startaddr;
    (void)length;
}

void Sys_SetFPCW(void) {
    unsigned short cw;
    __asm__ volatile ("fstcw %0" : "=m" (cw));
    cw &= ~0x300;
    cw |= 0x200;
    __asm__ volatile ("fldcw %0" : : "m" (cw));
}

char *Sys_FindFirst(char *path, unsigned musthave, unsigned canthave) {
    (void)path;
    (void)musthave;
    (void)canthave;
    return NULL;
}

char *Sys_FindNext(unsigned musthave, unsigned canthave) {
    (void)musthave;
    (void)canthave;
    return NULL;
}

void Sys_FindClose(void) {
}

void Sys_Mkdir(char *path) {
    (void)path;
}

void Hunk_Free(void *buf) {
    if (buf) kfree(buf);
}
