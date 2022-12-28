AUX_SOURCE_DIRECTORY(. deny_ptrace_cli_srcs)

include_directories("${PROJECT_SOURCE_DIR}/plugin_ebpf/skel"
                    "${PROJECT_SOURCE_DIR}/plugin_ebpf")

add_executable(deny_ptrace ${deny_ptrace_cli_srcs})
#target_compile_definitions(bootstrap_cli PRIVATE -DDEBUG)

target_link_libraries(deny_ptrace xmonitor-utils libzlog.a libbpf.a libcollectc.a elf pthread z m)