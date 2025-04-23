// Lockbox: main.c
// Contains main driver code for project


#include "main.h"
#include "fingerprint_wrapper.h"

//UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;


// This function is called by printf() to handle the text string
//We want it to be sent over the serial terminal, so we just delegate to that function
int _write(int file, char *data, int len) {
        serial_write(USART2, data, len);
        return len;
}

int main(void) {
    HAL_Init();
    host_serial_init();

    // FOR PRINTF DEBUGGING
    // while (1) {
    //     printf("hello world!\n");
    //     HAL_Delay(2000);
    // }

    
    // Enable D0/D1 for UART communication with fingerprint sensor
    if (gpio_config_mode(D0, ALTERNATE_FUNCTION) != EE14Lib_Err_OK) {   // RX
        printf("Failed to configure pin D0.\n");
    };
    if (gpio_config_alternate_function(D0, 7) != EE14Lib_Err_OK) {  // AF7 for USART1_RX
        printf("Failed to connect UART for pin D0.\n");
    };    
    if (gpio_config_mode(D1, ALTERNATE_FUNCTION) != EE14Lib_Err_OK) {   // TX
        printf("Failed to configure pin D1.\n");
    };
    if (gpio_config_alternate_function(D1, 7) != EE14Lib_Err_OK) { // AF7 for USART1_TX
        printf("Failed to connect UART for pin D1.\n");
    };     

    // Enable USART1 clock
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    // Initialize USART1
    USART_Init(USART1, true, true, BAUDRATE);

    host_serial_init();  // For printf() output via ST-Link

    printf("Starting fingerprint test...\n");
    setup_fingerprint();

    // Uncomment to add a fingerprint with ID 1
    // enroll_fingerprint(1);

    // Uncomment to match a fingerprint
    // match_fingerprint();

    while (1) {
        HAL_Delay(2000);
    }
}
