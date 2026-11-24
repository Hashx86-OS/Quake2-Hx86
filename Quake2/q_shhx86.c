#include "engine/qcommon/qcommon.h"
#include <Hx86/Hx86.h>

byte *membase;
int maxhunksize;
int curhunksize;

void *Hunk_Begin(int maxsize) {
    maxhunksize = maxsize + sizeof(int);
    curhunksize = 0;
    membase = (byte*)kmalloc(maxhunksize);
    if (!membase)
        Sys_Error("Hunk_Begin: unable to allocate %d bytes", maxsize);
    *(int*)membase = curhunksize;
    return membase + sizeof(int);
}

void *Hunk_Alloc(int size) {
    byte *buf;
    size = (size + 31) & ~31;
    if (curhunksize + size > maxhunksize)
        Sys_Error("Hunk_Alloc overflow");
    buf = membase + sizeof(int) + curhunksize;
    curhunksize += size;
    return buf;
}

int Hunk_End(void) {
    byte *n;
    n = (byte*)krealloc(membase, curhunksize + sizeof(int));
    if (n != membase) {
        Sys_Error("Hunk_End: krealloc moved the block!");
    }
    *(int*)membase = curhunksize;
    return curhunksize;
}

void Hunk_Free(void *base) {
    byte *m;
    if (base) {
        m = ((byte *)base) - sizeof(int);
        kfree(m);
    }
}
