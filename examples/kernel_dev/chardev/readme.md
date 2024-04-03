1. 安装

   ```
   make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
   sudo insmod mychardev.ko
   ```

2. 使用

   ```
   echo "hello world" > /dev/mychardev
   cat /dev/mychardev
   ```

3. KERNELRELEASE 作用

   KERNELRELEASE 是一个在内核源码的顶层 Makefile 中定义的一个变量，它表示当前内核的版本号。在编译内核模块的 Makefile 中，ifneq ($ (KERNELRELEASE),) 是一个条件判断，用来区分第一次和第二次执行 Makefile 的过程，

   第一次执行 Makefile 时，KERNELRELEASE 没有被定义，所以走 else 分支，执行 make -C /lib/modules/(shelluname−r)/buildM=[ (PWD) modules 这条指令，这条指令会进入到内核源码目录，调用顶层的 Makefile，在顶层 Makefile 中定义了 KERNELRELEASE 变量，并且递归地再次调用到当前目录下的 Makefile 文件。

   第二次执行 Makefile 时，KERNELRELEASE 已经被定义，所以走 if 分支，在可加载模块编译列表中添加当前模块，kbuild 就会将其编译成可加载模块放在当前目录下。
