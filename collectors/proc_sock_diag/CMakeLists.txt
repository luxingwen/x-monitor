# 目录下所有的源代码文件并将列表存储在XMONITOR_PROC_SOCK_DIAG_SRCS中
AUX_SOURCE_DIRECTORY(. XMONITOR_PROC_SOCK_DIAG_SRCS)


#设置静态库输出目录                   
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/libs)
add_library(xmonitor-proc-sock-diag ${XMONITOR_PROC_SOCK_DIAG_SRCS})