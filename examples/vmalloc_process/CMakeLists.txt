AUX_SOURCE_DIRECTORY(. vmalloc_process_srcs)
add_executable(vmalloc_process ${vmalloc_process_srcs})

target_compile_definitions(vmalloc_process PRIVATE -DDEBUG)

target_link_libraries(vmalloc_process xmonitor-utils libzlog.a pthread z m)