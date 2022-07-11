/*
 * @Author: CALM.WU
 * @Date: 2022-07-11 10:15:10
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-07-11 10:15:35
 */

#pragma once

#include <stdint.h>

extern void md5_calc(const uint8_t *src, uint32_t len, uint8_t *enc);