#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include <string.h>
#include "driver/uart.h"
//#include "driver/dma.h"

#define ECHO_TEST_TXD (1)
#define ECHO_TEST_RXD (3)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM      (CONFIG_EXAMPLE_UART_PORT_NUM)
#define ECHO_UART_BAUD_RATE     (CONFIG_EXAMPLE_UART_BAUD_RATE)
#define ECHO_TASK_STACK_SIZE    (CONFIG_EXAMPLE_TASK_STACK_SIZE)

#define BUTTON_GPIO  0  // Sử dụng GPIO0 làm nút nhấn

static const char *TAG = "UART TEST";

#define BUF_SIZE (1024)
bool button_press = false;
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    button_press =! button_press;
}

static void echo_task(void *arg) {
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    int intr_alloc_flags = 0;
#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    // Cấu hình GPIO cho ngắt
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,  // Ngắt khi có sự thay đổi mức lên
        .mode = GPIO_MODE_INPUT,          // Chế độ input
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .pull_up_en = 1,                  // Kích hoạt pull-up
        .pull_down_en = 0,                // Không sử dụng pull-down
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);  // Cài đặt dịch vụ ngắt
    gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, (void*) BUTTON_GPIO);

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    while (1) {
        // Đọc dữ liệu từ UART

        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_RATE_MS);
        if(button_press)
        {
        if (len > 0) {
            data[len] = '\0';  // Kết thúc chuỗi
            if (strncmp((const char *)data, "huan", 4) == 0) {
                const char *message = "IOT\n";
                uart_write_bytes(ECHO_UART_PORT_NUM, message, strlen(message));  // Gửi thông điệp "IOT"
                ESP_LOGI(TAG, "Recv str: %s", message);
            } else {
                const char *Notmassage = "WRONG\n";
                uart_write_bytes(ECHO_UART_PORT_NUM, Notmassage, strlen(Notmassage));  // Gửi thông điệp "WRONG"
                ESP_LOGI(TAG, "Recv str: %s", Notmassage);
            }
        }
    }
    }
} 

void app_main() {
    
    xTaskCreate(echo_task, "echo_task", 2048, NULL, 10, NULL);
}
