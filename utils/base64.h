/*
 * @Author: CALM.WU
 * @Date: 2022-07-11 10:24:31
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-07-11 10:26:58
 */

#pragma once

#include <stdint.h>

// 返回编码后长度
extern int32_t base64_encode(const char *src, int32_t src_len, char *dest, int32_t dest_len);

// 返回解码后的长度
extern int32_t base64_decode(const char *src, int32_t src_len, char *dest, int32_t dest_len);