#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "clock.h"
#include "esp_log.h"

typedef struct alarm_item_t {
    struct alarm_item_t *next;
    clock_alarm_t *alarm;
} alarm_item_t;

typedef struct {
    alarm_item_t *head;
} alarm_list_t;

static alarm_list_t s_alarm_list;

/* 1. Screen to be able to retrieve list of alarms, inlcude time, repeat/date
   2. Need to be able to set,
*/

static const char TAG[] = "alarm_list";

static void debug_print_tm(struct tm* time)
{
   char buffer[80];
   strftime(buffer,80,"%x - %I:%M:%S%p", time);
   ESP_LOGD(TAG, "|%s|", buffer );
}

static void alarm_list_insert(clock_alarm_t *alarm_time)
{
    alarm_item_t *new_item =  calloc(sizeof(alarm_item_t), 1);

    new_item->alarm = alarm_time;

    new_item->next = s_alarm_list.head;

    s_alarm_list.head = new_item;

    return;
}

uint32_t clock_alarm_get_num(void)
{
    uint32_t cnt = 0;

    alarm_item_t *i = s_alarm_list.head;

    while(i != NULL) {
        cnt++;
        i = i->next;
    }

    return cnt;
}

void clock_add_alarm(struct tm *time, clock_alarm_repeat_t repeat)
{
    static int id = 0;
    id++;

    clock_alarm_t *alarm = calloc(sizeof(clock_alarm_t), 1);
    alarm->id = id;
    alarm->alarm_time = *time;
    // Clear second seting, we do not care
    alarm->alarm_time.tm_sec = 0;
    alarm->repeat = repeat;

    alarm_list_insert(alarm);

}

void clock_delete_all_alarm(void)
{
    alarm_item_t *it = s_alarm_list.head;
    alarm_item_t *prev = NULL;

    while(it != NULL) {
        prev = it;
        it = it->next;

        free(prev);
    }
    s_alarm_list.head = NULL;
}

void clock_delete_by_id(int id)
{
    alarm_item_t *it = s_alarm_list.head;
    alarm_item_t *prev = NULL;

    while(it != NULL) {
        if(it->alarm->id == id) {

            prev->next = it->next;
            free(it);
            break;
        }

        prev = it;
        it = it->next;
    }
    s_alarm_list.head = NULL;
}

void clock_get_alarms(clock_alarm_t *alarms, size_t num_alarms)
{
    memset(alarms, 0, sizeof(clock_alarm_t)*num_alarms);

    alarm_item_t *it = s_alarm_list.head;

    for (int i = 0; i < num_alarms; i++) {
        if(it == NULL) {
            break;
        }
        alarms[i] = *(it->alarm);
        it = it->next;
    }
    return;
}


bool compare_time(struct tm *now, struct tm *alarm)
{
    return (now->tm_hour == alarm->tm_hour && now->tm_min == alarm->tm_min);
}

bool compare_tm(struct tm *now, struct tm *alarm)
{
    return (now->tm_hour == alarm->tm_hour && now->tm_min == alarm->tm_min);
}

bool clock_get_active_alarm(clock_alarm_t *active_alarm)
{
    alarm_item_t *it = s_alarm_list.head;
    time_t nowtime = time(NULL);
    struct tm now_tm;
    localtime_r(&nowtime, &now_tm);
    now_tm.tm_sec = 0;

    while(it != NULL) {
        // Check if alarm should be triggered
        struct tm *alarm_tm = &it->alarm->alarm_time;
        clock_alarm_repeat_t repeat = it->alarm->repeat;

        ESP_LOGD(TAG, "Comparing now with alarm time:\n");
        debug_print_tm(alarm_tm);
        debug_print_tm(&now_tm);
        ESP_LOGD(TAG, "\n");

        if(repeat == CLOCK_ALARM_REPEAT_NONE) {

            if (mktime(alarm_tm) == mktime(&now_tm)) {
                goto ret;
            }
        } else if (repeat == CLOCK_ALARM_REPEAT_WEEKLY) {
            if (alarm_tm->tm_wday == now_tm.tm_wday && compare_time(&now_tm, alarm_tm)) {
                goto ret;
            }
        }  else if (repeat == CLOCK_ALARM_REPEAT_DAILY) {
            if (compare_time(&now_tm, alarm_tm)) {
                goto ret;
            }
        }

        it = it->next;
    }
ret:
    if (it != NULL) {
        *active_alarm = *it->alarm;
        return true;
    } else {
        memset(active_alarm, 0, sizeof(clock_alarm_t));
        return false;
    }
 }