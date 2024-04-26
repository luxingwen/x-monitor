1. hung task检测设置

   ```
    ⚡ root@localhost  ~  sysctl -a --pattern hung
   kernel.hung_task_check_count = 4194304
   kernel.hung_task_panic = 0
   kernel.hung_task_timeout_secs = 120
   kernel.hung_task_warnings = 10
   ```

2. 编译

   ```
   make CFLAGS_EXTRA="-ggdb -DDEBUG" modules_install
   ```

3. 运行hung task，可见状态为D，信号9无法kill

   ```
    ⚡ root@localhost  ~  ps -aux|grep 114923
   root      114923  0.0  0.0      0     0 ?        D    16:09   0:00 [cw_hung_task_th]
   root      114948  0.0  0.0  12144  1152 pts/5    R+   16:09   0:00 grep --color=auto --exclude-dir=.bzr --exclude-dir=CVS --exclude-dir=.git --exclude-dir=.hg --exclude-dir=.svn --exclude-dir=.idea --exclude-dir=.tox 114923
    ⚡ root@localhost  ~  kill -9 114923
    ⚡ root@localhost  ~  ps -aux|grep 114923
   root      114923  0.0  0.0      0     0 ?        D    16:09   0:00 [cw_hung_task_th]
   root      114970  0.0  0.0  12144  1184 pts/5    R+   16:10   0:00 grep --color=auto --exclude-dir=.bzr --exclude-dir=CVS --exclude-dir=.git --exclude-dir=.hg --exclude-dir=.svn --exclude-dir=.idea --exclude-dir=.tox 114923
   ```

   