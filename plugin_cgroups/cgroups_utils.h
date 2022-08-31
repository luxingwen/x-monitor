/*
 * @Author: calmwu
 * @Date: 2022-08-31 10:51:27
 * @Last Modified by: calmwu
 * @Last Modified time: 2022-08-31 14:38:05
 */

#pragma once

enum cgroups_type {
    CGROUPS_V1,
    CGROUPS_V2,
    CGROUPS_FAILED,
};

extern enum cgroups_type cg_type;

extern enum cgroups_type get_cgroups_type();