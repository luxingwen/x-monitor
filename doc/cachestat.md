# CacheStat统计

Trace page cache hit/miss ratio，基于4.18内核代码。

## 内核函数

1.  **add_to_page_cache_lru**

    函数的调用时机，说明当page cache中不存在时会调用该函数。**所以应该是misses字段要加一**。

    The `add_to_page_cache_lru()` function in the Linux kernel is responsible for adding a page to the page cache LRU (Least Recently Used) list. This function is called by the kernel in various situations, depending on the operation being performed.

    Some examples of situations where `add_to_page_cache_lru()` might be called include:

    -   When a page fault occurs: If a process tries to access a page that is not currently in memory, the kernel will need to load that page from disk. When the page is loaded, it will be added to the page cache LRU list using `add_to_page_cache_lru()`.
    -   When a file is read or written: When a process reads from or writes to a file, the kernel will need to access the pages of that file. If the pages are not already in the page cache, they will be loaded and added to the LRU list using `add_to_page_cache_lru()`.
    -   When a page is reclaimed: The kernel has a mechanism to reclaim pages that are no longer being actively used, in order to free up memory. When a page is reclaimed, it will be removed from the LRU list, and if necessary, written back to disk.

    These are just a few examples of situations where `add_to_page_cache_lru()` might be called. The exact details of when and how it is used can vary depending on the specific implementation and configuration of the kernel.

2.  **mark_page_accessed**

    ```
    /*
     * Mark a page as having seen activity.
     *
     * inactive,unreferenced	->	inactive,referenced
     * inactive,referenced		->	active,unreferenced
     * active,unreferenced		->	active,referenced
     *
     * When a newly allocated page is not yet visible, so safe for non-atomic ops,
     * __SetPageReferenced(page) may be substituted for mark_page_accessed(page).
     */
    ```

    Page状态说明：

    In Linux, pages in the page cache are classified into two lists: active and inactive. 

    Active pages are more likely to be accessed again, while inactive pages are more likely to be evicted when memory is low. 

    Each page also has a flag that indicates whether it is referenced or unreferenced. 

    Referenced pages have been recently accessed, while unreferenced pages have not.

    A page can change its state from inactive,unreferenced to inactive,referenced when it is accessed for the first time. If it is accessed again, it becomes active,unreferenced. If an active,unreferenced page is accessed again, it becomes active,referenced.

    通过四个字段，三种组合标识了Page状态的迁移，所以该函数的hook**应该被total字段统计**，Page的第一次也包含在其中。

3.  **folio_account_dirtied**

    Function account_page_dirtied() is changed to folio_account_dirtied() in 5.15.

4.  **account_page_dirtied**

    通过下面的描述，可以理解为在Page Cache中的Page被修改过了，也意味着被命中了，**所以misses字段要减一**。

    The `account_page_dirtied()` function is called by the Linux kernel's memory management subsystem whenever a page of memory is modified or "dirtied" by a process.

    In summary, `account_page_dirtied()` is called whenever a page of memory is modified by a process, and its purpose is to update various statistics and counters used by the kernel's memory management subsystem to optimize system performance and resource usage.

5.  **mark_buffer_dirty**

    通过下面描述，说明是用来加速块设备访问的。也说明了page cache的缓存的两种主要对象资源：文件和块设备。**如果仅仅统计文件page的命中率，total字段在这里需要减一**。

    *From ChatGPT：*

    In the Linux kernel, the `mark_buffer_dirty()` function is called to mark a buffer as "dirty", which means that its contents have been modified and need to be written back to disk.

    There are several different situations in which the kernel might call `mark_buffer_dirty()`, including:

    1.  When a user-space process writes to a file that is backed by a buffer in memory. In this case, the kernel will mark the corresponding buffer as dirty and eventually write the changes back to disk.
    2.  When a file system operation such as file deletion or truncation occurs. In this case, the kernel may mark the affected buffers as dirty to ensure that any modifications to the file system are written back to disk.
    3.  When a block device driver writes data to a buffer. In this case, the driver must call `mark_buffer_dirty()` to notify the kernel that the buffer has been modified.
    4.  When a system call such as `mmap()` or `read()` is used to map a file into memory. In this case, the kernel may mark the corresponding buffers as dirty to ensure that any modifications to the file are written back to disk.

    Overall, the `mark_buffer_dirty()` function is used by the kernel to keep track of which buffers in memory have been modified and need to be written back to disk.

    *From NewBing：*

    The function **mark_buffer_dirty** is used to mark a buffer as dirty, meaning that it has been modified and needs to be written back to disk². It also sets the dirty bit of the buffer's backing page, tags the page as dirty in its address_space's radix tree, and attaches the address_space's inode to its superblock's dirty inode list².

    The Linux kernel calls this function when it wants to keep track of which buffers have been changed and need to be flushed to disk. It also invokes another function called **balance_dirty** to keep the number of dirty buffers in the system bounded¹.


## 统计公式

```
total number of accesses = number of mark_page_accessed() - number of mark_buffer_dirty()
total number of misses = number of add_to_page_cache_lru() - number of account_page_dirtied()
```

