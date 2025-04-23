#include "fingerprint_wrapper.h"

UART_HandleTypeDef huart1;
Adafruit_Fingerprint finger(&huart1);



// Initializing connection to fingerprint sensor
// Arguments: None
// Returns: None, check serial monitor for debug messages
void setup_fingerprint(void) {
        finger.begin(BAUDRATE);
        huart1.Instance = USART1;
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