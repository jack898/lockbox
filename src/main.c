#include "stm32l432xx.h"
#include "ee14lib.h"
#include "Adafruit_Fingerprint.h"
#include <stdio.h>

UART_HandleTypeDef huart1;
Adafruit_Fingerprint finger(&huart1);

// This function is called by printf() to handle the text string
// We want it to be sent over the serial terminal, so we just delegate to that function
int _write(int file, char *data, int len) {
        serial_write(USART2, data, len);
        return len;
}

void uart1_init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio_config_mode(D0, ALTERNATE_FUNCTION);  // PA10 = RX
    gpio_config_alternate_function(D0, 7);

    gpio_config_mode(D1, ALTERNATE_FUNCTION);  // PA9 = TX
    gpio_config_alternate_function(D1, 7);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 57600;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

void setup_fingerprint(void) {
    finger.begin(57600);
    if (finger.verifyPassword()) {
        printf("Fingerprint sensor found!\n");
    } else {
        printf("Sensor not found or wrong password\n");
        while (1);
    }
}

int enroll_fingerprint(uint16_t id) {
    printf("Place finger to enroll...\n");
    while (finger.getImage() != FINGERPRINT_OK);

    if (finger.image2Tz(1) != FINGERPRINT_OK) return -1;

    printf("Remove finger...\n");
    HAL_Delay(2000);

    printf("Place same finger again...\n");
    while (finger.getImage() != FINGERPRINT_OK);

    if (finger.image2Tz(2) != FINGERPRINT_OK) return -1;
    if (finger.createModel() != FINGERPRINT_OK) return -1;

    if (finger.storeModel(id) == FINGERPRINT_OK) {
        printf("Fingerprint enrolled successfully at ID %d\n", id);
        return 0;
    } else {
        printf("Failed to store fingerprint.\n");
        return -1;
    }
}

int match_fingerprint(void) {
    printf("Place finger to match...\n");
    if (finger.getImage() != FINGERPRINT_OK) return -1;
    if (finger.image2Tz(1) != FINGERPRINT_OK) return -1;

    if (finger.fingerFastSearch() == FINGERPRINT_OK) {
        printf("Matched ID #%d with confidence %d\n", finger.fingerID, finger.confidence);
        return finger.fingerID;
    } else {
        printf("No match found\n");
        return -1;
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();  // Assuming this exists in your project
    uart1_init();
    host_serial_init();  // For printf() output via ST-Link

    printf("Starting fingerprint test...\n");
    setup_fingerprint();

    // Uncomment to enroll a fingerprint
    // enroll_fingerprint(1);

    // Uncomment to match a fingerprint
    // match_fingerprint();

    while (1) {
        HAL_Delay(2000);
    }
}
