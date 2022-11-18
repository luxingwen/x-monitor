AUX_SOURCE_DIRECTORY(. proc_sock_test_srcs)
add_executable(proc_sock_test ${proc_sock_test_srcs})
target_link_libraries(proc_sock_test xmonitor-proc-sock xmonitor-utils libzlog.a liburcu-cds.a liburcu.a pthread z m)