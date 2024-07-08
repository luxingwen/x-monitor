AUX_SOURCE_DIRECTORY(. ioctl_userspace_srcs)
add_executable(ioctl_test ${ioctl_userspace_srcs})
target_link_libraries(ioctl_test xmonitor-utils libzlog.a pthread z m)