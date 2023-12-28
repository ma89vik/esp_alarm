#pragma once

#include "esp_err.h"
#include <sys/queue.h>

struct todo_item
{
    char *todo_str;
    TAILQ_ENTRY(todo_item) todo_items;
};

esp_err_t todo_list_update(char *apikey);

struct todo_item* todo_list_foreach(struct todo_item *todo);