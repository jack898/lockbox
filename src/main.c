#include "stm32l432xx.h"
#include "ee14lib.h"
#include "Adafruit_Fingerprint.h"
#include "uart.c"
#include <stdio.h>

#define BAUDRATE BAUDRATE

UART_HandleTypeDef huart1;
Adafruit_Fingerprint finger(&huart1);

// This function is called by printf() to handle the text string
// We want it to be sent over the serial terminal, so we just delegate to that function
int _write(int file, char *data, int len) {
        serial_write(USART2, data, len);
        return len;
}

// Initializing connection to fingerprint sensor
// Arguments: None
// Returns: None, check serial monitor for debug messages
void setup_fingerprint(void) {
    finger.begin(BAUDRATE);
    if (finger.verifyPassword()) {
        printf("Fingerprint sensor found!\n");
    } else {
        printf("Sensor not found or wrong password\n");
        while (1);
    }
}

// Enroll (store) new fingerprint
// Arguments: Fingerprint ID, must be unique 16-bit integer
// Returns: -1 on fingerprint sensor error, 0 on successful enrollment.
//          Check serial monitor for debug messages.
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

// Match scanned fingerprint to stored prints
// Arguments: None
// Returns: -1 on failure to match or other error.
//          Fingerprint ID on successful match.
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
    //SystemClock_Config();  // Probably unecessary
    
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
    USART_init(USART1, true, true, BAUDRATE);
    huart1.Instance = USART1;

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
