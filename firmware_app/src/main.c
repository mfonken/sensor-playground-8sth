// main.c

#include "app.h"


int main() 
{
    status_t status = app_init();
    if (status != STATUS_OK) 
    {
        printf("Unable to initialize app\n");
        return -1;
    }
    while(true) 
    {
        app_run();
    }
    return 0;
}