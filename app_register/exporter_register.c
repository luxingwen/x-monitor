/*
 * @Author: CALM.WU
 * @Date: 2022-07-07 10:31:11
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-07-07 18:06:42
 */

#include "exporter_register.h"
#include "utils/common.h"
#include "utils/compiler.h"
#include "utils/log.h"
#include "utils/backoff_algorithm.h"
#include "utils/http_client.h"
#include "utils/consts.h"
#include "utils/os.h"
#include "utils/strings.h"
#include "utils/clocks.h"
#include "utils/json/s2j.h"
#include "utils/sds/sds.h"

#include "app_config/app_config.h"

#define TOKEN_SIZE 128
#define ZONE_BUF_LEN 8
#define PROMETHEUS_MANAGER_URL_MAX_LEN 64
#define MAX_HTTP_RESPONSE_BUF_LEN 64

static const char *__register_config_path = "application.exporter_register";
static const char *__iface_config_path = "application.metrics_http_exporter.iface";
static const char *__port_config_path = "application.metrics_http_exporter.port";
static const char *__default_iface_name = "eth0";

struct register_mgr {
    int32_t enabled;
    char    ip[XM_IP_BUF_SIZE];
    int16_t port;
    char    hostname[HOST_NAME_MAX];
    char    zone[ZONE_BUF_LEN];
    char    token[TOKEN_SIZE];
    char    prometheus_manager_url[PROMETHEUS_MANAGER_URL_MAX_LEN];

    int16_t scrape_interval_secs;

    uint16_t retry_backoff_base_ms;
    uint16_t retry_max_backoff_delay_ms;
    uint32_t retry_max_attempts;

    struct http_client *hc;
};

struct http_common_resp_body {
    int32_t code;
    char    message[MAX_HTTP_RESPONSE_BUF_LEN];
};

static struct register_mgr __register_mgr = { .enabled = 0 };

static int32_t __init_register_config() {

    __register_mgr.enabled = appconfig_get_member_bool(__register_config_path, "enable", 0);
    if (likely(__register_mgr.enabled)) {
        //
        const char *iface_name = appconfig_get_str(__iface_config_path, __default_iface_name);

        get_ipaddr_by_iface(iface_name, __register_mgr.ip, XM_IP_BUF_SIZE);
        strlcpy(__register_mgr.hostname, get_hostname(), HOST_NAME_MAX);
        //
        __register_mgr.port = (int16_t)appconfig_get_int(__port_config_path, 0);
        if (unlikely(0 == __register_mgr.port)) {
            error("[app_register] metrics_http_exporter port is 0");
            return -1;
        }
        //
        const char *token = appconfig_get_member_str(__register_config_path, "token", NULL);
        if (likely(token)) {
            strlcpy(__register_mgr.token, token, TOKEN_SIZE);
        } else {
            error("[app_register] the token[%s.token] is not config. please check the config file.",
                  __register_config_path);
            return -1;
        }
        //
        const char *prometheus_manager_url =
            appconfig_get_member_str(__register_config_path, "prometheus_manager_url", NULL);
        if (likely(prometheus_manager_url)) {
            strlcpy(__register_mgr.prometheus_manager_url, prometheus_manager_url,
                    PROMETHEUS_MANAGER_URL_MAX_LEN);
        } else {
            error("[app_register] the prometheus_manager_url[%s.prometheus_manager_url] is not "
                  "config. "
                  "please check the config file.",
                  __register_config_path);
            return -1;
        }

        __register_mgr.scrape_interval_secs =
            (uint32_t)appconfig_get_member_int(__register_config_path, "scrape_interval_secs", 1);

        __register_mgr.retry_max_attempts =
            (uint32_t)appconfig_get_member_int(__register_config_path, "retry_max_attempts", 3);
        __register_mgr.retry_backoff_base_ms = (uint16_t)appconfig_get_member_int(
            __register_config_path, "retry_backoff_base_ms", 1000);
        __register_mgr.retry_max_backoff_delay_ms = (uint16_t)appconfig_get_member_int(
            __register_config_path, "retry_max_backoff_delay_ms", 5000);

        debug("[app_register] exporter register info: enabled: %d, ip: '%s', hostname: '%s', "
              "token: '%s', prometheus_manager_url: '%s', scrape_interval_secs: %d, "
              "retry_max_attempts: %d, retry_backoff_base_ms: %d, retry_max_backoff_delay_ms: %d",
              __register_mgr.enabled, __register_mgr.ip, __register_mgr.hostname,
              __register_mgr.token, __register_mgr.prometheus_manager_url,
              __register_mgr.scrape_interval_secs, __register_mgr.retry_max_attempts,
              __register_mgr.retry_backoff_base_ms, __register_mgr.retry_max_backoff_delay_ms);
    } else {
        info("[app_register] exporter register to prometheus manager is disabled");
    }
    return 0;
}

static char *__marshal_register_rerquest_body() {
    s2j_create_json_obj(j_req_body);
    s2j_json_set_basic_element(j_req_body, &__register_mgr, string, hostname);
    s2j_json_set_basic_element(j_req_body, &__register_mgr, string, ip);
    s2j_json_set_basic_element(j_req_body, &__register_mgr, int, port);
    // TODO:
    s2j_json_set_basic_element(j_req_body, &__register_mgr, string, zone);
    s2j_json_set_basic_element(j_req_body, &__register_mgr, int, scrape_interval_secs);
    // TODO:
    cJSON_AddStringToObject(j_req_body, "rule_name", "rule.for.calmwu");
    cJSON_AddIntToObject(j_req_body, "is_cm", 1);

    // j_str_req_body will free by cJSON_free
    char *j_str_req_body = cJSON_Print(j_req_body);
    debug("[app_register] request json body: %s", j_str_req_body);

    s2j_delete_json_obj(j_req_body);
    return j_str_req_body;
}

static struct http_common_resp_body *__unmarshal_register_response_body(char *j_str_resp_body) {
    cJSON *j_resp = cJSON_Parse(j_str_resp_body);
    s2j_create_struct_obj(o_resp, struct http_common_resp_body);

    // 反序列化
    s2j_struct_get_basic_element(o_resp, j_resp, int, code);
    s2j_struct_get_basic_element(o_resp, j_resp, string, message);

    debug("[app_register] response code: %d, message: '%s'", o_resp->code, o_resp->message);

    s2j_delete_json_obj(j_resp);
    // need s2j_delete_struct_obj to free the memory
    return o_resp;
}

static void __add_default_headers(struct http_request *req) {
    uuid_t request_id;
    char   out[UUID_STR_LEN] = { 0 };

    uuid_generate_time(request_id);
    uuid_unparse_lower(request_id, out);
    time_t now_secs = now_realtime_sec();

    sds header_timestamp = sdsnewlen("X-Timestamp: ", 13);
    sds header_appkey = sdsnewlen("X-Appkey: ", 10);
    sds header_signature = sdsnewlen("X-Signature: ", 13);
    sds header_request_id = sdsnewlen("X-RequestID: ", 13);

    header_request_id = sdscatlen(header_request_id, (char *)out, UUID_STR_LEN);
    header_timestamp = sdscatfmt(header_timestamp, "%i", now_secs);
    // curl_slist_append会调用strdup，所以sds可以释放
    http_header_add(req, "Content-Type: application/json");
    http_header_add(req, header_timestamp);
    http_header_add(req, header_appkey);
    http_header_add(req, header_signature);
    http_header_add(req, header_request_id);

    debug("[app_register] header_timestampd: '%s', header_appkey: '%s', header_signature: '%s', "
          "header_request_id: '%s'",
          header_timestamp, header_appkey, header_signature, header_request_id);

    sdsfree(header_timestamp);
    sdsfree(header_appkey);
    sdsfree(header_signature);
    sdsfree(header_request_id);
}

static struct http_request *__make_register_request() {
    struct http_request *reg_req = NULL;

    char *j_str_req_body = __marshal_register_rerquest_body();
    if (likely(j_str_req_body)) {
        reg_req = http_request_create(HTTP_POST, j_str_req_body, strlen(j_str_req_body));
        if (likely(reg_req)) {
            // 添加header curl_slist_append会调用strdup，所以sds可以释放
            __add_default_headers(reg_req);
        } else {
            error("[app_register] create http request failed");
        }
        cJSON_free(j_str_req_body);
    }

    return reg_req;
}

static int32_t __do_register() {
    struct http_request *reg_req = NULL;
    int32_t              http_do_ret = -1;

    BackoffAlgorithmStatus_t  retryStatus = BackoffAlgorithmSuccess;
    BackoffAlgorithmContext_t retryContext;
    uint16_t                  nextRetryBackoff = 0;

    reg_req = __make_register_request();
    if (likely(reg_req)) {
        BackoffAlgorithm_InitializeParams(&retryContext, __register_mgr.retry_backoff_base_ms,
                                          __register_mgr.retry_max_backoff_delay_ms,
                                          __register_mgr.retry_max_attempts);

        do {
            // backoff_retry
            struct http_response *reg_resp = http_do(__register_mgr.hc, reg_req);
            if (likely(reg_resp)) {
                if (likely(reg_resp->ret != -1 && reg_resp->http_code == 200)) {
                    // 解析response
                    struct http_common_resp_body *o_resp =
                        __unmarshal_register_response_body(reg_resp->data);

                    if (likely(o_resp)) {
                        if (o_resp->code == 0) {
                            info("[app_register] exporter register to prometheus manager success");
                            http_do_ret = 0;
                        }
                        s2j_delete_struct_obj(o_resp);
                        o_resp = NULL;
                    }
                }

                if (unlikely(-1 == http_do_ret)) {
                    // 退避重试
                    retryStatus =
                        BackoffAlgorithm_GetNextBackoff(&retryContext, rand(), &nextRetryBackoff);

                    if (reg_resp->ret == -1) {
                        error("[app_register] http_do failed, error: '%s'", reg_resp->err_msg);
                    } else if (reg_resp->http_code != 200) {
                        error("http_code:%ld is HTTP_OK", reg_resp->http_code);
                    }

                    if (retryStatus == BackoffAlgorithmSuccess) {
                        usleep(nextRetryBackoff * 1000);
                    }
                }

                http_response_free(reg_resp);
            }
        } while ((-1 == http_do_ret) && (BackoffAlgorithmRetriesExhausted != retryStatus));

        http_request_free(reg_req);
    }
    if (unlikely(-1 == http_do_ret)) {
        error("[app_register] exporter register to prometheus manager failed");
    }
    return http_do_ret;
}

static void __do_unregister() {
    return;
}

/**
 * It initializes the global variable `__register_mgr` and creates a `http_client` instance
 *
 * @return The return value is the result of the function.
 */
int32_t exporter_register() {
    debug("[app_register] start exporter register...");

    if (likely(0 == __init_register_config())) {
        curl_global_init(CURL_GLOBAL_ALL);

        __register_mgr.hc =
            http_client_create(__register_mgr.prometheus_manager_url, &default_http_client_options);
        if (likely(__register_mgr.hc)) {
            return __do_register();
        }
    } else {
        error("[app_register] exporter register init config failed");
        return -1;
    }

    debug("[app_register] exporter register success");
    return 0;
}

/**
 * It's a wrapper of the `http_client_destory` function, which is used to destroy the http client
 */
void exporter_unregister() {
    debug("[app_register] start exporter unregister...");

    if (likely(__register_mgr.enabled)) {
        __do_unregister();

        if (likely(__register_mgr.hc)) {
            http_client_destory(__register_mgr.hc);
        }

        curl_global_cleanup();
    } else {
        info("[app_register] exporter register to prometheus manager is disabled");
    }
    debug("[app_register] exporter unregister done");
}
