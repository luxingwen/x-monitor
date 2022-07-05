AUX_SOURCE_DIRECTORY(. http_client_test_srcs)
add_executable(http_client_test ${http_client_test_srcs})
target_link_libraries(http_client_test xmonitor-utils ${EXTRA_LIBS} pthread dl zstd z m)