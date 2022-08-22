/*
 * @Author: CALM.WU
 * @Date: 2022-08-22 11:28:44
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-08-22 15:29:49
 */

// http://man7.org/linux/man-pages/man7/cgroups.7.html
// https://segmentfault.com/a/1190000008323952?utm_source=sf-similar-article

#include "cgroups_metrics.h"

const char *__sys_cgroup_metric_cpu_shares_help =
    "The relative share of CPU that this cgroup gets. This number is divided into the sum total of "
    "all cpu share values to determine the share any individual cgroup is entitled to.";
