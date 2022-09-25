AUX_SOURCE_DIRECTORY(. libmill_chan_srcs)

add_executable(libmill_chan_test ${libmill_chan_srcs})

# target_compile_definitions(bootstrap_cli PRIVATE -DDEBUG)
target_link_libraries(libmill_chan_test xmonitor-utils libmill.a libzlog.a pthread z m)