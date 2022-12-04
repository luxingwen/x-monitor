AUX_SOURCE_DIRECTORY(. read_smaps_test_srcs)
add_executable(read_smaps_test ${read_smaps_test_srcs})
target_link_libraries(read_smaps_test xmonitor-utils libzlog.a pthread z m)