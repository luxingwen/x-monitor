/*
 * @Author: CALM.WU
 * @Date: 2022-04-14 11:22:09
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-04-14 17:12:18
 */

#pragma once

#include "utils/common.h"
#include "utils/consts.h"
#include "utils/list.h"

enum app_bind_process_type { APP_BIND_PROCESSTREE = 0, APP_BIND_PROCESS };

struct app_process_filter_rule {
    struct list_head l_member;

    int32_t enable;
    char app_type_name[XM_APP_NAME_SIZE];
    char app_name[XM_APP_NAME_SIZE];
    enum app_bind_process_type assign_type;
    char **keys; // 多个匹配 key
    uint16_t key_count; // 匹配 key 的个数
    uint8_t is_matched; // 是否匹配过
};

struct app_filter_rules {
    struct list_head rule_list_head;
    int16_t rule_count;
};

// 通过配置文件生成 app 实例的过滤规则，返回规则的个数
extern struct app_filter_rules *create_filter_rules(const char *config_path);

extern void clean_filter_rules(struct app_filter_rules *rules);

extern void free_filter_rules(struct app_filter_rules *rules);
