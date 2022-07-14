/*
 * @Author: CALM.WU
 * @Date: 2022-07-07 10:31:11
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-07-11 18:11:20
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
#include "utils/md5.h"

#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/md5.h>

#include "app_config/app_config.h"

#define TOKEN_SIZE 128
#define ZONE_BUF_LEN 8
#define PROMETHEUS_MANAGER_URL_MAX_LEN 64
#define MAX_HTTP_RESPONSE_BUF_LEN 256
#define SECRET_KEY_LEN 64

static const char *__exporter_register_config_base_path = "application.exporter_register";
// static const char *__exporter_register_env_base_path_fmt =
// "application.exporter_register.envs.%s";
static const char *__iface_config_path = "application.metrics_http_exporter.iface";
static const char *__port_config_path = "application.metrics_http_exporter.port";
static const char *__default_iface_name = "eth0";

struct register_mgr {
    int32_t enabled;
    char    ip[XM_IP_BUF_SIZE];
    int16_t port;
    char    endpoint[HOST_NAME_MAX];
    char    zone[ZONE_BUF_LEN];

    sds secret_key;
    sds header_app_key;
    sds url_register_path;
    sds url_offline_path;

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

// https://gist.github.com/yoshiki/812d35a32bcf175f11cb952ed9d1582a
// https://stackoverflow.com/questions/7627723/how-to-create-a-md5-hash-of-a-string-in-c

static uint8_t *__hmac_sha256(const void *key, int32_t keylen, const uint8_t *data,
                              int32_t datalen) {
    return HMAC(EVP_sha256(), key, keylen, data, datalen, NULL, NULL);
}

static sds __str_2_hexstr(uint8_t *c_str, int32_t c_str_len, int32_t dest_len) {
    int32_t    i, j = 0;
    char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                                 '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    sds        hex_str = sdsnewlen(SDS_NOINIT, dest_len);

    for (i = 0; i < c_str_len; i++) {
        uint8_t byte = c_str[i];
        hex_str[j] = hex_chars[(byte & 0xF0) >> 4];
        hex_str[j + 1] = hex_chars[byte & 0x0F];
        j += 2;
    }
    hex_str[j] = '\0';
    return hex_str;
}

static int32_t __init_register_config() {
    sds env_config_base_path = sdsnew(__exporter_register_config_base_path);

    __register_mgr.enabled =
        appconfig_get_member_bool(__exporter_register_config_base_path, "enable", 0);
    if (likely(__register_mgr.enabled)) {
        // 获取配置网卡名称
        const char *iface_name = appconfig_get_str(__iface_config_path, __default_iface_name);

        // 获取本机ip地址、端口号、endpoint
        get_ipaddr_by_iface(iface_name, __register_mgr.ip, XM_IP_BUF_SIZE);
        strlcpy(__register_mgr.endpoint, get_hostname(), HOST_NAME_MAX);
        __register_mgr.port = (int16_t)appconfig_get_int(__port_config_path, 0);
        if (unlikely(0 == __register_mgr.port)) {
            error("[app_register] metrics_http_exporter port is 0");
            return -1;
        }

        // 获取部署环境名
        const char *env_name =
            appconfig_get_member_str(__exporter_register_config_base_path, "env", NULL);
        if (unlikely(!env_name)) {
            error("[app_register] the env[%s.env] is not config. please check the config file.",
                  __exporter_register_config_base_path);
            goto FAILED_CLEANUP;
        }

        // 通过hostname获取zone
        int32_t index = 0;
        while (index < HOST_NAME_MAX) {
            char s_byte = __register_mgr.endpoint[index];
            if (s_byte == '-' || s_byte == '\0') {
                __register_mgr.zone[index] = '\0';
                break;
            }
            __register_mgr.zone[index] = s_byte;
            index++;
        }
        // 通过环境名获取对应的配置
        env_config_base_path = sdscatfmt(env_config_base_path, ".envs.%s", env_name);
        debug("[app_register] env_config_path: %s", env_config_base_path);

        //
        const char *prometheus_manager_url =
            appconfig_get_member_str(env_config_base_path, "prometheus_manager_url", NULL);
        const char *url_register_path =
            appconfig_get_member_str(__exporter_register_config_base_path, "register_path", NULL);
        const char *url_offline_path =
            appconfig_get_member_str(__exporter_register_config_base_path, "offline_path", NULL);
        if (unlikely(NULL == prometheus_manager_url || NULL == url_register_path
                     || NULL == url_offline_path)) {
            error("[app_register] the url_register_path[%s.url_register_path] or "
                  "url_offline_path[%s.url_offline_path] or "
                  "prometheus_manager_url[%s.prometheus_manager_url] is not config. please check "
                  "the config file.",
                  __exporter_register_config_base_path, __exporter_register_config_base_path,
                  env_config_base_path);
            goto FAILED_CLEANUP;
        }

        const char *app_key = appconfig_get_member_str(env_config_base_path, "app_key", NULL);
        if (likely(app_key)) {
            __register_mgr.header_app_key = sdsnewlen("X-Appkey: ", 10);
            __register_mgr.header_app_key = sdscat(__register_mgr.header_app_key, app_key);

        } else {
            error(
                "[app_register] the token[%s.app_key] is not config. please check the config file.",
                env_config_base_path);
            goto FAILED_CLEANUP;
        }

        const char *secret_key = appconfig_get_member_str(env_config_base_path, "secret_key", NULL);
        if (likely(secret_key)) {
            __register_mgr.secret_key = sdsnew(secret_key);
        } else {
            error("[app_register] the secret_key[%s.secret_key] is not config. please check the "
                  "config file.",
                  env_config_base_path);
            goto FAILED_CLEANUP;
        }

        sdsfree(env_config_base_path);

        //----------------------------------------------------------------------

        // 注册、注销 url-path
        __register_mgr.url_register_path = sdsnew(prometheus_manager_url);
        __register_mgr.url_register_path =
            sdscat(__register_mgr.url_register_path, url_register_path);

        __register_mgr.url_offline_path = sdsnew(prometheus_manager_url);
        __register_mgr.url_offline_path = sdscat(__register_mgr.url_offline_path, url_offline_path);

        __register_mgr.scrape_interval_secs = (uint32_t)appconfig_get_member_int(
            __exporter_register_config_base_path, "scrape_interval_secs", 1);

        __register_mgr.retry_max_attempts = (uint32_t)appconfig_get_member_int(
            __exporter_register_config_base_path, "retry_max_attempts", 3);
        __register_mgr.retry_backoff_base_ms = (uint16_t)appconfig_get_member_int(
            __exporter_register_config_base_path, "retry_backoff_base_ms", 1000);
        __register_mgr.retry_max_backoff_delay_ms = (uint16_t)appconfig_get_member_int(
            __exporter_register_config_base_path, "retry_max_backoff_delay_ms", 5000);

        debug("[app_register] exporter register info: enabled: %d, ip: '%s', endpoint: '%s', zone: "
              "'%s', header_app_key: '%s', secret_key: '%s', register_path: '%s',  offline_path: "
              "'%s', scrape_interval_secs: %d, retry_max_attempts: %d, retry_backoff_base_ms: %d, "
              "retry_max_backoff_delay_ms: %d",
              __register_mgr.enabled, __register_mgr.ip, __register_mgr.endpoint,
              __register_mgr.zone, __register_mgr.header_app_key, __register_mgr.secret_key,
              __register_mgr.url_register_path, __register_mgr.url_offline_path,
              __register_mgr.scrape_interval_secs, __register_mgr.retry_max_attempts,
              __register_mgr.retry_backoff_base_ms, __register_mgr.retry_max_backoff_delay_ms);
    } else {
        info("[app_register] exporter register to prometheus manager is disabled");
    }
    return 0;

FAILED_CLEANUP:
    if (env_config_base_path) {
        sdsfree(env_config_base_path);
    }
    return -1;
}

static char *__marshal_register_rerquest_body() {
    s2j_create_json_obj(j_req_body);
    s2j_json_set_basic_element(j_req_body, &__register_mgr, string, endpoint);
    s2j_json_set_basic_element(j_req_body, &__register_mgr, string, ip);
    s2j_json_set_basic_element(j_req_body, &__register_mgr, int, port);
    s2j_json_set_basic_element(j_req_body, &__register_mgr, string, zone);
    s2j_json_set_basic_element(j_req_body, &__register_mgr, int, scrape_interval_secs);
    // TODO:
    cJSON_AddStringToObject(j_req_body, "rule_name", "");
    cJSON_AddIntToObject(j_req_body, "is_cm", 1);

    // j_str_req_body will free by cJSON_free
    char *j_str_req_body = cJSON_PrintUnformatted(j_req_body);
    debug("[app_register] request register json body: %s", j_str_req_body);

    s2j_delete_json_obj(j_req_body);
    return j_str_req_body;
}

static struct http_common_resp_body *__unmarshal_common_response_body(char *j_str_resp_body) {
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

static void __add_default_headers(struct http_request *req, const char *sign, time_t now_secs) {
    uuid_t request_id;
    char   out[UUID_STR_LEN] = { 0 };

    uuid_generate_time(request_id);
    uuid_unparse_lower(request_id, out);

    sds header_timestamp = sdsnewlen("X-Timestamp: ", 13);
    sds header_signature = sdsnewlen("X-Signature: ", 13);
    sds header_request_id = sdsnewlen("X-RequestID: ", 13);

    header_request_id = sdscatlen(header_request_id, (char *)out, UUID_STR_LEN);
    header_signature = sdscat(header_signature, sign);
    header_timestamp = sdscatfmt(header_timestamp, "%i", now_secs);
    // curl_slist_append会调用strdup，所以sds可以释放
    http_header_add(req, "Content-Type: application/json");
    http_header_add(req, header_timestamp);
    http_header_add(req, __register_mgr.header_app_key);
    http_header_add(req, header_signature);
    http_header_add(req, header_request_id);

    debug("[app_register] header_timestampd: '%s', header_appkey: '%s', header_signature: '%s', "
          "header_request_id: '%s'",
          header_timestamp, __register_mgr.header_app_key, header_signature, header_request_id);

    sdsfree(header_timestamp);
    sdsfree(header_signature);
    sdsfree(header_request_id);
}

static sds __generate_signature(const char *method, const char *url, const char *j_str_body,
                                time_t now_secs) {
    // 计算j_str_body的md5，16进制字符串
    uint8_t md5_digest[MD5_DIGEST_LENGTH] = { 0 };
    // md5_calc((const uint8_t *)j_str_body, strlen(j_str_body), (uint8_t *)md5_val);
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, j_str_body, strlen(j_str_body));
    MD5_Final(md5_digest, &ctx);

    sds md5_digest_hex =
        __str_2_hexstr((uint8_t *)md5_digest, XM_MD5_BLOCK_SIZE, XM_MD5_BLOCK_SIZE * 2 + 1);

    // sha256计算的字符串
    sds sign_str = sdscatfmt(sdsempty(), "%s\n%s\n%i\n%s", method, url, now_secs, md5_digest_hex);
    debug("[app_register] sign_str: '%s'", sign_str);

    // 计算sha256
    uint8_t *sha_digest =
        __hmac_sha256(__register_mgr.secret_key, sdslen(__register_mgr.secret_key),
                      (const uint8_t *)sign_str, sdslen(sign_str));
    // sha_digest ---> hex
    sds sha_digest_hex =
        __str_2_hexstr(sha_digest, SHA256_DIGEST_LENGTH, SHA256_DIGEST_LENGTH * 2 + 1);

    debug("[app_register] body_md5_digest_hex: '%s', sha_digest_hex: '%s'", md5_digest_hex,
          sha_digest_hex);

    sdsfree(sign_str);
    sdsfree(md5_digest_hex);

    return sha_digest_hex;
}

static struct http_request *__make_register_request() {
    struct http_request *reg_req = NULL;

    // 生成json body字符串
    char *j_str_req_body = __marshal_register_rerquest_body();
    if (likely(j_str_req_body)) {
        // 创建http请求
        reg_req = http_request_create(HTTP_POST, j_str_req_body, strlen(j_str_req_body));
        if (likely(reg_req)) {
            time_t now_secs = now_realtime_sec();
            // TODO: 计算签名
            sds sha_digest_hex = __generate_signature("POST", __register_mgr.url_register_path,
                                                      j_str_req_body, now_secs);
            if (likely(sha_digest_hex)) {
                // 添加headers curl_slist_append会调用strdup，所以sds可以释放
                __add_default_headers(reg_req, sha_digest_hex, now_secs);

                sdsfree(sha_digest_hex);
            } else {
                error("[app_register] register request generate signature failed");
            }
        } else {
            error("[app_register] create http  register request failed");
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
                        __unmarshal_common_response_body(reg_resp->data);

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
                        error("[app_register] register http_do failed, error: '%s'",
                              reg_resp->err_msg);
                    } else if (reg_resp->http_code != 200) {
                        error("[app_register] http_code:%ld is HTTP_OK", reg_resp->http_code);
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

static struct http_request *__make_offline_request() {
    struct http_request *reg_req = NULL;

    s2j_create_json_obj(j_req_body);
    s2j_json_set_basic_element(j_req_body, &__register_mgr, string, endpoint);
    s2j_json_set_basic_element(j_req_body, &__register_mgr, string, ip);
    s2j_json_set_basic_element(j_req_body, &__register_mgr, int, port);
    s2j_json_set_basic_element(j_req_body, &__register_mgr, string, zone);
    cJSON_AddIntToObject(j_req_body, "is_cm", 1);

    // j_str_req_body will free by cJSON_free
    char *j_str_req_body = cJSON_PrintUnformatted(j_req_body);
    s2j_delete_json_obj(j_req_body);
    debug("[app_register] request offline json body: %s", j_str_req_body);

    if (likely(j_str_req_body)) {
        // 创建http请求
        reg_req = http_request_create(HTTP_POST, j_str_req_body, strlen(j_str_req_body));
        if (likely(reg_req)) {
            time_t now_secs = now_realtime_sec();
            sds    sha_digest_hex = __generate_signature("POST", __register_mgr.url_offline_path,
                                                         j_str_req_body, now_secs);
            if (likely(sha_digest_hex)) {
                // 添加headers curl_slist_append会调用strdup，所以sds可以释放
                __add_default_headers(reg_req, sha_digest_hex, now_secs);

                sdsfree(sha_digest_hex);
            } else {
                error("[app_register] offline request generate signature failed.");
            }
        } else {
            error("[app_register] create http offline request failed");
        }
        cJSON_free(j_str_req_body);
    }

    return reg_req;
}

static void __do_unregister() {
    struct http_request *offline_req = NULL;
    int32_t              http_do_ret = -1;

    offline_req = __make_offline_request();
    if (likely(offline_req)) {
        struct http_response *offline_resp = http_do(__register_mgr.hc, offline_req);
        if (likely(offline_resp)) {
            if (likely(offline_resp->ret != -1 && offline_resp->http_code == 200)) {
                // 解析response
                struct http_common_resp_body *o_resp =
                    __unmarshal_common_response_body(offline_resp->data);

                if (likely(o_resp)) {
                    if (o_resp->code == 0) {
                        http_do_ret = 0;
                        info("[app_register] exporter offline to prometheus manager success");
                    }
                    s2j_delete_struct_obj(o_resp);
                    o_resp = NULL;
                }
            }

            if (unlikely(-1 == http_do_ret)) {

                if (offline_resp->ret == -1) {
                    error("[app_register] offline http_do failed, error: '%s'",
                          offline_resp->err_msg);
                } else if (offline_resp->http_code != 200) {
                    error("[app_register] offline http_code:%ld is HTTP_OK",
                          offline_resp->http_code);
                }
            }
            http_response_free(offline_resp);
        }

        http_request_free(offline_req);
    }
    return;
}

#if 0
static void __test_digest() {
    const char *str = "hello world";

    // 计算j_str_body的md5，16进制字符串
    uint8_t md5_digest[MD5_DIGEST_LENGTH] = { 0 };
    // md5_calc((const uint8_t *)j_str_body, strlen(j_str_body), (uint8_t *)md5_val);
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, str, strlen(str));
    MD5_Final(md5_digest, &ctx);

    sds md5_digest_hex =
        __str_2_hexstr((uint8_t *)md5_digest, XM_MD5_BLOCK_SIZE, XM_MD5_BLOCK_SIZE * 2 + 1);

    uint8_t *sha_digest =
        __hmac_sha256(__register_mgr.secret_key, sdslen(__register_mgr.secret_key),
                      (const uint8_t *)str, strlen(str));
    // sha_digest ---> hex
    sds sha_digest_hex =
        __str_2_hexstr(sha_digest, SHA256_DIGEST_LENGTH, SHA256_DIGEST_LENGTH * 2 + 1);

    debug("[app_register] test 'hello world' md5_digest_hex: '%s', sha_digest_hex: '%s'",
          md5_digest_hex, sha_digest_hex);

    sdsfree(sha_digest_hex);
    sdsfree(md5_digest_hex);
}
#endif

/**
 * It initializes the global variable `__register_mgr` and creates a `http_client` instance
 *
 * @return The return value is the result of the function.
 */
int32_t exporter_register() {
    debug("[app_register] start exporter register...");

    if (likely(0 == __init_register_config())) {
        curl_global_init(CURL_GLOBAL_ALL);

        //__test_digest();

        __register_mgr.hc =
            http_client_create(__register_mgr.url_register_path, &default_http_client_options);
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
 * It's a wrapper of the `http_client_destory` function, which is used to destroy the http
 * client
 */
void exporter_unregister() {
    debug("[app_register] start exporter unregister...");

    if (likely(__register_mgr.enabled)) {
        http_client_reset(__register_mgr.hc, __register_mgr.url_offline_path);

        __do_unregister();

        if (likely(__register_mgr.hc)) {
            http_client_destory(__register_mgr.hc);
        }

        if (likely(__register_mgr.url_register_path)) {
            sdsfree(__register_mgr.url_register_path);
        }

        if (likely(__register_mgr.url_offline_path)) {
            sdsfree(__register_mgr.url_offline_path);
        }

        if (likely(__register_mgr.header_app_key)) {
            sdsfree(__register_mgr.header_app_key);
        }

        if (likely(__register_mgr.secret_key)) {
            sdsfree(__register_mgr.secret_key);
        }

        curl_global_cleanup();
    } else {
        info("[app_register] exporter register to prometheus manager is disabled");
    }
    debug("[app_register] exporter unregister done");
}
