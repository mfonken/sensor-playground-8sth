#include <unity.h>

#include <pthread.h>
#include <unistd.h>

#include "app.h"


void setUp(void)
{
}

void tearDown(void)
{
}

void* app_runner_thread(void* arg)
{
    status_t status = app_init();
    if (status != STATUS_OK) 
    {
        printf("Unable to initialize app\n");
        TEST_FAIL();
    }

    while(true) 
    {
        app_run();
    }
    return NULL;
}

void test_app(void)
{
    pthread_t thread;
    pthread_create(&thread, NULL, app_runner_thread, NULL);

    int us_delay = 1e6 / TIMER_RATE_HZ;
    usleep(2500); // Wait for app to start
    for(int i = 0; i < 1e5; i++)
    {
        timer_isr();
        usleep(us_delay);
    }
}

int main(void) 
{
    UNITY_BEGIN();
    RUN_TEST(test_app);
    return UNITY_END();
}
