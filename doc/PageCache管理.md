# PageCache管理

## inode、pageCache，page的关系

![address_space](./img/address_space.png)

## PageCache

### 类型

内核代码并不直接读写磁盘，基本都是通过页高速缓存，PageCache的类型可能是以下几种

1. 含有普通文件数据的Page。
2. 含有目录的Page。
3. 含有直接从块设备（skip file system）读取的数据Page。
4. 含有用户态进程数据的页，但页中的数据已经被交换到磁盘。
5. 属于特殊文件系统文件的Page，例如shm。

所以，PageCache中每个Page包含的数据肯定属于某个文件。这个文件（更确切的说是inode）就是Page的所有者（owner）。

几乎所有的文件读、写都依赖于PageCache，只有使用O_DIRECT标志的进程打开文件的时候才会出现例外，从而绕过了PageCache，少数数据库采用自己的缓存算法。

### 目的

内核设计者实现PageCache主要是满足下面两个需求

1. 快速定位给定inode相关数据的特定页。为了尽可能充分发挥PageCache的优势，使用radix_tree。
2. 记录在读或写Page中的数据是应该如何处理高速缓存中的每个页。例如，从普通文件、块设备文件或者交换区读取一个数据页必须用不同的实现方式，因此内核必须根据页的所有者（inode的类型不同）选择适当的操作。

### Page识别

通过页的所有者和所有者数据中的索引（通常是一个索引节点和在相应文件中的偏移量）来识别PageCache中的Page。

## address_space

address_space是linux内核中的一个关键抽象，它是PageCache和外部设备中文件系统的桥梁，上层应用读、写数据会进入到该结构

### 结构成员描述

| field           | type                                  | desc                                                         |
| --------------- | ------------------------------------- | ------------------------------------------------------------ |
| i_pages         | struct radix_tree_root                | 指向了这个地址空间对应的页缓存的基数树。这样就可以通过 inode --> address_space -->i_pages 找到文件对应缓存页 |
| a_ops           | const struct address_space_operations | 定义了抽象的文件系统交互接口，由具体文件系统负责实现。例如如果文件是存储在ext4文件系统之上，那么该结构便被初始化为***ext4_aops\*** （见fs/ext4/inode.c） |
| host            | struct inode *                        | owner: inode, block_device                                   |
| nrpages         | unsigned long                         | address_space中缓存的总页面数                                |
| writeback_index | pgoff_t                               | 该address_space中回写（writeback）操作的起始偏移量，即将缓存中的脏页面写回到文件或设备中的位置。 |
| i_mmap_writable | atomic_t                              | 这个成员是一个原子变量，用于记录该address_space中有多少个共享映射（VM_SHARED），即多个进程共享同一段内存映射的情况 |
| gfp_mask        | gfp_t                                 | 这个成员是一个标志位，用于指定为该address_space分配内存时使用的内存分配器类型和参数，例如是否允许睡眠、是否允许交换等。 |
| flags           | unsigned long                         | 这个成员是一个位图，用于记录该address_space中发生的一些错误或异常情况，例如内存不足、读写错误等。 |

![img](./img/address_space.webp)

### address_space操作函数

| type                                                         | desc                                                         |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| int (*writepage)(struct page *page, struct writeback_control *wbc) | 函数指针，指向一个用于将一个脏页面（dirty page）写回到文件或设备中的函数。脏页面是指缓存在内存中的页面，但是与文件或设备中的内容不一致的页面。wbc参数是一个用于控制回写操作的结构体，包含了一些信息，例如回写的范围、模式、同步或异步等。 |
| int (*readpage)(struct file *, struct page *)                | 函数指针，指向一个用于从文件或设备中读取一个页面到内存中的函数。file参数是一个指向打开的文件或设备的结构体，page参数是一个指向要读取到的页面的结构体。 |
| int (*set_page_dirty)(struct page *page)                     | 函数指针，指向一个用于将一个页面标记为脏的函数。page参数是一个指向要标记的页面的结构体。这个函数通常在内存中修改了页面内容后调用，以便在适当的时候回写到文件或设备中。 |
| int (*write_begin)(struct file *, struct address_space *mapping, | 函数指针，指向一个用于开始写入操作前准备工作的函数。file参数与readpage相同，mapping参数与readpages相同，pos参数是要写入的位置偏移量，len参数是要写入的长度，flags参数是一些标志位，例如是否需要分配新页面等，pagep参数是一个指向要写入到的页面的指针的指针，fsdata参数是一个指向文件系统特定数据的指针的指针。这个函数通常会锁定要写入到的页面，并返回给调用者。 |
| int (*write_end)(struct file *, struct address_space *mapping, | 函数指针，指向一个用于结束写入操作后清理工作的函数。file参数与write_begin相同，mapping参数与write_begin相同，pos参数与write_begin相同，len参数与write_begin相同，copied参数是实际写入到页面中的字节数，page参数与write_begin相同，fsdata参数与write_begin相同。这个函数通常会解锁已经写入到的页面，并更新文件或设备中的内容。 |

### 在PageCache中查找一个Page

```
/**
 * find_get_page - find and get a page reference
 * @mapping: the address_space to search
 * @offset: the page index
 *
 * Looks up the page cache slot at @mapping & @offset.  If there is a
 * page cache page, it is returned with an increased refcount.
 *
 * Otherwise, %NULL is returned.
 */
static inline struct page *find_get_page(struct address_space *mapping,
					pgoff_t offset)
{
	return pagecache_get_page(mapping, offset, 0, 0);
}
```



## radix tree

linux支持给个T的文件。访问大文件时，PageCache中充满了太多的Page，如果顺序扫描这些页要消耗大量的时间。为了实现Page的高效查找，使用了radix tree。

每个address_space对象对应一个搜索树，成员i_pages是基树的根。

![img](./img/radix_tree.jpg)



## 资料

1. [Linux内核页高速缓存 (feilengcui008.github.io)](https://feilengcui008.github.io/post/linux内核页高速缓存/)