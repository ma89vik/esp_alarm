
#include "todo_list.h"
#include "http_requester.h"

#include "cJSON.h"

#include "esp_log.h"
#include "esp_check.h"

#include <string.h>
#include <sys/queue.h>

#define TODO_LIST_RESPONSE_MAX_LEN 3000
#define TODO_LIST_MAX_REQUEST_LEN 120

#define TODO_LIST_URL "fanmenrui.xyz"
#define TODO_LIST_APIKEY "testkey"

TAILQ_HEAD(todo_head, todo_item) todo_list;
static char *TAG = "todo_list";

static esp_err_t todo_list_parse_response(char *json_str);

esp_err_t todo_list_update(char *apikey)
{
    esp_err_t ret = ESP_OK;
    char request[TODO_LIST_MAX_REQUEST_LEN];

    snprintf(request, TODO_LIST_MAX_REQUEST_LEN, "/homestation/api/active?apikey=%s", apikey );

    ESP_LOGI(TAG, "%s", request);
    char* response_data = malloc(TODO_LIST_RESPONSE_MAX_LEN*sizeof(char));
    ESP_GOTO_ON_FALSE(response_data, ESP_ERR_NO_MEM, err, TAG, "No mem for response data");

    ESP_GOTO_ON_ERROR(http_requester_get(TODO_LIST_URL, request, response_data, TODO_LIST_RESPONSE_MAX_LEN), err, TAG, "Failed to fetch TODO_LIST data");

    ESP_GOTO_ON_ERROR(todo_list_parse_response(response_data), err, TAG, "Failed to parse TODO_LIST data");

    free(response_data);

err:
    return ret;
}

static esp_err_t todo_list_parse_response(char *json_str)
{
 int err;
    cJSON * root = cJSON_Parse(json_str);

    if(root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON response");
        err  = ESP_ERR_INVALID_ARG;
        goto cleanup;
    }

    if (TAILQ_EMPTY(&todo_list)) {
        TAILQ_INIT(&todo_list);
    }

    size_t n = cJSON_GetArraySize(root);
	for (int i=0; i<n; i++)
	{
		cJSON *subitem=cJSON_GetArrayItem(root, i);
        cJSON *title = cJSON_GetObjectItemCaseSensitive(subitem, "title");
        cJSON *completed = cJSON_GetObjectItemCaseSensitive(subitem, "completed");
        if (cJSON_IsFalse(completed)) {
            struct todo_item *item = calloc(1, sizeof(struct todo_item));
            item->todo_str = malloc(strlen(title->valuestring) + 1);
            strcpy(item->todo_str, title->valuestring);

            TAILQ_INSERT_TAIL(&todo_list, item, todo_items);
        }

	}
    err =ESP_OK;

cleanup:
    cJSON_Delete(root);
    return err;
}

struct todo_item * todo_list_foreach(struct todo_item *todo)
{
    if(todo == NULL) {
        return TAILQ_FIRST(&todo_list);
    } else {
        return TAILQ_NEXT(todo, todo_items);
    }
}

void todo_list_clear()
{
    struct todo_item *todo;
    while((todo = TAILQ_FIRST(&todo_list))) {
        TAILQ_REMOVE(&todo_list, todo, todo_items);
        free(todo->todo_str);
        free(todo);
    }
}
