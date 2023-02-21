AUX_SOURCE_DIRECTORY(. cachestat_top_cli_srcs)

include_directories("${PROJECT_SOURCE_DIR}/plugin_ebpf/skel"
                    "${PROJECT_SOURCE_DIR}/plugin_ebpf")

add_executable(cachestat_top_cli ${cachestat_top_cli_srcs})
target_compile_definitions(cachestat_top_cli PRIVATE -DDEBUG)

target_link_libraries(cachestat_top_cli xmonitor-utils libzlog.a libbpf.a elf pthread z m)