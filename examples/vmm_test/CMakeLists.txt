AUX_SOURCE_DIRECTORY(. vmm_test_srcs)
add_executable(vmm_test ${vmm_test_srcs})

target_compile_definitions(vmm_test PRIVATE -DDEBUG)

target_link_libraries(vmm_test xmonitor-utils libzlog.a pthread z m)