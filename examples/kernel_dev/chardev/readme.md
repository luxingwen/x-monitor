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

   