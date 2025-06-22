#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_system.h"

#define task_1_done (1 << 0)
#define task_2_done (1 << 1)
EventGroupHandle_t barrier;
SemaphoreHandle_t mtx;
int Task_A_id = 0;
int Task_B_id = 1;

int M1[4][4] = {
    {1,2,3,4},
    {5,6,7,8},
    {9,10,11,12},
    {13,14,15,16},
};
int M2[4][4] = {
    {-1,-2,-3,-4},
    {-5,-6,-7,-8},
    {-9,-10,-11,-12},
    {-13,-14,-15,-16},
};
int M3[4][4] = {
    {0,0,0,0},
    {0,0,0,0},
    {0,0,0,0},
    {0,0,0,0}
};
int sum = 0;

void Task(void* pvparameters)
{
    int id = (int)pvparameters;
    int row_start = id*2;
    int row_end = row_start+2;
    int my_bit = (id == 0) ? task_1_done : task_2_done;
    int local_sum = 0;

    for(int i = row_start ; i < row_end; i++)
    {
        for(int j = 0 ; j < 4 ; j++)
        {
            for(int k = 0 ; k < 4 ; k++)
            {
                M3[i][j] += M1[i][k]*M2[k][j];
            }
        }
    }

    xEventGroupSetBits(barrier, my_bit);
    xEventGroupWaitBits(barrier, task_1_done | task_2_done, pdTRUE, pdTRUE, portMAX_DELAY);

    for(int i = 0 ; i < 4 ; i++)
    {
        for(int j = 0 ; j < 4 ; j++)
        {
            local_sum += M3[i][j];
        }
    }

    xSemaphoreTake(mtx, portMAX_DELAY);
    sum += local_sum;
    xSemaphoreGive(mtx);

    vTaskDelete(NULL);
}

void app_main(void)
{
    barrier =  xEventGroupCreate();
    mtx = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(Task, "Run TaskA", 4096, (void*)Task_A_id, 1, NULL, 0);
    xTaskCreatePinnedToCore(Task, "Run TaskB", 4096, (void*)Task_B_id, 1, NULL, 1);

    for(int i = 0 ; i < 4 ; i++)
    {
        for(int j = 0 ; j < 4 ; j++)
        {
            printf("%d ", M3[i][j]);
        }
        printf("\n");
    }

}