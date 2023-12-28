#include <stdio.h>
#include "esp_err.h"
#include "clock.h"

#include "unity.h"
#include "unity_test_utils.h"

#define NUM_ALARMS 7

static void create_alarms_no_trigger(void)
{
    time_t rawtime;
    /* Insert some alarms that wont be triggered */
    for (int i = 0; i < NUM_ALARMS; i++) {
        time ( &rawtime );
        struct tm *time = localtime(&rawtime);
        time->tm_year = 2000;
        clock_add_alarm(time, CLOCK_ALARM_REPEAT_NONE);
    }
}


TEST_CASE("Check get number of alarms", "[esp-clock]")
{
    TEST_ASSERT_EQUAL(0, clock_alarm_get_num());

    create_alarms_no_trigger();

    TEST_ASSERT_EQUAL(NUM_ALARMS, clock_alarm_get_num());

    clock_delete_all_alarm();

    TEST_ASSERT_EQUAL(0, clock_alarm_get_num());
}


TEST_CASE("Test can find active REPEAT_NONE alarm", "[esp-clock]")
{
    time_t rawtime;

    TEST_ASSERT_EQUAL(0, clock_alarm_get_num());

    create_alarms_no_trigger();

    clock_alarm_t active_alarm = {0};
    TEST_ASSERT_TRUE(!clock_get_active_alarm(&active_alarm));

    time ( &rawtime );
    struct tm alarm_time = {};
    localtime_r(&rawtime, &alarm_time);
    clock_add_alarm(&alarm_time, CLOCK_ALARM_REPEAT_NONE);
    TEST_ASSERT_TRUE(clock_get_active_alarm(&active_alarm));

    clock_delete_all_alarm();

}


TEST_CASE("Test can find active REPEAT_DAILY alarm", "[esp-clock]")
{
    time_t rawtime;

    TEST_ASSERT_EQUAL(0, clock_alarm_get_num());

    create_alarms_no_trigger();

    clock_alarm_t active_alarm = {0};
    TEST_ASSERT_TRUE(!clock_get_active_alarm(&active_alarm));

    time ( &rawtime );
    struct tm alarm_time = {};
    localtime_r(&rawtime, &alarm_time);
    /* Daily should not trigger if weekday is wrong */
    alarm_time.tm_wday = 8;

    clock_add_alarm(&alarm_time, CLOCK_ALARM_REPEAT_DAILY);
    TEST_ASSERT_TRUE(clock_get_active_alarm(&active_alarm));
    clock_delete_all_alarm();


    time ( &rawtime );
    localtime_r(&rawtime, &alarm_time);
    /* Daily should still trigger if date is wrong */
    alarm_time.tm_mday = 40;

    clock_add_alarm(&alarm_time, CLOCK_ALARM_REPEAT_DAILY);
    TEST_ASSERT_TRUE(clock_get_active_alarm(&active_alarm));

    clock_delete_all_alarm();
}


TEST_CASE("Test can find active REPEAT_WEEKLY alarm", "[esp-clock]")
{
    time_t rawtime;

    TEST_ASSERT_EQUAL(0, clock_alarm_get_num());

    create_alarms_no_trigger();

    clock_alarm_t active_alarm = {0};
    TEST_ASSERT_TRUE(!clock_get_active_alarm(&active_alarm));

    time ( &rawtime );
    struct tm alarm_time = {};
    localtime_r(&rawtime, &alarm_time);
    /* WEEKLY should not trigger if weekday is wrong */
    alarm_time.tm_wday = 8;

    clock_add_alarm(&alarm_time, CLOCK_ALARM_REPEAT_WEEKLY);
    TEST_ASSERT_TRUE(!clock_get_active_alarm(&active_alarm));


    time ( &rawtime );
    localtime_r(&rawtime, &alarm_time);
    /* Weekly should still trigger if date is wrong */
    alarm_time.tm_mday = 40;

    clock_add_alarm(&alarm_time, CLOCK_ALARM_REPEAT_WEEKLY);
    TEST_ASSERT_TRUE(clock_get_active_alarm(&active_alarm));

    clock_delete_all_alarm();

}


TEST_CASE("Test alarm trigger next minute (slow test)", "[esp-clock]")
{
    time_t rawtime;

    TEST_ASSERT_EQUAL(0, clock_alarm_get_num());

    create_alarms_no_trigger();

    clock_alarm_t active_alarm = {0};
    TEST_ASSERT_TRUE(!clock_get_active_alarm(&active_alarm));

    time ( &rawtime );
    struct tm alarm_time = {};
    localtime_r(&rawtime, &alarm_time);
    alarm_time.tm_min++;

    clock_add_alarm(&alarm_time, CLOCK_ALARM_REPEAT_DAILY);
    while(!clock_get_active_alarm(&active_alarm)) {
        vTaskDelay(pdMS_TO_TICKS(500));
        printf("Checking alarms...\n");
    }

    printf("Alarm triggered!\n");

    clock_delete_all_alarm();

}



static bool check_id_unique(int *ids, int num_ids, size_t MAX_IDS)
{
    int count = 0;
    for (int i = 0; i < MAX_IDS; i++) {
        if (ids[i] > 1) {
            printf("Duplicate id: %d\n", i);
            return false;
        }
        if (ids[i] == 1) {
            count++;
        }

    }

    if (count != num_ids) {
        printf("Expected %d ids, got %d\n", num_ids, count);
        return false;
    }
    return true;
}

#define MAX_IDS_NUM 100
TEST_CASE("Check alarm ids are unique", "[esp-clock]")
{
    time_t rawtime;
    int ids[MAX_IDS_NUM] = {};

    create_alarms_no_trigger();

    clock_alarm_t alarms[NUM_ALARMS + 1];
    clock_get_alarms(alarms, NUM_ALARMS + 1);

    // Test we returned the expected amount of alarms
    TEST_ASSERT_EQUAL(0, alarms[NUM_ALARMS].id);
    TEST_ASSERT_NOT_EQUAL(0, alarms[NUM_ALARMS - 1].id);

    // Test alarms are ordered
    for (int i = 0; i < NUM_ALARMS; i++) {
        ids[alarms[i].id]++;
    }

    TEST_ASSERT_TRUE(check_id_unique(ids, NUM_ALARMS, MAX_IDS_NUM));

    /* Delete and re add alarms, ID should not be the same as last time*/
    clock_delete_all_alarm();

    for (int i = 0; i < NUM_ALARMS; i++) {
        rawtime = random();
        struct tm *tm_time = malloc(sizeof(struct tm));
        localtime_r(&rawtime, tm_time);
        clock_add_alarm(tm_time, CLOCK_ALARM_REPEAT_NONE);
    }

    clock_get_alarms(alarms, NUM_ALARMS + 1);

    // Test we returned the expected amount of alarms
    TEST_ASSERT_EQUAL(0, alarms[NUM_ALARMS].id);
    TEST_ASSERT_NOT_EQUAL(0, alarms[NUM_ALARMS - 1].id);

    // Test alarms are ordered
    for (int i = 0; i < NUM_ALARMS; i++) {
        ids[alarms[i].id]++;
    }

    TEST_ASSERT_TRUE(check_id_unique(ids, 2*NUM_ALARMS, MAX_IDS_NUM));

    clock_delete_all_alarm();
}

void app_main(void)
{
    unity_run_menu();
}
