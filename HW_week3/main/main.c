#include <stdio.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <string.h>

#define PIN_NUM_MOSI 12
#define PIN_NUM_CLK  27
#define PIN_NUM_CS   14
#define start_point_x 1
#define start_point_y 1
#define end_point_x 4
#define end_point_y 6
#define frame_rate 500

int x = start_point_x;
int y = start_point_y;
int x_direction = 1;
int y_direction = 1;
int frame_count = 0;

uint8_t framebuffer[8] = {};
spi_device_handle_t spi;
esp_timer_handle_t timer_led;

void max7219_send(uint8_t address, uint8_t data) {
    uint8_t tx_buf[2] = {address, data};
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_buf
    };
    spi_device_transmit(spi, &t);  // Blocking transmit
}
void max7219_init() {
    max7219_send(0x0F, 0x00);  // Display test off
    max7219_send(0x0C, 0x01);  // Shutdown mode off
    max7219_send(0x0B, 0x07);  // Scan all digits
    max7219_send(0x0A, 0x07);  // Intensity (0x00 ~ 0x0F)
    max7219_send(0x09, 0x00);  // No decode mode
    for (int i = 0; i < 8; i++) {
        max7219_send(i, 0x00); // Clear all rows
    }
}

void clear(){
    for (int i = 0; i < 8; i++) {
        framebuffer[i] = 0x00;
        max7219_send(i, framebuffer[i]);
    }
}
void set_led(uint8_t row, uint8_t col){
    framebuffer[row] |= (1 << (7-col));
    max7219_send(8-row, framebuffer[row]);
}
void on_timer(void* args){
    frame_count++;
    if(x + x_direction <= 0 || x + x_direction  > 7) {
        x_direction *= -1;
        
    }
    if(y + y_direction < 0 || y + y_direction > 7){
        y_direction *= -1;
    } 
    x += x_direction;
    y += y_direction;
    if(x == end_point_x && y == end_point_y){
        esp_timer_stop(timer_led);
        clear();
        for(int i = 0 ; i < 8 ; i++)set_led(0,i);
        set_led(x,y);
        printf("%d\n", frame_count);

    }
    clear();
    for(int i = 0 ; i < 8 ; i++)set_led(0,i);
    set_led(x,y);
}
void start_timer() {
    const esp_timer_create_args_t timer_args = {
        .callback = &on_timer,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,  // 用任務觸發（預設）
        .name = "frame_timer"
    };
    
    esp_timer_create(&timer_args, &timer_led);
    esp_timer_start_periodic(timer_led, frame_rate*1000);
}
void app_main(void) {
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 2
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,        // 1 MHz
        .mode = 0,                         // SPI mode 0
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1
    };

    spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    spi_bus_add_device(HSPI_HOST, &devcfg, &spi);

    max7219_init();
    start_timer();
}
