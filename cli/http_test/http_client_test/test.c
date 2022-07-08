/*
 * @Author: CALM.WU
 * @Date: 2022-07-05 15:49:52
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-07-07 11:28:06
 */

#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/os.h"
#include "utils/backoff_algorithm.h"
#include "utils/clocks.h"
#include "utils/http_client.h"
#include "utils/consts.h"
#include "utils/json/s2j.h"

#define MAX_ZONE_NAME_LEN 8
#define MAX_RULE_NAME_LEN 16
#define MAX_PORT_LEN 8

static const char   *__ip = "127.0.0.1";
static const char   *__zone = "SZA";
static const int32_t __default_scrape_interval_secs = 15;

struct simple_http_request {
    const char      *url;
    enum http_action action;
} requests[] = {
    { "www.baidu.com", HTTP_GET },
    { "127.0.0.1:8071", HTTP_POST },
};

struct export_register_params {
    char    ip[XM_IP_BUF_SIZE];
    char    endpoint[HOST_NAME_MAX];
    char    port[MAX_PORT_LEN];
    char    zone[MAX_ZONE_NAME_LEN];
    int16_t scrape_interval_secs;
    char    rule_name[MAX_RULE_NAME_LEN];
    int16_t is_cm;
};

static char *__marshal_register_params() {
    struct export_register_params erp = {
        .ip = "127.0.0.1",
        .endpoint = "localhost",
        .port = "8071",
        .zone = "SZA",
        .scrape_interval_secs = __default_scrape_interval_secs,
        .rule_name = "test_rule",
        .is_cm = 1,
    };

    s2j_create_json_obj(json_erp);
    // 序列化
    s2j_json_set_basic_element(json_erp, &erp, string, ip);
    s2j_json_set_basic_element(json_erp, &erp, string, endpoint);
    s2j_json_set_basic_element(json_erp, &erp, string, port);
    s2j_json_set_basic_element(json_erp, &erp, string, zone);
    s2j_json_set_basic_element(json_erp, &erp, int, scrape_interval_secs);
    s2j_json_set_basic_element(json_erp, &erp, string, rule_name);
    s2j_json_set_basic_element(json_erp, &erp, int, is_cm);

    char *json_str_erp = cJSON_Print(json_erp);
    debug("export_register_params serialize '%s'", json_str_erp);

    s2j_delete_json_obj(json_erp);

    // cJSON_free(json_str_erp);
    return json_str_erp;
}

static void __unmarshal_register_params(cJSON *json_res) {
    s2j_create_struct_obj(erp, struct export_register_params);

    // 反序列化
    s2j_struct_get_basic_element(erp, json_res, string, ip);
    s2j_struct_get_basic_element(erp, json_res, string, endpoint);
    s2j_struct_get_basic_element(erp, json_res, string, port);
    s2j_struct_get_basic_element(erp, json_res, string, zone);
    s2j_struct_get_basic_element(erp, json_res, int, scrape_interval_secs);
    s2j_struct_get_basic_element(erp, json_res, string, rule_name);
    s2j_struct_get_basic_element(erp, json_res, int, is_cm);

    debug("unmarshal struct export_register_params ip:%s, endpoint:%s, port:%s, zone:%s, "
          "scrape_interval_secs:%d, rule_name:%s, is_cm:%d",
          erp->ip, erp->endpoint, erp->port, erp->zone, erp->scrape_interval_secs, erp->rule_name,
          erp->is_cm);

    s2j_delete_struct_obj(erp);
    s2j_delete_json_obj(json_res);
}

static void __do_get(struct http_client *hc) {
    struct http_request *req = NULL;

    req = http_request_create(HTTP_GET, NULL, 0);

    struct http_response *resp = http_do(hc, req);
    if (resp) {
        if (resp->ret == -1) {
            error("http do failed,  error: '%s'", resp->err_msg);
        } else if (resp->http_code != 200) {
            error("http do failed, http_code:%ld", resp->http_code);
        } else {
            debug("http do success, response data: '%s'", resp->data);
        }
        http_response_free(resp);
    }

    http_request_free(req);
}

static void __do_post(struct http_client *hc) {
    struct http_request *req = NULL;

    uuid_t request_id;
    uuid_generate_time(request_id);

    char *json_str_erps = __marshal_register_params();

    req = http_request_create(HTTP_POST, json_str_erps, strlen(json_str_erps));
    http_header_add(req, "Content-Type: application/json");
    http_header_add(req, "X-Timestamp: 121323");
    http_header_add(req, "X-AppKey: 12345");
    http_header_add(req, "X-Signature: ssdafdasfasdfasdf");
    http_header_add(req, "X-RequestID: 56564-434-343432-2323");

    struct http_response *resp = http_do(hc, req);
    if (resp) {
        if (resp->ret == -1) {
            error("http do failed,  error: '%s'", resp->err_msg);
        } else if (resp->http_code != 200) {
            error("http do failed, http_code:%ld", resp->http_code);
        } else {
            debug("http do success, response data: '%s'", resp->data);
            cJSON *json_res = cJSON_Parse(resp->data);
            __unmarshal_register_params(json_res);
        }
        http_response_free(resp);
    }

    cJSON_free(json_str_erps);
    http_request_free(req);
}

int32_t main(int32_t argc, char **argv) {
    struct http_client *hc = NULL;

    if (log_init("../cli/log.cfg", "http_client_test") != 0) {
        fprintf(stderr, "log init failed\n");
        return -1;
    }

    curl_global_init(CURL_GLOBAL_ALL);

    debug("url count: %ld", ARRAY_SIZE(requests));

    for (size_t i = 0; i < ARRAY_SIZE(requests); i++) {
        struct simple_http_request *shr = &requests[i];

        if (NULL == hc) {
            hc = http_client_create(shr->url, &default_http_client_options);
        } else {
            http_client_reset(hc, shr->url);
        }

        debug("------------------------>");
        if (shr->action == HTTP_GET) {
            __do_get(hc);
        } else if (shr->action == HTTP_POST) {
            __do_post(hc);
        } else {
            error("invalid action: %d", shr->action);
        }
        debug("<------------------------\n");
        sleep(1);
    }

    if (hc) {
        http_client_destory(hc);
    }

    curl_global_cleanup();

    log_fini();
    return 0;
}