/*
 * @Author: CALM.WU
 * @Date: 2022-07-05 11:34:16
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-07-05 17:16:50
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
    HTTP_ACTION_GET = 0,
    HTTP_ACTION_POST,
    HTTP_ACTION_PUT,
    HTTP_ACTION_DELETE,
};

struct http_request {
    enum http_action action;
    struct curl_slist
         *headers;   // use curl_slist_free_all() after the *perform() call to free this list again
    char *data;
    long  data_len;
};

struct http_response {
    int32_t ret;
    long    http_code;
    char   *err_msg;
    char   *response_data;
    size_t  response_data_len;
};

extern struct http_client *http_client_create(const char *url, struct http_client_options *cfg);

extern void http_client_destory(struct http_client *client);

extern void http_client_reset(struct http_client *client, const char *url);

extern const char *get_url(struct http_client *client);

extern void http_add_header(struct http_request *request, const char *header);

extern struct http_response *http_do(struct http_client *client, struct http_request *request);

extern void free_http_response(struct http_response *response);

extern void free_http_request(struct http_request *request);