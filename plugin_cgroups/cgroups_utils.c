/*
 * @Author: calmwu
 * @Date: 2022-08-31 10:52:08
 * @Last Modified by: calmwu
 * @Last Modified time: 2022-08-31 20:57:38
 */

#include "cgroups_utils.h"

#include "utils/compiler.h"
#include "utils/consts.h"
#include "utils/popen.h"
#include "utils/log.h"
#include "utils/files.h"

enum cgroups_type cg_type = CGROUPS_FAILED;

static const char *systemctl_cmd = "systemctl --version";

enum cgroups_type get_cgroups_type() {
    pid_t   command_pid;
    FILE   *fp = NULL;
    char    buff[XM_PROC_LINE_SIZE] = { 0 };
    int32_t cgroupv2_available = 0;

    // 检查系统是否安装了cgroup
    fp = mypopen("grep cgroup /proc/filesystems", &command_pid);
    if (unlikely(NULL == fp)) {
        error("[PLUGIN_CGROUPS]  popen grep cgroup /proc/filesystems failed.");
        return CGROUPS_FAILED;
    }

    // 一行一行读取
    while (fgets(buff, XM_PROC_LINE_SIZE, fp) != NULL) {
        if (strstr(buff, "cgroup2")) {
            cgroupv2_available = 1;
            break;
        }
    }

    if (unlikely(mypclose(fp, command_pid) != 0)) {
        error("[PLUGIN_CGROUPS]  pclose grep cgroup /proc/filesystems failed.");
        return CGROUPS_FAILED;
    }

    if (0 == cgroupv2_available) {
        cg_type = CGROUPS_V1;
        return cg_type;
    }

    // 判断systemctl --version返回内容
    // default-hierarchy=legacy v1
    // default-hierarchy=hybrid v1
    // default-hierarchy=unified v2
    // NULL 没有配置cgroup
    fp = mypopen(systemctl_cmd, &command_pid);
    if (unlikely(NULL == fp)) {
        error("[PLUGIN_CGROUPS]  popen systemctl --version failed.");
        return CGROUPS_FAILED;
    }

    while (fgets(buff, XM_PROC_LINE_SIZE, fp) != NULL) {
        if (strstr(buff, "default-hierarchy=legacy") || strstr(buff, "default-hierarchy=hybrid")) {
            debug(
                "[PLUGIN_CGROUPS] systemctl --version output contains default-hierarchy=legacy or "
                "hybrid.");
            cg_type = CGROUPS_V1;
            break;
        } else if (strstr(buff, "default-hierarchy=unified")) {
            debug(
                "[PLUGIN_CGROUPS] systemctl --version unified contains default-hierarchy=unified.");
            cg_type = CGROUPS_V2;
            break;
        }
    }

    if (unlikely(mypclose(fp, command_pid) != 0)) {
        error("[PLUGIN_CGROUPS]  pclose systemctl --version failed.");
        cg_type = CGROUPS_FAILED;
        return cg_type;
    }

    if (cg_type == CGROUPS_V1) {
        return CGROUPS_V1;
    }

    // 检查/proc/cmdline是否有systemd.unified_cgroup_hierarchy=0 则是v1
    if (likely(read_file("/proc/cmdline", buff, XM_PROC_LINE_SIZE) > 0)) {
        if (strstr(buff, "systemd.unified_cgroup_hierarchy=0")) {
            debug("[PLUGIN_CGROUPS]  /proc/cmdline has systemd.unified_cgroup_hierarchy=0");
            cg_type = CGROUPS_V1;
        } else if (strstr(buff, "systemd.unified_cgroup_hierarchy=1")) {
            debug("[PLUGIN_CGROUPS]  /proc/cmdline has systemd.unified_cgroup_hierarchy=1");
            cg_type = CGROUPS_V2;
        }
    }

    return cg_type;
}