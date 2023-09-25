AUX_SOURCE_DIRECTORY(. stack_unwind_cli_srcs)

add_executable(stack_unwind_cli ${stack_unwind_cli_srcs})
target_compile_definitions(stack_unwind_cli PRIVATE -DDEBUG)

target_link_libraries(stack_unwind_cli xmonitor-utils libzlog.a elf pthread z m)