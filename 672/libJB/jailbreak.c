#include "defs.h"
#include "kernelrw.h"
#include "jailbreak.h"

static uintptr_t prison0;
static uintptr_t rootvnode;

static int resolve(void) {
    // Declare local variables
    uintptr_t td, proc, pid1_ucred, pid1_fd;
    int pid;

    // Get the thread descriptor for the current thread
    td = jbc_krw_get_td();
    // Read the proc structure associated with the current thread
    proc = jbc_krw_read64(td + 8, KERNEL_HEAP);

    // Loop until we find the process with PID 1 (init)
    for (;;) {
        // Read the PID of the current process
        if (jbc_krw_memcpy((uintptr_t)&pid, proc + 0xb0, sizeof(pid), KERNEL_HEAP)) {
            // If the memory copy fails, start over
            goto restart;
        }

        // If the PID is 1, we have found the init process
        if (pid == 1){break;}

        // Read the proc2 and proc1 structures associated with the current process
        uintptr_t proc2 = jbc_krw_read64(proc, KERNEL_HEAP);
        uintptr_t proc1 = jbc_krw_read64(proc2 + 8, KERNEL_HEAP);

        // If proc1 and proc2 are not the same, start over
        if (proc1 != proc){goto restart;}

        // Update the proc variable for the next iteration of the loop
        proc = proc2;
    }

    // Read the pid1_ucred and pid1_fd structures associated with the init process
    pid1_ucred = jbc_krw_read64(proc + 0x40, KERNEL_HEAP);
    pid1_fd = jbc_krw_read64(proc + 0x48, KERNEL_HEAP);

    // Copy the prison0 and rootvnode values from these structures into local variables
    if (jbc_krw_memcpy((uintptr_t)&prison0, pid1_ucred + 0x30, sizeof(prison0), KERNEL_HEAP)){
        // If the memory copy fails, return -1 to indicate an error
        return -1;
    }
    if (jbc_krw_memcpy((uintptr_t)&rootvnode, pid1_fd + 0x18, sizeof(rootvnode), KERNEL_HEAP)){
        // If the memory copy fails, set prison0 to 0 and return -1 to indicate an error
        prison0 = 0;
        return -1;
    }

    return 0;
}


uintptr_t jbc_get_prison0(void) {
    if(!prison0) resolve();
    return prison0;
}

uintptr_t jbc_get_rootvnode(void) {
    if(!rootvnode) resolve();
    return rootvnode;
}

static inline int ppcopyout(void* u1, void* u2, uintptr_t k) {
    return jbc_krw_memcpy((uintptr_t)u1, k, (uintptr_t)u2-(uintptr_t)u1, KERNEL_HEAP);
}

static inline int ppcopyin(const void* u1, const void* u2, uintptr_t k) {
    return jbc_krw_memcpy(k, (uintptr_t)u1, (uintptr_t)u2-(uintptr_t)u1, KERNEL_HEAP);
}
int jbc_get_cred(struct jbc_cred* ans) {
    uintptr_t td = jbc_krw_get_td();
    uintptr_t proc = jbc_krw_read64(td + 8, KERNEL_HEAP);
    uintptr_t ucred = jbc_krw_read64(proc + 0x40, KERNEL_HEAP);
    uintptr_t fd = jbc_krw_read64(proc + 0x48, KERNEL_HEAP);

    if (ppcopyout(&ans->uid, 1 + &ans->svuid, ucred + 4)
        || ppcopyout(&ans->rgid, 1 + &ans->svgid, ucred + 20)
        || ppcopyout(&ans->prison, 1 + &ans->prison, ucred + 0x30)
        || ppcopyout(&ans->cdir, 1 + &ans->jdir, fd + 0x10)
        || ppcopyout(&ans->sceProcType, 1 + &ans->sceProcCap, ucred + 88))
        return -1;

    return 0;
}

int jbc_set_cred(const struct jbc_cred* ans) {
    uintptr_t td = jbc_krw_get_td();
    uintptr_t proc = jbc_krw_read64(td + 8, KERNEL_HEAP);
    uintptr_t ucred = jbc_krw_read64(proc + 0x40, KERNEL_HEAP);
    uintptr_t fd = jbc_krw_read64(proc + 0x48, KERNEL_HEAP);
        

   if (ppcopyin(&ans->uid, 1 + &ans->svuid, ucred + 4)
        || ppcopyin(&ans->rgid, 1 + &ans->svgid, ucred + 20)
        || ppcopyin(&ans->prison, 1 + &ans->prison, ucred + 0x30)
        || ppcopyin(&ans->cdir, 1 + &ans->jdir, fd + 0x10) 
        || ppcopyin(&ans->sceProcType, 1 + &ans->sceProcCap, ucred + 88))
        return -1;
    return 0;
}

int jbc_jailbreak_cred(struct jbc_cred* ans) {
    uintptr_t prison0 = jbc_get_prison0();
    if (!prison0) return -1;
    uintptr_t rootvnode = jbc_get_rootvnode();
    if (!rootvnode) return -1;

    //without some modules wont load like Apputils
    ans->sceProcCap = 0xffffffffffffffff;
    ans->sceProcType = 0x3801000000000013;
    ans->sonyCred = 0xffffffffffffffff;

    ans->uid = ans->ruid = ans->svuid = ans->rgid = ans->svgid = 0;
    ans->prison = prison0;
    ans->cdir = ans->rdir = ans->jdir = rootvnode;
    return 0;
}

