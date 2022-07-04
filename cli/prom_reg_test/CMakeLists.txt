AUX_SOURCE_DIRECTORY(. prom_reg_test_srcs)
add_executable(prom_reg_test ${prom_reg_test_srcs})
target_link_libraries(prom_reg_test xmonitor-utils ${EXTRA_LIBS} pthread dl zstd z m)