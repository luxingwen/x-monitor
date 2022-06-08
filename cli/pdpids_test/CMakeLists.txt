AUX_SOURCE_DIRECTORY(. pdpids_test_srcs)
add_executable(pdpids_test ${pdpids_test_srcs})
target_link_libraries(pdpids_test xmonitor-utils libzlog.a pthread z m)