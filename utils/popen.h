/*
 * @Author: CALM.WU
 * @Date: 2021-10-12 11:42:23
 * @Last Modified by: calmwu
 * @Last Modified time: 2022-08-31 11:18:31
 */

#pragma once

#include "common.h"

// fork-exec
extern FILE *mypopen(const char *command, volatile pid_t *pidptr);
extern FILE *mypopene(const char *command, volatile pid_t *pidptr, char **env);
// 子进程退出清理
extern int32_t mypclose(FILE *fp, pid_t pid);