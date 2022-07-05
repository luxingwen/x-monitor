/*
 * @Author: CALM.WU
 * @Date: 2022-07-05 15:49:52
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-07-05 18:33:00
 */

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/os.h"
#include "utils/backoff_algorithm.h"
#include "utils/clocks.h"
#include "utils/http_client.h"

static const char *__urls[] = {
    "www.baidu.com",
    "www.sina.com.cn",
    "127.0.0.1:8000",
};

int32_t main(int32_t argc, char **argv) {
    struct http_client *hc = NULL;

    if (log_init("../cli/log.cfg", "http_client_test") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    char *p_leak = malloc(1024);
    p_leak = NULL;
#if 0
    curl_global_init(CURL_GLOBAL_ALL);

    debug("url count: %ld", ARRAY_SIZE(__urls));

    struct http_request *req = calloc(1, sizeof(struct http_request));
    req->action = HTTP_ACTION_GET;

    for (size_t i = 0; i < ARRAY_SIZE(__urls); i++) {
        const char *url = __urls[i];

        if (NULL == hc) {
            hc = http_client_create(url, &default_http_client_options);
        } else {
            http_client_reset(hc, url);
        }

        struct http_response *resp = http_do(hc, req);
        if (resp) {
            if (resp->ret == -1) {
                error("http do failed,  error: '%s'", resp->err_msg);
            } else if (resp->http_code != 200) {
                error("http do failed, http_code:%ld", resp->http_code);
            } else {
                debug("http do success, response data: '%s'", resp->response_data);
            }
            free_http_response(resp);
            debug("\n------------------\n");
        }

        sleep(3);
    }

    if (req) {
        free_http_request(req);
    }

    if (hc) {
        http_client_destory(hc);
    }

    curl_global_cleanup();
#endif
    log_fini();
    return 0;
}