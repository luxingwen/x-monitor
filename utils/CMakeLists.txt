# 目录下所有的源代码文件并将列表存储在XMONITOR_UTILS_SRCS中
# AUX_SOURCE_DIRECTORY(. XMONITOR_UTILS_SRCS)
file(GLOB_RECURSE X-MONITOR_UTILS_SRCS
    "*.c"
    "*.h"
    "json/*.c"
    "json/*.h"
    "sds/*.c"
    "sds/*.h"
    "hash/*.c"
    "hash/*.h"
    "ringbuf/*.c"
    "ringbuf/*.h"
)

if("${CMAKE_HOST_SYSTEM_VERSION}" MATCHES "3.10.*.el7.x86_64")
    list(REMOVE_ITEM X-MONITOR_UTILS_SRCS "./x_ebpf.c")
endif()

# 会追加到父CMakeLists.txt的include变量中
include_directories("${EXTRA_DIR}/include/libbpf/libbpf/include/uapi"
    "${EXTRA_DIR}/include/libbpf/libbpf/include")

add_definitions(-D_GNU_SOURCE)

# 设置静态库输出目录
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/libs)
add_library(xmonitor-utils ${X-MONITOR_UTILS_SRCS})