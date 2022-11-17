# 目录下所有的源代码文件并将列表存储在XMONITOR_PROC_SOCK_SRCS中
file(GLOB_RECURSE XMONITOR_PROC_SOCK_SRCS
    "*.c"
    "*.h"
    "internal/*.c"
    "internal/*.h"
)


#设置静态库输出目录                   
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/libs)
add_library(xmonitor-proc-sock ${XMONITOR_PROC_SOCK_SRCS})