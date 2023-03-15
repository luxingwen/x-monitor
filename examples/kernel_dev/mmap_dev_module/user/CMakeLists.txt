AUX_SOURCE_DIRECTORY(. mmap_dev_srcs)
add_executable(mmap_dev_test ${mmap_dev_srcs})
target_link_libraries(mmap_dev_test xmonitor-utils libzlog.a pthread z m)