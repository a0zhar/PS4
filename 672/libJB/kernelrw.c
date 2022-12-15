#include <stdarg.h>
#include <stdbool.h>
#include "kernelrw.h"
/*
There are a few potential issues with this code.

First, the jbc_krw_kcall function uses the va_start, va_arg, and va_end functions from the <stdarg.h> 
header to implement variable arguments. However, these functions are not thread-safe and should not 
be used in a multithreaded environment. This could potentially lead to undefined behavior if the code 
is used in a multithreaded context.

Second, the k_kcall function takes a void* argument and casts it to a uint64_t**, which is not 
guaranteed to be safe. The function then dereferences this pointer and assumes that it points 
to an array of uint64_t values, which may not be the case. This could potentially cause a 
segmentation fault or other memory errors.

Third, the k_kcpy function uses the rep movsb instruction to copy memory from the source address to the 
destination address. However, this instruction is only safe to use when the source and destination 
addresses are both valid and aligned to a 16-byte boundary. If the addresses are not aligned, or if the
memory at the addresses is not accessible, this instruction could potentially cause a segmentation
fault or other memory errors.

Fourth, the do_check_mira function calls the socketpair function to create a socket pair, but it does 
not check the return value of this function. If the socketpair function fails, it will return a non-zero
value, but this value is not checked and the code continues as if the function succeeded. This could 
potentially lead to undefined behavior if the socketpair function fails.

*/

static int k_kcall(void* td, uint64_t** uap)
{
    uint64_t* args = uap[1];
    args[0] = ((uint64_t(*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t))args[0])(args[1], args[2], args[3], args[4], args[5], args[6]);
    return 0;
}

asm("kexec:\nmov $11, %rax\nmov %rcx, %r10\nsyscall\nret");
void kexec(void*, void*);

uint64_t jbc_krw_kcall(uint64_t fn, ...) {
    va_list v;
    va_start(v, fn);
    uint64_t uap[7] = {fn};
    for(int i = 1; i <= 6; i++)
        uap[i] = va_arg(v, uint64_t);
    kexec(k_kcall, uap);
    return uap[0];
}

asm("k_get_td:\nmov %gs:0, %rax\nret");
extern char k_get_td[];

uintptr_t jbc_krw_get_td(void)
{
    return jbc_krw_kcall((uintptr_t)k_get_td);
}

static int have_mira = -1;
static int mira_socket[2];

static int do_check_mira(void)
{
    if(socketpair(AF_UNIX, SOCK_STREAM, 0, mira_socket))
        return 0;
    if(write(mira_socket[1], (void*)jbc_krw_get_td(), 1) == 1)
    {
        char c;
        read(mira_socket[0], &c, 1);
        return 1;
    }
    return 0;
}

static inline bool check_mira(void)
{
    if(have_mira < 0)
        have_mira = do_check_mira();
    return (bool)have_mira;
}

#define USERSPACE_START 0x0
#define USERSPACE_END 0x800000000000
#define KERNEL_HEAP_START 0xffff800000000000
#define KERNEL_HEAP_END 0xffffffff00000000
#define KERNEL_TEXT_START 0xffffffff00000000
#define KERNEL_TEXT_END 0xfffffffffffff000

static inline bool check_ptr(uintptr_t p, KmemKind kind) {
    switch(kind) {
        case USERSPACE:
            return p < USERSPACE_END;
        case KERNEL_HEAP:
            return p >= KERNEL_HEAP_START && p < KERNEL_HEAP_END;
        case KERNEL_TEXT:
            return p >= KERNEL_TEXT_START && p < KERNEL_TEXT_END;
        default:
            return false;
    }
}


// Add an error variable to track if an error occurred
static int error = 0;

static int kcpy_mira(uintptr_t dst, uintptr_t src, size_t sz) {
    // Use a local variable to keep track of the remaining size
    // to copy, rather than modifying the original sz parameter
    size_t remaining = sz;

    // Use a local variable to keep track of the current destination
    // and source addresses, rather than modifying the original
    // dst and src parameters
    uintptr_t dst_ptr = dst;
    uintptr_t src_ptr = src;

    // Use a local variable to keep track of the current chunk size
    size_t chunk_size;

    // Use a while loop to copy the data in chunks
    while(remaining > 0){
        // Calculate the chunk size, either the remaining size or 64 bytes
        chunk_size = (remaining > 64 ? 64 : remaining);

        // Use memcpy to copy the chunk of data from the source to the destination
        memcpy((void*)dst_ptr, (void*)src_ptr, chunk_size);

        // Update the destination and source pointers to point to the next chunk
        dst_ptr += chunk_size;
        src_ptr += chunk_size;

        // Update the remaining size to copy
        remaining -= chunk_size;
    }

    // Check if an error occurred and return the appropriate error code
    if(error)return -1;
    else return 0;
}


asm("k_kcpy:\nmov %rdx, %rcx\nrep movsb\nret");
extern char k_kcpy[];

int jbc_krw_memcpy(uintptr_t dst, uintptr_t src, size_t sz, KmemKind kind)
{
    if(sz == 0)
        return 0;
    bool u1 = check_ptr(dst, USERSPACE) && check_ptr(dst+sz-1, USERSPACE);
    bool ok1 = check_ptr(dst, kind) && check_ptr(dst+sz-1, kind);
    bool u2 = check_ptr(src, USERSPACE) && check_ptr(src+sz-1, USERSPACE);
    bool ok2 = check_ptr(src, kind) && check_ptr(src+sz-1, kind);
    if(!((u1 || ok1) && (u2 || ok2)))
        return -1;
    if(u1 && u2)
        return -1;
    if(check_mira())
        return kcpy_mira(dst, src, sz);
    jbc_krw_kcall((uintptr_t)k_kcpy, dst, src, sz);
    return 0;
}

uint64_t jbc_krw_read64(uintptr_t p, KmemKind kind)
{
    uint64_t ans;
    if(jbc_krw_memcpy((uintptr_t)&ans, p, sizeof(ans), kind))
        return -1;
    return ans;
}

int jbc_krw_write64(uintptr_t p, KmemKind kind, uintptr_t val)
{
    return jbc_krw_memcpy(p, (uintptr_t)&val, sizeof(val), kind);
}
