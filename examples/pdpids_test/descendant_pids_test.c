/*
 * @Author: CALM.WU
 * @Date: 2021-12-23 11:43:29
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2021-12-23 17:13:37
 */

#include "utils/common.h"
#include "utils/log.h"
#include "utils/strings.h"
#include "utils/compiler.h"
#include "utils/os.h"

#define NAME_MAX_LEN 64
#define PERSON_MAX_COUNT 8

struct person {
    char name[NAME_MAX_LEN];
    int32_t age;
};

struct person_ary {
    uint32_t size;
    struct person persons[];
    // unsigned char persons[] __attribute__((aligned(__alignof__(struct
    // person))));
};

static void __test_person_ary() {
    struct person_ary *ary = NULL;

    ary = calloc(1, sizeof(struct person_ary)
                        + sizeof(struct person) * PERSON_MAX_COUNT);
    ary->size = PERSON_MAX_COUNT;

    for (uint32_t i = 0; i < PERSON_MAX_COUNT; i++) {
        snprintf(ary->persons[i].name, NAME_MAX_LEN, "person_%d", i);
        ary->persons[i].age = i;
    }

    for (uint32_t i = 0; i < PERSON_MAX_COUNT; i++) {
        debug("name: %s, age: %d", ary->persons[i].name, ary->persons[i].age);
    }

    free(ary);
}

int32_t main(int32_t argc, char **argv) {
    if (log_init("../examples/log.cfg", "descendant_pids_test") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    if (unlikely(argc != 2)) {
        fatal("./descendant_pids_test <parent-pid>\n");
    }

    pid_t parent_pid = str2int32_t(argv[1]);

    debug("get process pid '%d' descendant pids", parent_pid);

    //__test_person_ary();

    struct process_descendant_pids pd_pids = { NULL, 0 };
    get_process_descendant_pids(parent_pid, &pd_pids);

    for (uint32_t i = 0; i < pd_pids.pids_size; i++) {
        debug("descendant pid: %d", pd_pids.pids[i]);
    }

    free(pd_pids.pids);

    log_fini();

    return 0;
}