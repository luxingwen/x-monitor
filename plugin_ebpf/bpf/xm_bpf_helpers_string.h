/*
 * @Author: CALM.WU
 * @Date: 2024-02-27 10:31:50
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-03-04 16:02:04
 */

#pragma once

#include <bpf/bpf_helpers.h>

/*
Summary: This code defines a function __xm_strncmp that compares two strings up
to a specified length and returns 1 if they are not equal or if either string is
null-terminated, otherwise returns 0.

__xm_strncmp: Function to compare two strings up to a specified length.

Parameters:
- cs: Pointer to the first string to compare
- ct: Pointer to the second string to compare
- len: Maximum number of characters to compare

Returns:
- 1 if the strings are not equal or if either string is null-terminated, 0
otherwise
*/
static __always_inline __s32 __xm_strncmp(const unsigned char *cs,
                                          const unsigned char *ct, size_t len) {
    unsigned char c1, c2;
    size_t i;

    for (i = 0; i < len; i++) {
        c1 = (unsigned char)cs[i];
        c2 = (unsigned char)ct[i];

        if (c1 != c2 || c1 == '\0' || c2 == '\0') {
            return 1;
        }
    }

    return 0;
}

/*
Summary: This file contains a C function __xm_strlen that calculates the length
of a string up to a maximum length, similar to the standard strlen function but
with a specified maximum length.

__xm_strlen: Function to calculate the length of a string up to a maximum
length.

Parameters:
- s: Pointer to the string
- max_len: Maximum length to consider while calculating the length

Returns:
- Length of the string up to a maximum of max_len characters
*/
static __always_inline size_t __xm_strlen(const unsigned char *s,
                                          size_t max_len) {
    size_t len = 0;
    while (len < max_len && s[len] != '\0') {
        len++;
    }
    return len;
}