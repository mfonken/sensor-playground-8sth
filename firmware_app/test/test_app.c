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
    app_init();
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

    usleep(250000);
    for(int i = 0; i < 1000; i++)
    {
        timer_isr();
        usleep(100000);
    }
}

int main(void) 
{
    UNITY_BEGIN();
    RUN_TEST(test_app);
    return UNITY_END();
}
