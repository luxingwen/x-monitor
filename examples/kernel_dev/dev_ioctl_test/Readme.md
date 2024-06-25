1. 编译

   make noisy

2. 安装模块，拷贝到内核对应目录

   ```
   make modules_install
   
    ⚡ root@localhost  ~  ls /lib/modules/4.18.0-425.19.2.el8_7.x86_64/extra/calmwu_modules/cw_dev_ioctl_test 
   cw_dev_ioctl_test.ko
   ```

3. 更新initramfs

   ```
   sodu make install
   
    calmwu@localhost  ~  sudo lsinitrd 
   [sudo] password for calmwu: 
   Image: /boot/initramfs-4.18.0-425.19.2.el8_7.x86_64.img: 31M
   ========================================================================
   drwxr-xr-x   3 root     root            0 Jan 13  2023 usr/lib/modules/4.18.0-425.19.2.el8_7.x86_64/extra
   drwxr-xr-x   3 root     root            0 Jan 13  2023 usr/lib/modules/4.18.0-425.19.2.el8_7.x86_64/extra/calmwu_modules
   drwxr-xr-x   2 root     root            0 Jan 13  2023 usr/lib/modules/4.18.0-425.19.2.el8_7.x86_64/extra/calmwu_modules/cw_dev_ioctl_test
   -rw-r--r--   1 root     root        18648 Jan 13  2023 usr/lib/modules/4.18.0-425.19.2.el8_7.x86_64/extra/calmwu_modules/cw_dev_ioctl_test/cw_dev_ioctl_test.ko
   ```

4. 配置模块参数，添加配置文件

   ```
    ⚡ root@localhost  ~  ccat /etc/modprobe.d/cw_dev_ioctl_test.conf 
   options cw_dev_ioctl_test __cw_ioctl_nr_devs=1
   ```

5. 启动自动加载ko，添加配置文件

   ```
    ⚡ root@localhost  ~  ccat /etc/modules-load.d/cw_dev_ioctl_test.conf 
   cw_dev_ioctl_test
   ```

6. DKMS管理

   

   

