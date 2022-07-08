/*
 * @Author: CALM.WU
 * @Date: 2022-07-05 11:34:16
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-07-07 18:06:22
 */

#pragma

#include "curl/curl.h"
#include "common.h"

struct http_client_options {
    int32_t connect_timeout_secs;
    int32_t transfer_timeout_secs;   // maximum time the transfer is allowed to complete
    int32_t verbose;
};

extern struct http_client_options default_http_client_options;

struct http_client {
    char                      *url;
    CURL                      *curl;   // curl_easy_cleanup / curl_easy_reset
    struct http_client_options cfg;
};

enum http_action {
    HTTP_GET = 0,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
};

// struct http_request;
// typedef void (*http_request_custom_free_fn_t)(struct http_request *req);

struct http_request {
    enum http_action action;
    struct curl_slist
        *headers;   // use curl_slist_free_all() after the *perform() call to free this list again
    const char *data;
    long        data_len;
};

struct http_response {
    int32_t ret;
    long    http_code;
    char   *err_msg;
    char   *data;
    size_t  data_len;
};

extern struct http_client *http_client_create(const char *url, struct http_client_options *cfg);

extern void http_client_destory(struct http_client *client);

extern void http_client_reset(struct http_client *client, const char *url);

extern const char *http_url(struct http_client *client);

extern void http_header_add(struct http_request *request, const char *header);

extern struct http_response *http_do(struct http_client *client, struct http_request *request);

extern void http_response_free(struct http_response *response);

extern struct http_request *http_request_create(enum http_action action, const char *req_data,
                                                size_t req_data_len);

extern void http_request_free(struct http_request *request);