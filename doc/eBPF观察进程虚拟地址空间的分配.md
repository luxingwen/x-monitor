# eBPF观察进程虚拟地址空间的分配

## 关键函数

### sys_brk

`sys_brk()`是Linux系统中的一个系统调用，用于修改进程的堆空间大小。堆空间是进程可用的动态分配内存空间，当进程需要更多的内存时，可以使用`sys_brk()`增加堆空间大小，当不需要使用这些内存时，可以使用`sys_brk()`缩小堆空间大小。

```
#include <unistd.h>

void *brk(void *addr);
```

该系统调用的参数是一个指向新的结束地址的指针`addr`，该指针指向将要成为堆的新结束地址。如果`addr`指向的地址高于当前的结束地址，那么`sys_brk()`将增加堆的大小，使其包含该地址；如果`addr`指向的地址低于当前的结束地址，那么`sys_brk()`将减小堆的大小，使其以该地址为结束地址。`sys_brk()`的返回值是指向堆的新结束地址的指针，如果系统调用失败，则返回`(void*)-1`。在成功调用`sys_brk()`后，进程可以使用新的堆空间。需要注意的是，`sys_brk()`仅仅修改堆空间的结束地址，并不保证新的内存空间已经被分配。当新的堆空间被使用时，操作系统会为其分配物理内存。以下是一个简单的示例程序，展示如何使用`sys_brk()`来修改进程的堆空间大小：

```
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    void *start = sbrk(0); // 获取当前堆的结束地址
    printf("Current heap end: %p\n", start);
    
    void *end = sbrk(4096); // 增加堆空间大小
    if (end == (void*)-1) {
        perror("sbrk");
        exit(EXIT_FAILURE);
    }
    
    printf("New heap end: %p\n", end);
    
    end = sbrk(-4096); // 减小堆空间大小
    if (end == (void*)-1) {
        perror("sbrk");
        exit(EXIT_FAILURE);
    }
    
    printf("New heap end: %p\n", end);
    
    return 0;
}
```

### do_brk_flags

The `do_brk_flags` function is a part of the Linux kernel's memory management system and is responsible for handling the allocation and deallocation of memory for a process's user-space address space. It is used to implement the `brk` and `sbrk` system calls.

```
void *do_brk_flags(unsigned long addr, unsigned long len, unsigned long flags);
```

- `addr`: the starting address for the memory region to be allocated or deallocated.

- `len`: the size of the memory region to be allocated or deallocated.

- flags

  : additional flags to control the behavior of the allocation. The following flags are supported:

  - `MAY_EXEC`: allow the memory region to be executable (i.e., used for code).
  - `MAY_WRITE`: allow the memory region to be writable.
  - `MAY_READ`: allow the memory region to be readable.
  - `VM_GROWSDOWN`: allocate the memory region in the process's stack space, growing it downwards.
  - `VM_DONTDUMP`: exclude the memory region from core dumps.
  - `VM_PFNMAP`: map the memory region to physical pages.
  - `VM_LOCKED`: lock the memory region in physical memory.
  - `VM_IO`: mark the memory region as being used for I/O operations.
  - `VM_SEQ_READ`: optimize the memory region for sequential read access.
  - `VM_RAND_READ`: optimize the memory region for random read access.
  - `VM_DONTEXPAND`: do not allow the memory region to be expanded.
  - `VM_ACCOUNT`: account for the memory usage of the memory region.
  - `VM_NORESERVE`: do not reserve swap space for the memory region.

The function returns a pointer to the start of the allocated memory region if successful, or `NULL` if the allocation failed. If `addr` is 0, the function will allocate a new memory region of size `len`. If `addr` is non-zero, the function will attempt to resize the existing memory region starting at `addr` to size `len`. If `len` is 0, the function will deallocate the memory region starting at `addr`.

Note that `do_brk_flags` is a low-level function and should not be used directly by user-space programs. Instead, the `brk` and `sbrk` system calls should be used, which are implemented in terms of `do_brk_flags`.

### do_mmap

The "do_mmap" function is a critical part of the Linux kernel's memory management subsystem that is responsible for creating a new virtual memory mapping for a process. Here are the details of the function and its parameters:

```
unsigned long do_mmap(struct file *file, unsigned long addr, unsigned long len, unsigned long prot, unsigned long flags, vm_flags_t vm_flags, struct page **pages)
```

Parameters:

- `file`: A pointer to the file object associated with the mapping. This parameter is optional and can be set to NULL for anonymous mappings.
- `addr`: The starting address for the new memory mapping. If `addr` is 0, the kernel will choose a suitable address for the mapping.
- `len`: The size of the new memory mapping in bytes.
- `prot`: The protection flags for the new memory mapping. This parameter is a combination of `PROT_READ`, `PROT_WRITE`, and `PROT_EXEC`.
- `flags`: The type of memory mapping to create. This parameter is a combination of `MAP_PRIVATE`, `MAP_SHARED`, `MAP_FIXED`, `MAP_ANONYMOUS`, and other flags.
- `vm_flags`: Additional virtual memory flags for the new memory mapping. This parameter is a combination of `VM_READ`, `VM_WRITE`, `VM_EXEC`, `VM_MAYREAD`, `VM_MAYWRITE`, `VM_MAYEXEC`, and other flags.
- `pages`: A pointer to an array of struct page pointers that will be used to initialize the pages of the new memory mapping. This parameter is optional and can be set to NULL if the pages should be zero-initialized.

Return value:

- On success, the function returns the starting address of the new memory mapping.
- On failure, the function returns one of the following error codes: `-ENOMEM` (out of memory), `-EACCES` (permission denied), `-EINVAL` (invalid argument), or `-ENFILE` (too many open files).

Overall, the `do_mmap` function is a complex and powerful function that allows the kernel to manage virtual memory mappings in a flexible and efficient way. It is used extensively by many different parts of the Linux kernel, including the process scheduler, file system drivers, and device drivers.