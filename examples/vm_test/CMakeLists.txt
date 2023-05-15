AUX_SOURCE_DIRECTORY(. xm_vm_test_srcs)
add_executable(xm_vm_test ${xm_vm_test_srcs})

target_compile_definitions(xm_vm_test PRIVATE -DDEBUG)

target_link_libraries(xm_vm_test xmonitor-utils libzlog.a pthread z m)