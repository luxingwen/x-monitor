/*
 * @Author: CALM.WU
 * @Date: 2021-10-15 16:55:32
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2021-10-15 17:24:02
 */

#pragma once

#include "common.h"

//
extern int32_t read_file(const char *file_name, char *buffer, size_t buffer_size);
//
extern int32_t read_file_to_uint64(const char *file_name, uint64_t *number);
//
extern int32_t read_file_to_int64(const char *file_name, int64_t *number);
//
extern int32_t write_int64_to_file(const char *file_name, int64_t number);

static __always_inline int32_t file_exists(const char *file_path) {
    return access(file_path, F_OK) == 0;
}