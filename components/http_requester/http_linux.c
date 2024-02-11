#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <curl/curl.h>
#include "esp_err.h"

typedef struct {
  char *data;
  size_t size;
  size_t max_size;
} Response;

static size_t response_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    Response *mem = (Response *)userp;
    assert(mem->size + realsize < mem->max_size);
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

esp_err_t http_requester_get(char* host, char* path, char* response_data, size_t len)
{
    CURL *curl_handle;
    CURLcode res;
    esp_err_t ret = ESP_OK;
    char url[200] = {};
    Response response = {};
    response.data = response_data;
    response.max_size = len;

    curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    curl_handle = curl_easy_init();

    /* specify URL to get */
    strcat(url, host);
    strcat(url, path);
    printf("libcurl URL: %s\n", url);
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, response_cb);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&response);

    /* some servers do not like requests that are made without a user-agent
        field, so we provide one */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);

    /* get it! */
    res = curl_easy_perform(curl_handle);

    /* check for errors */
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        ret = ESP_FAIL;
    }
    else {
        long response_code;
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
        printf("%lu bytes retrieved with response code %ld\n", (long)response.size, response_code);
        printf("%s", response_data);
        if (response_code != 200) {
            ret = ESP_FAIL;
        }

    }

    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);

    return ret;
}

