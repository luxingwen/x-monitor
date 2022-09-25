AUX_SOURCE_DIRECTORY(. libmill_signal_test_srcs)

add_executable(libmill_signal_test ${libmill_signal_test_srcs})

# target_compile_definitions(bootstrap_cli PRIVATE -DDEBUG)
target_link_libraries(libmill_signal_test xmonitor-utils libmill.a libzlog.a pthread z m)