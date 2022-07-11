/*
 * @Author: CALM.WU
 * @Date: 2022-07-11 10:27:22
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-07-11 10:39:57
 */

#include "base64.h"
#include <stddef.h>

#define B0(a) (a & 0xFF)
#define B1(a) (a >> 8 & 0xFF)
#define B2(a) (a >> 16 & 0xFF)
#define B3(a) (a >> 24 & 0xFF)

int32_t base64_encode(const char *src, int32_t src_len, char *dest, int32_t dest_len) {
    //参数检查
    if (NULL == src || NULL == dest)
        return -1;
    //是否以NULL结束的字符串
    if (src_len <= 0 || dest_len <= 0)
        return -1;
    //输出空间长度检查
    if (src_len % 3 == 0) {
        if (dest_len < src_len / 3 * 4 + 1)
            return -1;
    } else {
        if (dest_len < (src_len / 3 + 1) * 4 + 1)
            return -1;
    }
    //编码表
    const char Base64EncodeTable[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

    //目标编码长度
    int32_t result_destlen = 0;

    uint32_t num = 0;
    int32_t  len = src_len - 3;
    int32_t  index = 0;
    for (; index < len; index += 3) {
        num = *((uint32_t *)(src + index));
        *(dest + result_destlen++) = Base64EncodeTable[(B0(num) >> 2) & 0x3F];
        *(dest + result_destlen++) = Base64EncodeTable[((B0(num) << 4) | (B1(num) >> 4)) & 0x3F];
        *(dest + result_destlen++) = Base64EncodeTable[((B1(num) << 2) | (B2(num) >> 6)) & 0x3F];
        *(dest + result_destlen++) = Base64EncodeTable[B2(num) & 0x3F];
    }

    // 处理最后余下的不足3字节的数据
    int32_t rest = src_len - index;
    if (rest) {
        num = *((uint32_t *)(src + index));
        *(dest + result_destlen++) = Base64EncodeTable[(B0(num) >> 2) & 0x3F];
        *(dest + result_destlen++) = Base64EncodeTable[((B0(num) << 4) | (B1(num) >> 4)) & 0x3F];
        *(dest + result_destlen++) =
            Base64EncodeTable[rest > 1 ? (((B1(num) << 2) | (B2(num) >> 6)) & 0x3F) : 64];
        *(dest + result_destlen++) = Base64EncodeTable[rest > 2 ? (B2(num) & 0x3F) : 64];
    }
    return result_destlen;
}

int32_t base64_decode(const char *src, int32_t src_len, char *dest, int32_t dest_len) {
    //参数检查
    if (NULL == src || NULL == dest)
        return -1;
    if (src_len <= 0 || dest_len <= 0)
        return -1;

    //输出空间长度检查
    if (dest_len < src_len + src_len / 4 * 3 + 1)
        return -1;
    //编码表
    unsigned char Base64DecodeTable[] = {
        // 38个255
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255,
        // 255,255,255,255,255,'+', 255,255,255, '/',
        255, 255, 255, 255, 255, 62, 255, 255, 255, 63,
        //'0'...'9',
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
        // 255,255,255, '=', 255,255,255,
        255, 255, 255, 0, 255, 255, 255,
        //'A'...'Z',
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
        25, 255, 255, 255, 255, 255, 255,
        //'a'...'z',
        26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51
    };

    //目标编码长度
    int32_t result_destlen = 0;

    uint32_t num = 0;
    int32_t  len = src_len - 4;
    int32_t  index = 0;
    for (; index < len; index += 4) {
        num = *((uint32_t *)(src + index));
        *(dest + result_destlen++) =
            ((Base64DecodeTable[B0(num)] << 2) | (Base64DecodeTable[B1(num)] >> 4)) & 0xFF;
        *(dest + result_destlen++) =
            ((Base64DecodeTable[B1(num)] << 4) | (Base64DecodeTable[B2(num)] >> 2)) & 0xFF;
        *(dest + result_destlen++) =
            ((Base64DecodeTable[B2(num)] << 6) | Base64DecodeTable[B3(num)]) & 0xFF;
    }
    // 处理最后余下的不足4字节的数据
    int32_t rest = src_len - index;
    if (rest) {
        num = *((uint32_t *)(src + index));
        *(dest + result_destlen++) =
            ((Base64DecodeTable[B0(num)] << 2) | (Base64DecodeTable[B1(num)] >> 4)) & 0xFF;
        *(dest + result_destlen++) =
            ((Base64DecodeTable[B1(num)] << 4) | (Base64DecodeTable[B2(num)] >> 2)) & 0xFF;
        *(dest + result_destlen++) =
            ((Base64DecodeTable[B0(num)] << 6) | Base64DecodeTable[B3(num)]) & 0xFF;
    }
    *(dest + result_destlen++) = 0;
    return result_destlen;
}