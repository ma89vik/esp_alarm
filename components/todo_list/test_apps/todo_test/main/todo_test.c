#include <stdio.h>
#include "todo_list.h"

void app_main(void)
{
     todo_list_update("testkey");

    struct todo_item *item = NULL;
    while((item = todo_list_foreach(item))) {
        printf("item: %s \n", item->todo_str);
    }
    todo_list_clear();

    exit(0);
}
