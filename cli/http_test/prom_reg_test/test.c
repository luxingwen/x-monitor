/*
 * @Author: CALM.WU
 * @Date: 2022-07-04 11:47:47
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-07-07 15:16:03
 */

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/os.h"
#include "utils/consts.h"
#include "utils/strings.h"
#include "utils/backoff_algorithm.h"
#include "utils/clocks.h"

#include "curl/curl.h"

// https://github.com/FreeRTOS/backoffAlgorithm

enum curl_action { CURL_GET = 0, CURL_POST = 1 };

static const uint16_t __retry_backoff_base_ms = 1000;
static const uint16_t __retry_max_backoff_delay_ms = 5000;
static const uint32_t __retry_max_attempts = 3;

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

static __always_inline int32_t __check_dohttp_result(CURLcode code, long res) {
    if (code != CURLE_OK || res != 200) {
        error("http do failed, res:%d, error: '%s', response_code:%ld", code,
              curl_easy_strerror(code), res);
        return -1;
    }
    return 0;
}

int32_t main(int32_t argc, char **argv) {
    int32_t                   opt = 0;
    CURL                     *curl;
    CURLcode                  curl_code;
    long                      http_res_code = 0;
    int32_t                   do_http_ret = 0;
    struct curl_slist        *headers = NULL;
    char                     *url = NULL;
    enum curl_action          action = CURL_GET;
    bool                      verbose = false;
    BackoffAlgorithmStatus_t  retryStatus = BackoffAlgorithmSuccess;
    BackoffAlgorithmContext_t retryContext;
    uint16_t                  nextRetryBackoff = 0;

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
                error("invalid action");
                return -1;
            }
            break;
        case 'v':
            verbose = true;
            break;
        default:
            error("Unrecognized option '%c'", opt);
            usage(basename(argv[0]));
            return EXIT_FAILURE;
        }
    }

    BackoffAlgorithm_InitializeParams(&retryContext, __retry_backoff_base_ms,
                                      __retry_max_backoff_delay_ms, __retry_max_attempts);

    srand(now_realtime_sec());

    curl_global_init(CURL_GLOBAL_ALL);

    curl = curl_easy_init();
    if (likely(curl)) {
        curl_easy_setopt(curl, CURLOPT_URL, url);

        // 设置http header内容
        headers = curl_slist_append(headers, "Content-Type: charset=UTF-8");

        // 设置连接超时，单位为秒
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2);
        // 读写数据超时，单位为秒
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);

        /* Now specify we want to POST data */
        if (action == CURL_POST) {
            curl_easy_setopt(curl, CURLOPT_POST, 1);

            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, "Accept: application/json");
            // 构造json数据
            // 设置post数据

            // curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
            // "{\"name\":\"test\",\"password\":\"123456\"}");
        } else {
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
        }

        if (verbose) {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        }

        // 设置服务器返回回调函数
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, __write_response);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, __write_header_tag);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __write_response);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, __write_data_tag);

        do {
            // 执行请求
            curl_code = curl_easy_perform(curl);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_res_code);

            do_http_ret = __check_dohttp_result(curl_code, http_res_code);
            if (unlikely(0 != do_http_ret)) {

                // 退避重试
                retryStatus =
                    BackoffAlgorithm_GetNextBackoff(&retryContext, rand(), &nextRetryBackoff);

                debug("retryStatus: %d, nextRetryBackoff: %d", retryStatus, nextRetryBackoff);

                if (retryStatus == BackoffAlgorithmSuccess) {
                    usleep(nextRetryBackoff * 1000);
                }
            }

        } while ((0 != do_http_ret) && retryStatus != BackoffAlgorithmRetriesExhausted);

        /* always cleanup */
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    if (likely(url)) {
        free(url);
        url = NULL;
    }
    debug("prom_reg_test done");

    log_fini();
    return 0;
}