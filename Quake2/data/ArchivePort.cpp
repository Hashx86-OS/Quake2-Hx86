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

// C-linkage allocator glue for the vendored miniz C sources. libhx86's
// allocator wrappers are C++ definitions, so the C archive code needs these
// unmangled entry points. The C memory/string helpers come from libmbedtls.

#include <stddef.h>
#include <stdint.h>

void* kmalloc(int size);
void* kcalloc(int count, int size);
void* krealloc(void* pointer, int size);
void kfree(void* pointer);

extern "C" {

void free(void* pointer);

void* malloc(size_t size) {
    if (size == 0 || size > 0x7FFFFFFFu) return nullptr;
    return kmalloc((int)size);
}

void* calloc(size_t count, size_t size) {
    if (count == 0 || size == 0) return nullptr;
    if (count > 0x7FFFFFFFu / size) return nullptr;
    return kcalloc((int)count, (int)size);
}

void* realloc(void* pointer, size_t size) {
    if (size > 0x7FFFFFFFu) return nullptr;
    if (pointer == nullptr) return malloc(size);
    if (size == 0) {
        free(pointer);
        return nullptr;
    }
    return krealloc(pointer, (int)size);
}

void free(void* pointer) {
    if (pointer != nullptr) kfree(pointer);
}

}  // extern "C"
