/*
 * @Author: CALM.WU
 * @Date: 2021-10-18 11:47:28
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-04-14 17:26:31
 */

#pragma once

#include <stdint.h>
#include "libconfig/libconfig.h"

extern int32_t appconfig_load(const char *config_file);
extern void    appconfig_destroy();
extern void    appconfig_reload();

// extern void appconfig_rdlock();
// extern void appconfig_wrlock();
// extern void appconfig_unlock();

// ---------------------------------------------------------
extern const char *appconfig_get_str(const char *key, const char *def);
extern int32_t     appconfig_get_bool(const char *key, int32_t def);
extern int32_t     appconfig_get_int(const char *key, int32_t def);

extern const char *appconfig_get_member_str(const char *path, const char *key, const char *def);
extern int32_t     appconfig_get_member_bool(const char *path, const char *key, int32_t def);
extern int32_t     appconfig_get_member_int(const char *path, const char *key, int32_t def);

config_setting_t *appconfig_lookup(const char *path);