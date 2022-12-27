AUX_SOURCE_DIRECTORY(. trace_syscalls_cli_srcs)

include_directories("${PROJECT_SOURCE_DIR}/plugin_ebpf/skel"
                    "${EXTRA_DIR}/include/libbpf/libbpf/include"
                    "${EXTRA_DIR}/include/libbpf/libbpf/include/uapi")

add_executable(trace_syscalls_cli ${trace_syscalls_cli_srcs})
#target_compile_definitions(bootstrap_cli PRIVATE -DDEBUG)

target_link_libraries(trace_syscalls_cli xmonitor-utils libzlog.a libbpf.a elf pthread z m)