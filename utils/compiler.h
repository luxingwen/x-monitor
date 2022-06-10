/*
 * @Author: CALM.WU
 * @Date: 2021-10-14 16:33:29
 * @Last Modified by: calmwu
 * @Last Modified time: 2022-05-04 10:39:49
 */

#pragma once

#if __GNUC__
#if __x86_64__ || __ppc64__
#define LONG_BITS 64
#else
#define LONG_BITS 64
#endif
#endif

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// linux kernel的ARRAY_SIZE
// macro為了預防傳入的變數是pointer而不是真正的array，背後其實花了很多心力來防範這件事，使得如果發生問題就會在編譯時期就失敗，避免了因為誤用而在run-time才發生問題的狀況。
// /* Are two types/vars the same type (ignoring qualifiers)? */
#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

/*
https://hackmd.io/@sysprog/c-bitfield

這個 macro 是用來檢查是否會有 compilation error，e 是個判斷式，當它經由這個 macro 結果為 true
，則可以在 compile-time 的時候被告知有錯
!!(e)：對 e 做兩次 NOT，確保結果一定是 0 或 1
-!!(e)：對 !!(e) 乘上 -1，因此結果會是 0 或 -1
*/
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int : -!!(e); }))

#define __must_be_array(a) BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0]))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))

// 定义纯函数
#ifndef __pure
#define __pure __attribute__((pure))
#endif

// 强制内嵌函数
#ifndef __always_inline
#define __always_inline inline __attribute__((always_inline))
#endif

#ifdef __GNUC__
#define UNUSED(x) UNUSED_##x __attribute__((__unused__))
#else
#define UNUSED(x) UNUSED_##x
#endif

#ifdef __GNUC__
#define UNUSED_FUNCTION(x) __attribute__((__unused__)) UNUSED_##x
#else
#define UNUSED_FUNCTION(x) UNUSED_##x
#endif

#ifndef __concat
#define __concat(a, b) a##b
#endif

#if !defined ROUNDUP
#define ROUNDUP(X, Y) (((X) + ((Y)-1)) & ~((Y)-1))
#endif

#ifdef __compiler_offsetof
#define offsetof(TYPE, MEMBER) __compiler_offsetof(TYPE, MEMBER)
#else
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)
#endif
#endif

#ifndef container_of
/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member)                    \
    ({                                                     \
        const typeof(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member)); \
    })
#endif

/**
 * sizeof_field(TYPE, MEMBER)
 *
 * @TYPE: The structure containing the field of interest
 * @MEMBER: The field to return the size of
 */
#define sizeof_field(TYPE, MEMBER) sizeof((((TYPE *)0)->MEMBER))

/**
 * offsetofend(TYPE, MEMBER)
 *
 * @TYPE: The type of the structure
 * @MEMBER: The member within the structure to get the end offset of
 */
#define offsetofend(TYPE, MEMBER) (offsetof(TYPE, MEMBER) + sizeof_field(TYPE, MEMBER))

// 支持宏嵌套
#define TO_STRING_1(x) #x
#define TO_STRING(x) TO_STRING_1(x)