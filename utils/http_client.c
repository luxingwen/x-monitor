/*
 * @Author: CALM.WU
 * @Date: 2022-07-05 14:21:27
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2022-07-07 17:31:46
 */

#include "http_client.h"
#include "compiler.h"
#include "consts.h"
#include "log.h"

#if GCC_VERSION >= 50100
static const int32_t __default_connect_timeout_secs = 2;
static const int32_t __default_transfer_timeout_secs = 3;
#else
#define __default_connect_timeout_secs 2
#define __default_transfer_timeout_secs 3
#endif

struct http_client_options default_http_client_options = {
    .connect_timeout_secs = __default_connect_timeout_secs,
    .transfer_timeout_secs = __default_transfer_timeout_secs,
    .verbose = 0,
};

/**
 * It frees the memory allocated for the http_client struct
 *
 * @param client The http_client struct that we're freeing.
 */
static void __free_http_client(struct http_client *client) {
    if (likely(client)) {
        if (likely(client->curl)) {
            curl_easy_cleanup(client->curl);
            client->curl = NULL;
        }

        if (likely(client->url)) {
            free(client->url);
            client->url = NULL;
        }
        free(client);
    }
}

/**
 * It's a callback function that is called by libcurl when it receives data from the server
 *
 * @param ptr a pointer to the data that was received
 * @param size the size of each element in the memory block, in bytes.
 * @param nmemb number of elements
 * @param data a pointer to the data that will be passed to the callback function
 *
 * @return The number of bytes written to the response.
 */
static size_t __write_response(void *ptr, size_t size, size_t nmemb, void *data) {
    size_t                real_size = size * nmemb;
    struct http_response *response = (struct http_response *)data;

    char *new_data = realloc(response->data, response->data_len + real_size + 1);
    if (unlikely(NULL == new_data)) {
        error("not enough memory to allocate memory for response data");
        return 0;
    }

    response->data = new_data;
    memcpy(response->data + response->data_len, ptr, real_size);
    response->data_len += real_size;
    response->data[response->data_len] = '\0';

    return real_size;
}

/**
 * It initializes a `http_client` struct, and returns a pointer to it
 *
 * @param url The URL to connect to.
 * @param fn a callback function that allows you to set the following parameters:
 *
 * @return A pointer to a struct http_client.
 */
struct http_client *http_client_create(const char *url, struct http_client_options *cfg) {
    struct http_client *hc = NULL;

    hc = calloc(1, sizeof(struct http_client));
    if (unlikely(NULL == hc)) {
        error("calloc failed, reason: %s", strerror(errno));
        return NULL;
    }

    hc->curl = curl_easy_init();
    if (unlikely(NULL == hc->curl)) {
        error("curl_easy_init failed");
        goto failed_cleanup;
    }

    hc->url = strdup(url);
    memcpy(&hc->cfg, cfg, sizeof(struct http_client_options));

    if (cfg->connect_timeout_secs < __default_connect_timeout_secs) {
        hc->cfg.connect_timeout_secs = __default_connect_timeout_secs;
    }
    if (cfg->transfer_timeout_secs < __default_transfer_timeout_secs) {
        hc->cfg.transfer_timeout_secs = __default_transfer_timeout_secs;
    }

    // 全局设置
    curl_easy_setopt(hc->curl, CURLOPT_URL, hc->url);
    curl_easy_setopt(hc->curl, CURLOPT_CONNECTTIMEOUT, hc->cfg.connect_timeout_secs);
    curl_easy_setopt(hc->curl, CURLOPT_TIMEOUT, hc->cfg.transfer_timeout_secs);

    return hc;

failed_cleanup:
    __free_http_client(hc);

    return NULL;
}

/**
 * It frees the memory allocated for the client
 *
 * @param client The client object.
 */
void http_client_destory(struct http_client *client) {
    if (likely(client)) {
        __free_http_client(client);
    }
}

/**
 * It resets the CURL handle and sets the URL, connect timeout and transfer timeout
 *
 * @param client The http_client instance to reset.
 * @param url The URL to connect to.
 * @param fn A function that will be called to initialize the context.
 */
void http_client_reset(struct http_client *client, const char *url) {
    if (likely(NULL != client && NULL != client->curl)) {
        curl_easy_reset(client->curl);

        if (client->url) {
            free(client->url);
            client->url = strdup(url);
        }

        curl_easy_setopt(client->curl, CURLOPT_URL, client->url);
        curl_easy_setopt(client->curl, CURLOPT_CONNECTTIMEOUT, client->cfg.connect_timeout_secs);
        curl_easy_setopt(client->curl, CURLOPT_TIMEOUT, client->cfg.transfer_timeout_secs);
    }
}

/**
 * It returns the URL of the HTTP client
 *
 * @param client The http_client object.
 *
 * @return The URL of the client.
 */
const char *http_url(struct http_client *client) {
    if (likely(client)) {
        return client->url;
    }
    return NULL;
}

void http_header_add(struct http_request *request, const char *header) {
    if (likely(request && header)) {
        request->headers = curl_slist_append(request->headers, header);
    }
}

struct http_response *http_do(struct http_client *client, struct http_request *request) {
    struct http_response *resp = NULL;
    CURLcode              ret = CURLE_OK;

    resp = calloc(1, sizeof(struct http_response));
    if (unlikely(NULL == resp)) {
        error("calloc failed, reason: %s", strerror(errno));
        return NULL;
    }

    // 默认是get
    curl_easy_setopt(client->curl, CURLOPT_HTTPGET, 1);

    if (request->action == HTTP_POST) {
        curl_easy_setopt(client->curl, CURLOPT_POST, 1);

        if (likely(NULL != request->data && request->data_len > 0)) {
            curl_easy_setopt(client->curl, CURLOPT_POSTFIELDS, request->data);
            curl_easy_setopt(client->curl, CURLOPT_POSTFIELDSIZE, request->data_len);
        }
    }

    curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, __write_response);
    curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, resp);

    if (likely(request->headers)) {
        curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, request->headers);
    }

    ret = curl_easy_perform(client->curl);
    if (unlikely(CURLE_OK != ret)) {
        resp->ret = -1;
        resp->err_msg = strdup(curl_easy_strerror(ret));
        error("curl_easy_perform failed, reason: %s", resp->err_msg);
        return resp;
    }

    // 获得服务器返回的状态码
    curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, &resp->http_code);

    return resp;
}

/**
 * It takes a pointer to a struct http_response, frees the memory allocated for the data
 * and err_msg fields, and then frees the struct itself
 *
 * @param response The response object that will be returned to the caller.
 */
void http_response_free(struct http_response *response) {
    if (likely(response)) {
        if (likely(response->data)) {
            free(response->data);
            response->data = NULL;
        }
        if (likely(response->err_msg)) {
            free(response->err_msg);
            response->err_msg = NULL;
        }
        free(response);
        response = NULL;
    }
}

struct http_request *http_request_create(enum http_action action, const char *req_data,
                                         size_t req_data_len) {
    struct http_request *request = NULL;

    request = calloc(1, sizeof(struct http_request));
    if (likely(request)) {
        request->action = action;
        if (likely(NULL != req_data && req_data_len > 0)) {
            request->data = strndup(req_data, req_data_len);
            request->data_len = (long)req_data_len;
        }
    }
    return request;
}

/**
 * It frees the memory allocated for the http_request struct
 *
 * @param request The request object to free.
 */
void http_request_free(struct http_request *request) {
    if (likely(request)) {
        // 释放headers
        if (likely(request->headers)) {
            curl_slist_free_all(request->headers);
        }
        if (likely(request->data && request->data_len > 0)) {
            free((char *)(request->data));
            request->data = NULL;
        }
        free(request);
        request = NULL;
    }
}