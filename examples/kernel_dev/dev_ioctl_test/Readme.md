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
    ⚡ root@localhost  ~  cat /etc/modprobe.d/cw_dev_ioctl_test.conf 
   options cw_dev_ioctl_test __cw_ioctl_nr_devs=1
   ```

5. 启动自动加载ko，添加配置文件

   ```
    ⚡ root@localhost  ~  cat /etc/modules-load.d/cw_dev_ioctl_test.conf 
   cw_dev_ioctl_test
   ```

6. 内核升级钩子，该目录下的脚本在内核升级后会自动执行

   ```
    ✘ ⚡ root@localhost  /etc/kernel  cd postinst.d 
    ⚡ root@localhost  /etc/kernel/postinst.d  ls -trl
   total 4
   -rwxr-xr-x. 1 root root 1566 Jan 13  2023 51-dracut-rescue-postinst.sh
   ```

7. DKMS

   DKMS全称是Dynamic Kernel Module Support，在内核版本变动之后可以自动重新生成新的模块。

   [DKMS简介 - kk Blog —— 通用基础 (abcdxyzk.github.io)](https://abcdxyzk.github.io/blog/2020/09/21/kernel-dkms/)

   ![image-20240625171246482](./image-20240625171246482.png)

8. megaraid_sas-07.729.00.00

   1. 下载https://docs.broadcom.com/docs-and-downloads/07.729.00.00-1_MR7.29_Linux_driver.zip

   2. 构建目录，/usr/src/megaraid_sas-07.729.00.00

      ```
       ⚡ root@localhost  /usr/src  cd megaraid_sas-07.729.00.00 
       ⚡ root@localhost  /usr/src/megaraid_sas-07.729.00.00  ls -trl
      total 608
      -rwxr-xr-x 1 root root 272271 Nov 19  2023 megaraid_sas_base.c
      -rwxr-xr-x 1 root root 163423 Nov 19  2023 megaraid_sas_fusion.c
      -rwxr-xr-x 1 root root  44381 Nov 19  2023 megaraid_sas_fp.c
      -rwxr-xr-x 1 root root   4683 Nov 19  2023 megaraid_sas_debugfs.c
      -rwxr-xr-x 1 root root  69420 Nov 19  2023 megaraid_sas.h
      -rwxr-xr-x 1 root root  43194 Nov 19  2023 megaraid_sas_fusion.h
      -rwxr-xr-x 1 root root   1440 Nov 19  2023 compile.sh
      -rwxr-xr-x 1 root root   1021 Nov 19  2023 Makefile.standalone
      -rwxr-xr-x 1 root root    132 Nov 19  2023 Makefile
      -rwxr-xr-x 1 root root   1819 Jul  1 15:27 dkms.conf
      ```

   3. 配置dkms.conf文件，修改PACKAGE_VERSION

      ```
       ⚡ root@localhost  /usr/src/megaraid_sas-07.729.00.00  cat dkms.conf 
      #
      # Master copy of dkms.conf for megaraid_sas.
      # Dont edit this file manually. Auto build script makes necessary changes.
      #
      
      PACKAGE_NAME="megaraid_sas"
      PACKAGE_VERSION=07.729.00.00
      MOD_PATH=${dkms_tree}/${PACKAGE_NAME}/${PACKAGE_VERSION}
      
      MAKE[0]="make -C ${kernel_source_dir} SUBDIRS=${MOD_PATH}/build modules"
      CLEAN="make -C ${kernel_source_dir} SUBDIRS=${MOD_PATH}/build clean"
      
      BUILT_MODULE_NAME[0]="megaraid_sas"
      DEST_MODULE_LOCATION[0]="/kernel/drivers/scsi/megaraid/"
      MODULES_CONF_ALIAS_TYPE[0]="scsi_hostadapter"
      
      REMAKE_INITRD="yes"
      ```

   4. add/status

      ```
       ⚡ root@localhost  /usr/src   dkms add -m megaraid_sas -v 07.729.00.00          
      Deprecated feature: REMAKE_INITRD (/usr/src/megaraid_sas-07.729.00.00/dkms.conf)
      Deprecated feature: MODULES_CONF_ALIAS_TYPE (/usr/src/megaraid_sas-07.729.00.00/dkms.conf)
      Creating symlink /var/lib/dkms/megaraid_sas/07.729.00.00/source -> /usr/src/megaraid_sas-07.729.00.00
       ⚡ root@localhost  /usr/src  dkms status 
      dev_ioctl_test/0.1: added
      megaraid_sas/07.729.00.00: added
      ```

   5. 编译，ko生成在/var/lib/dkms/megaraid_sas/07.729.00.00/build/目录下

      ```
       ⚡ root@localhost  /usr/src  dkms build -m megaraid_sas -v 07.729.00.00
      Sign command: /lib/modules/4.18.0-425.19.2.el8_7.x86_64/build/scripts/sign-file
      Signing key: /var/lib/dkms/mok.key
      Public certificate (MOK): /var/lib/dkms/mok.pub
      Deprecated feature: REMAKE_INITRD (/var/lib/dkms/megaraid_sas/07.729.00.00/source/dkms.conf)
      Deprecated feature: MODULES_CONF_ALIAS_TYPE (/var/lib/dkms/megaraid_sas/07.729.00.00/source/dkms.conf)
      
      Building module:
      Cleaning build area...........
      make -j8 KERNELRELEASE=4.18.0-425.19.2.el8_7.x86_64 -C /lib/modules/4.18.0-425.19.2.el8_7.x86_64/build SUBDIRS=/var/lib/dkms/megaraid_sas/07.729.00.00/build modules........................
      Signing module /var/lib/dkms/megaraid_sas/07.729.00.00/build/megaraid_sas.ko
      Cleaning build area...........
      ```

   6. 查看kernel所带默认驱动版本，07.719.03.00-rh1

      ```
       ⚡ root@localhost  /lib/modules/4.18.0-425.19.2.el8_7.x86_64/kernel/drivers/scsi/megaraid  modinfo megaraid_sas
      filename:       /lib/modules/4.18.0-425.19.2.el8_7.x86_64/kernel/drivers/scsi/megaraid/megaraid_sas.ko.xz
      description:    Broadcom MegaRAID SAS Driver
      author:         megaraidlinux.pdl@broadcom.com
      version:        07.719.03.00-rh1
      ```

   7. 安装，modinfo看到已经使用extra目录下的驱动模块

      ```
       ⚡ root@localhost  /usr/src/megaraid_sas-07.729.00.00  dkms install -m megaraid_sas -v 07.729.00.00
      Deprecated feature: REMAKE_INITRD (/var/lib/dkms/megaraid_sas/07.729.00.00/source/dkms.conf)
      Deprecated feature: MODULES_CONF_ALIAS_TYPE (/var/lib/dkms/megaraid_sas/07.729.00.00/source/dkms.conf)
      
      megaraid_sas.ko.xz:
      Running module version sanity check.
       - Original module
         - Found /lib/modules/4.18.0-425.19.2.el8_7.x86_64/kernel/drivers/scsi/megaraid/megaraid_sas.ko.xz
         - Storing in /var/lib/dkms/megaraid_sas/original_module/4.18.0-425.19.2.el8_7.x86_64/x86_64/
         - Archiving for uninstallation purposes
       - Installation
         - Installing to /lib/modules/4.18.0-425.19.2.el8_7.x86_64/extra/
      Adding any weak-modules
      depmod: ERROR: fstatat(6, kvdo.ko): No such file or directory
      depmod: ERROR: fstatat(6, uds.ko): No such file or directory
      depmod: WARNING: /lib/modules/4.18.0-425.19.2.el8_7.x86_64/extra/calmwu_modules/interrupt_test/interrupt_test.ko needs unknown symbol vector_irq
      depmod..........
       ⚡ root@localhost  /usr/src/megaraid_sas-07.729.00.00  modinfo megaraid_sas
      filename:       /lib/modules/4.18.0-425.19.2.el8_7.x86_64/extra/megaraid_sas.ko.xz
      description:    Broadcom MegaRAID SAS Driver
      author:         megaraidlinux.pdl@broadcom.com
      ```

   8. 从DKMS中删除内核模块

      ```
      dkms remove -m megaraid_sas -v 07.729.00.00
      Deprecated feature: REMAKE_INITRD (/var/lib/dkms/megaraid_sas/07.729.00.00/source/dkms.conf)
      Deprecated feature: MODULES_CONF_ALIAS_TYPE (/var/lib/dkms/megaraid_sas/07.729.00.00/source/dkms.conf)
      Module megaraid_sas-07.729.00.00 for kernel 4.18.0-425.19.2.el8_7.x86_64 (x86_64).
      Before uninstall, this module version was ACTIVE on this kernel.
      Removing any linked weak-modules
      
      megaraid_sas.ko.xz:
       - Uninstallation
         - Deleting from: /lib/modules/4.18.0-425.19.2.el8_7.x86_64/extra/
       - Original module
         - Archived original module found in the DKMS tree
         - Moving it to: /lib/modules/4.18.0-425.19.2.el8_7.x86_64/updates/
      depmod..........
      
      Removing original_module from DKMS tree for kernel 4.18.0-425.19.2.el8_7.x86_64 (x86_64)
      Deleting module megaraid_sas-07.729.00.00 completely from the DKMS tree.
      ```

   9. 重启系统，需要配置/etc/modules-load.d/megaraid_sas.conf文件自动加载

      ```
       ⚡ root@localhost  ~  lsmod|grep mega
      megaraid_sas          180224  0
       ⚡ root@localhost  ~  modinfo megaraid_sas
      filename:       /lib/modules/4.18.0-425.19.2.el8_7.x86_64/extra/megaraid_sas.ko.xz
      description:    Broadcom MegaRAID SAS Driver
      author:         megaraidlinux.pdl@broadcom.com
      version:        07.729.00.00
      ```

   10. 安装内核，前四位相同。该版本自带了megaraid_sas驱动模块，看安装后729版本的驱动是否生效，**切记要安装headers包**，**可实际上还是需要devel包**，不然/usr/src/kernels/目录下没有代码，没法进行内核模块的编译。

       如下输出可以看到，安装呢内核后，会首先发现默认的驱动模块/lib/modules/4.18.0-425.3.1.el8.x86_64/kernel/drivers/scsi/megaraid/megaraid_sas.ko.xz，同时也会安装dkms管理的内核模块。

       ```
       dnf -y install kernel-devel-4.18.0-425.3.1.el8.x86_64 kernel-headers-4.18.0-425.3.1.el8.x86_64 kernel-4.18.0-425.3.1.el8.x86_64
       
       Building module:
       Cleaning build area....
       make -j8 KERNELRELEASE=4.18.0-425.3.1.el8.x86_64 -C /lib/modules/4.18.0-425.3.1.el8.x86_64/build SUBDIRS=/var/lib/dkms/megaraid_sas/07.729.00.00/build modules......
       Signing module /var/lib/dkms/megaraid_sas/07.729.00.00/build/megaraid_sas.ko
       Cleaning build area....
       
       megaraid_sas.ko.xz:
       Running module version sanity check.
        - Original module
          - Found /lib/modules/4.18.0-425.3.1.el8.x86_64/kernel/drivers/scsi/megaraid/megaraid_sas.ko.xz
          - Storing in /var/lib/dkms/megaraid_sas/original_module/4.18.0-425.3.1.el8.x86_64/x86_64/
          - Archiving for uninstallation purposes
        - Installation
          - Installing to /lib/modules/4.18.0-425.3.1.el8.x86_64/extra/
       ```

       实际上默认去的驱动模块会被删除

       ```
        ⚡ root@localhost  /lib/modules  find . -name megaraid_sas.ko.xz
       ./4.18.0-425.19.2.el8_7.x86_64/extra/megaraid_sas.ko.xz
       ./4.18.0-425.19.2.el8_7.x86_64/weak-updates/megaraid_sas.ko.xz
       ./4.18.0-425.3.1.el8.x86_64/extra/megaraid_sas.ko.xz
       ```

   11. 重启，选择这个4.18.0-425.3.1.el8.x86_64

       ```
        ⚡ root@localhost  ~  uname -r
       4.18.0-425.3.1.el8.x86_64
        ⚡ root@localhost  ~  lsmod|grep megaraid_sas
       megaraid_sas          180224  0
        ⚡ root@localhost  ~  modinfo megaraid_sas
       filename:       /lib/modules/4.18.0-425.3.1.el8.x86_64/extra/megaraid_sas.ko.xz
       description:    Broadcom MegaRAID SAS Driver
       author:         megaraidlinux.pdl@broadcom.com
       version:        07.729.00.00
       ```

       可见使用的dkms管理的，729版本。通过dkms status命令可以看到

       ```
        ⚡ root@localhost  ~  dkms status
       dev_ioctl_test/0.1: added
       megaraid_sas/07.729.00.00, 4.18.0-425.19.2.el8_7.x86_64, x86_64: installed (original_module exists)
       megaraid_sas/07.729.00.00, 4.18.0-425.3.1.el8.x86_64, x86_64: installed (original_module exists)
       ```

9. i40e-2.25.9

   1. 配置dkms.conf

      ```
      #
      # Master copy of dkms.conf for i40e.
      # Dont edit this file manually. Auto build script makes necessary changes.
      #
      
      PACKAGE_NAME="i40e"
      PACKAGE_VERSION=2.25.9
      MOD_PATH=${dkms_tree}/${PACKAGE_NAME}/${PACKAGE_VERSION}
      
      CLEAN="make clean"
      MAKE="'make' BUILD_KERNEL=$kernelver noisy"
      
      BUILT_MODULE_NAME[0]="i40e"
      DEST_MODULE_LOCATION[0]="/updates/dkms"
      
      AUTOINSTALL="yes"
      ```

   2. 内核模块添加到 DKMS 树中，查看状态，内核默认i40e驱动信息

      ```
       ⚡ root@localhost  /usr/src   dkms add -m i40e -v 2.25.9      
      Creating symlink /var/lib/dkms/i40e/2.25.9/source -> /usr/src/i40e-2.25.9
       ⚡ root@localhost  /usr/src  dkms status
      dev_ioctl_test/0.1: added
      i40e/2.25.9: added
      megaraid_sas/07.729.00.00, 4.18.0-425.19.2.el8_7.x86_64, x86_64: installed (original_module exists)
      megaraid_sas/07.729.00.00, 4.18.0-425.3.1.el8.x86_64, x86_64: installed (original_module exists)
       ⚡ root@localhost  /usr/src  modinfo i40e
      filename:       /lib/modules/4.18.0-425.3.1.el8.x86_64/kernel/drivers/net/ethernet/intel/i40e/i40e.ko.xz
      version:        4.18.0-425.3.1.el8.x86_64
      license:        GPL v2
      description:    Intel(R) Ethernet Connection XL710 Network Driver
      author:         Intel Corporation, <e1000-devel@lists.sourceforge.net>
      rhelversion:    8.7
      srcversion:     31A438ACFB9B2CE2EBFCDA7
      ```

   3. 使用 DKMS 构建内核模块

      ```
       ⚡ root@localhost  /usr/src  dkms build -m i40e -v 2.25.9
      Sign command: /lib/modules/4.18.0-425.3.1.el8.x86_64/build/scripts/sign-file
      Signing key: /var/lib/dkms/mok.key
      Public certificate (MOK): /var/lib/dkms/mok.pub
      
      Building module:
      Cleaning build area......
      'make' BUILD_KERNEL=4.18.0-425.3.1.el8.x86_64 noisy..........................
      Signing module /var/lib/dkms/i40e/2.25.9/build/i40e.ko
      Cleaning build area......
      ```

   4. 使用DKMS安装内核模块，这里需要加--force

      ```
       ⚡ root@localhost  /usr/src  dkms install -m i40e -v 2.25.9 --force
      -rw-r--r--. 1 root root 194780 Apr  6  2023 ./4.18.0-425.19.2.el8_7.x86_64/kernel/drivers/net/ethernet/intel/i40e/i40e.ko.xz
      lrwxrwxrwx  1 root root     55 Jul  2 14:42 ./4.18.0-425.19.2.el8_7.x86_64/weak-updates/i40e.ko.xz -> /lib/modules/4.18.0-425.3.1.el8.x86_64/extra/i40e.ko.xz
      -rw-r--r--  1 root root 238132 Jul  2 14:41 ./4.18.0-425.3.1.el8.x86_64/extra/i40e.ko.xz
      ```

   5. 重启，测试内核4.18.0-425.19.2.el8_7.x86_64，重启后发现dkms的i40e并没有安装

      ```
       ⚡ root@localhost  ~  dkms status 
      dev_ioctl_test/0.1: added
      i40e/2.25.9, 4.18.0-425.3.1.el8.x86_64, x86_64: installed (original_module exists)
      megaraid_sas/07.729.00.00, 4.18.0-425.19.2.el8_7.x86_64, x86_64: installed (original_module exists)
      megaraid_sas/07.729.00.00, 4.18.0-425.3.1.el8.x86_64, x86_64: installed (original_module exists)
      ```

      执行dkms install -m i40e -v 2.25.9 --force，其会进行编译，安装

      ```
       ⚡ root@localhost  ~                                         
      dkms install -m i40e -v 2.25.9 --force
      Sign command: /lib/modules/4.18.0-425.19.2.el8_7.x86_64/build/scripts/sign-file
      Signing key: /var/lib/dkms/mok.key
      Public certificate (MOK): /var/lib/dkms/mok.pub
      
      Building module:
      Cleaning build area......
      'make' BUILD_KERNEL=4.18.0-425.19.2.el8_7.x86_64 noisy........................
      Signing module /var/lib/dkms/i40e/2.25.9/build/i40e.ko
      Cleaning build area......
      
      i40e.ko.xz:
      Running module version sanity check.
       - Original module
         - Found /lib/modules/4.18.0-425.19.2.el8_7.x86_64/kernel/drivers/net/ethernet/intel/i40e/i40e.ko.xz
         - Storing in /var/lib/dkms/i40e/original_module/4.18.0-425.19.2.el8_7.x86_64/x86_64/
         - Archiving for uninstallation purposes
       - Installation
         - Installing to /lib/modules/4.18.0-425.19.2.el8_7.x86_64/extra/
      Adding any weak-modules
      depmod: ERROR: fstatat(6, kvdo.ko): No such file or directory
      depmod: ERROR: fstatat(6, uds.ko): No such file or directory
      depmod: WARNING: /lib/modules/4.18.0-425.19.2.el8_7.x86_64/extra/calmwu_modules/interrupt_test/interrupt_test.ko needs unknown symbol vector_irq
      depmod....
       ⚡ root@localhost  ~  modinfo i40e 
      filename:       /lib/modules/4.18.0-425.19.2.el8_7.x86_64/extra/i40e.ko.xz
      version:        2.25.9
      license:        GPL
      description:    Intel(R) 40-10 Gigabit Ethernet Connection Network Driver
      author:         Intel Corporation, <e1000-devel@lists.sourceforge.net>
      ```

      查看状态，都已安装，原生的驱动模块也都保存

      ```
       ⚡ root@localhost  ~  dkms status
      dev_ioctl_test/0.1: added
      i40e/2.25.9, 4.18.0-425.19.2.el8_7.x86_64, x86_64: installed (original_module exists)
      i40e/2.25.9, 4.18.0-425.3.1.el8.x86_64, x86_64: installed (original_module exists)
      megaraid_sas/07.729.00.00, 4.18.0-425.19.2.el8_7.x86_64, x86_64: installed (original_module exists)
      megaraid_sas/07.729.00.00, 4.18.0-425.3.1.el8.x86_64, x86_64: installed (original_module exists)
      ```

10. 资料

   [使用 DKMS 添加内核模块 — Documentation for Clear Linux* project](https://www.clearlinux.org/clear-linux-documentation/zh_CN/guides/kernel/kernel-modules-dkms.html#build-install-and-load-an-out-of-tree-module)

   [DKMS的使用详解-CSDN博客](https://blog.csdn.net/ldswfun/article/details/131554905)

   [DKMS简介 - wwang - 博客园 (cnblogs.com)](https://www.cnblogs.com/wwang/archive/2011/06/21/2085571.html)

   [Is DKMS provided in Red Hat Enterprise Linux? - Red Hat Customer Portal](https://access.redhat.com/solutions/1132653)

   [dkms 101 (terenceli.github.io)](https://terenceli.github.io/技术/2018/07/14/dkms-101)

   

   

