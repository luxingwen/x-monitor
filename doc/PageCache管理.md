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