/*
 * @Author: CALM.WU
 * @Date: 2022-07-05 11:34:16
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-07-05 15:34:54
 */

#pragma

#include "curl/curl.h"
#include "common.h"

struct http_client_context {
    char   *url;
    int32_t connect_timeout_secs;
    int32_t transfer_timeout_secs;   // maximum time the transfer is allowed to complete
};

typedef void (*http_client_ctx_init_fn_t)(struct http_client_context *ctx);

struct http_client {
    CURL                      *curl;   // curl_easy_cleanup / curl_easy_reset
    struct http_client_context ctx;
};

struct http_request {
    struct curl_slist *headers;   // use curl_slist_free_all() after the *perform() call to free
                                  // this list again
    char *post_data;
    long  post_data_len;
};

struct http_response {
    int32_t ret;
    long    http_code;
    char   *err_msg;
    char   *response_data;
    size_t  response_data_len;
};

extern struct http_client *http_client_init(const char *url, http_client_ctx_init_fn_t fn);

extern void http_client_fini(struct http_client *client);

extern void http_client_reset(struct http_client *client, const char *url,
                              http_client_ctx_init_fn_t fn);

extern const char *get_url(struct http_client *client);

extern void http_add_header(struct http_request *request, const char *header);

extern struct http_response *http_post(struct http_client *client, struct http_request *request);

extern void free_http_response(struct http_response *response);

extern void free_http_request(struct http_request *request);