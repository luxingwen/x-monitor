/*
 * @Author: CALM.WU
 * @Date: 2022-07-04 11:47:47
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-07-04 16:01:53
 */

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/os.h"
#include "utils/consts.h"
#include "utils/strings.h"

#include "curl/curl.h"

enum curl_action { CURL_GET = 0, CURL_POST = 1 };

static const struct option __opts[] = {
    { "url", required_argument, NULL, 'u' },
    { "action", optional_argument, NULL, 'a' },
    { "verbose", optional_argument, NULL, 'v' },
    { 0, 0, 0, 0 },
};

static const char *__optstr = "a:u:v";
static const char *__write_header_tag = "writer_header";
static const char *__write_data_tag = "writer_data";

static void usage(const char *prog) {
    fprintf(stderr,
            "usage: %s [OPTS]\n\n"
            "OPTS:\n"
            "    -a    action(get/post)\n"
            "    -u    url\n"
            "    -v    verbose\n",
            prog);
}

static size_t __write_response(void *ptr, size_t size, size_t nmemb, void *data) {
    const char *tag = (const char *)data;
    int32_t     bytes = size * nmemb;
    // *是指定的位宽
    debug("---%s--- %.*s", tag, bytes, (char *)ptr);
    return bytes;
}

int32_t main(int32_t argc, char **argv) {
    int32_t            opt = 0;
    CURL              *curl;
    CURLcode           res;
    struct curl_slist *headers = NULL;
    char              *url = NULL;
    enum curl_action   action = CURL_GET;
    bool               verbose = false;

    if (log_init("../cli/log.cfg", "prom_reg_test") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    while ((opt = getopt_long(argc, argv, __optstr, __opts, NULL)) != -1) {
        switch (opt) {
        case 'u':
            url = strndup(optarg, strlen(optarg));
            break;
        case 'a':
            if (strcmp(optarg, "get") == 0) {
                action = CURL_GET;
            } else if (strcmp(optarg, "post") == 0) {
                action = CURL_POST;
            } else {
                error("invalid action\n");
                return -1;
            }
            break;
        case 'v':
            verbose = true;
            break;
        default:
            error("Unrecognized option '%c'\n", opt);
            usage(basename(argv[0]));
            return EXIT_FAILURE;
        }
    }

    curl_global_init(CURL_GLOBAL_ALL);

    curl = curl_easy_init();
    if (likely(curl)) {
        curl_easy_setopt(curl, CURLOPT_URL, url);

        // 设置http header内容
        headers = curl_slist_append(headers, "Content-Type: charset=UTF-8");

        // 设置连接超时，单位为秒
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
        // 读写数据超时，单位为秒
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

        /* Now specify we want to POST data */
        if (action == CURL_POST) {
            curl_easy_setopt(curl, CURLOPT_POST, 1);

            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, "Accept: application/json");
            // 构造json数据
            // 设置post数据
        } else {
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
        }

        if (verbose) {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        }

        // 设置响应输出回调函数
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, __write_response);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, __write_header_tag);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __write_response);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, __write_data_tag);

        // 执行请求
        res = curl_easy_perform(curl);
        if (unlikely(res != CURLE_OK)) {
            error("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    if (likely(url)) {
        free(url);
        url = NULL;
    }

    log_fini();
    return 0;
}