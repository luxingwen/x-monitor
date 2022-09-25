AUX_SOURCE_DIRECTORY(. readbtf_cli_srcs)

include_directories("${PROJECT_SOURCE_DIR}//plugin_ebpf/skel"
                    "${EXTRA_DIR}/include/libbpf/libbpf/include"
                    "${EXTRA_DIR}/include/libbpf/libbpf/include/uapi")

add_executable(readbtf_cli ${readbtf_cli_srcs})
target_compile_definitions(readbtf_cli PRIVATE -DDEBUG)

target_link_libraries(readbtf_cli xmonitor-utils libzlog.a libbpf.a elf pthread z m)