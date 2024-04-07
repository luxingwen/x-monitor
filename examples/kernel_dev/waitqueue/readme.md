1. 编译动态初始化waitqueue

   ```
   make CFLAGS_EXTRA=-DINIT_BY_DNY noisy
   ```

2. 编译静态初始化

   ```
   make noisy
   ```

3. 拷贝到安装路径

   ```
   make install
   ```

4. 安装模块

   ```
   modprobe waitqueue_test
   ```

   dmesg查看

   ```
   Sun Apr  7 14:48:36 2024] waitqueue_test: loading out-of-tree module taints kernel.
   [Sun Apr  7 14:48:36 2024] waitqueue_test: module verification failed: signature and/or required key missing - tainting kernel
   [Sun Apr  7 14:48:36 2024] cw_waitqueue_test_init
   [Sun Apr  7 14:48:36 2024] major: 241, minor: 0
   [Sun Apr  7 14:48:36 2024] use static waitqueue initialization
   [Sun Apr  7 14:48:36 2024] cw_wait_thread: 18232 created
   [Sun Apr  7 14:48:36 2024] waitqueue test device '/dev/cw_waitqueue_test' created
   [Sun Apr  7 14:48:36 2024] cw_wait_thread:18232, Waiting for Event...
   ```

5. 查看设备

   ```
   cat /dev/cw_waitqueue_test
   ```

   dmesg查看输出

   ```
   [Sun Apr  7 14:48:36 2024] cw_waitqueue_test_init
   [Sun Apr  7 14:48:36 2024] major: 241, minor: 0
   [Sun Apr  7 14:48:36 2024] use static waitqueue initialization
   [Sun Apr  7 14:48:36 2024] cw_wait_thread: 18232 created
   [Sun Apr  7 14:48:36 2024] waitqueue test device '/dev/cw_waitqueue_test' created
   [Sun Apr  7 14:48:36 2024] cw_wait_thread:18232, Waiting for Event...
   [Sun Apr  7 14:50:18 2024] Device '/dev/cw_waitqueue_test' Opened...!!!
   [Sun Apr  7 14:50:18 2024] Read from Device '/dev/cw_waitqueue_test'
   [Sun Apr  7 14:50:18 2024] Device '/dev/cw_waitqueue_test' Closed...!!!
   [Sun Apr  7 14:50:18 2024] cw_wait_thread:18232, Event came from Read function - 1
   [Sun Apr  7 14:50:18 2024] cw_wait_thread:18232, Waiting for Event...
   [Sun Apr  7 14:52:26 2024] Device '/dev/cw_waitqueue_test' Opened...!!!
   [Sun Apr  7 14:52:26 2024] Read from Device '/dev/cw_waitqueue_test'
   [Sun Apr  7 14:52:26 2024] Device '/dev/cw_waitqueue_test' Closed...!!!
   [Sun Apr  7 14:52:26 2024] cw_wait_thread:18232, Event came from Read function - 2
   [Sun Apr  7 14:52:26 2024] cw_wait_thread:18232, Waiting for Event...
   ```

6. 卸载模块，查看内核线程的退出

   ```
   rmmod waitqueue_test
   ```

   dmesg查看

   ```
   [Sun Apr  7 14:54:20 2024] Event came from Exit function
   [Sun Apr  7 14:54:20 2024] cw_wait_thread:18232 Stopping...
   [Sun Apr  7 14:54:20 2024] cw_wait_thread:18232 stopped
   [Sun Apr  7 14:54:20 2024] waitqueue test device '/dev/cw_waitqueue_test' removed
   ```

   





