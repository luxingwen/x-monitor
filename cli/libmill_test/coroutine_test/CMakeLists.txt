AUX_SOURCE_DIRECTORY(. libmill_coroutine_test_srcs)

add_executable(libmill_coroutine_test ${libmill_coroutine_test_srcs})

# target_compile_definitions(bootstrap_cli PRIVATE -DDEBUG)
target_link_libraries(libmill_coroutine_test xmonitor-utils libdill.a libzlog.a pthread z m)