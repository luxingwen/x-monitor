/*
 * @Author: CALM.WU
 * @Date: 2021-12-23 11:43:29
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2021-12-23 17:13:37
 */

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/simple_pattern.h"
#include "utils/strings.h"

void match(const char *pattern, const char *str) {
    SIMPLE_PATTERN *sp = simple_pattern_create(pattern, NULL, SIMPLE_PATTERN_EXACT);

    // simple_pattern_dump(sp);

    if (simple_pattern_matches(sp, str)) {
        debug("+++ '%s' matches with pattern: '%s'\n", str, pattern);
    } else {
        debug("---'%s' mismatches with pattern: '%s'\n", str, pattern);
    }

    simple_pattern_free(sp);
}

int32_t main(int32_t argc, char **argv) {
    if (log_init("../cli/log.cfg", "parttern_test") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    if (unlikely(argc != 3)) {
        fatal("./simplepattern_test pattern_str match_str\n");
        return -1;
    }

    // match("*foobar* !foo* !*bar *", "foo");
    // match("*foobar* foo !*bar *", "foo");
    // match("*foobar* foo !*bar *", "foobar");
    // match("!foobar foo !*bar *", "foobar");
    // match("!*foobar foo !*bar *", "csdfoobar");
    // match("*", "csdfoobar");
    // // mismatch
    // match("!c*", "csdfoobar");
    // // mismatch
    // match("!*c*", "sdsdcsdfoobar");
    // // match
    // match("/proc/* /sys/* /var/run/user/* /run/user/* /snap/* /var/lib/docker/*",
    //            "/sys/fs/cgroup/");

    match(argv[1], argv[2]);

    log_fini();

    return 0;
}

/*
~/Program/x-monitor/bin » ./simplepattern_test "\!*foo* sdsd" sdsd
22-08-31 23:53:01:466 [DEBUG] +++ 'sdsd' matches with pattern: '!*foo* sdsd'
*/