/* Fingerprint Library/Proof-of-Concept
 * Jack Burton, Katherine Deane, Ege Ozgul, Brady Kamali
 * April 2025
 * 
 * Enables UART communication with fingerprint sensor via helper functions,
 * and demonstrates enrolling/matching fingerprints
 */

#include "ee14lib.h"
#include <cstdio>

// Serial monitor print helper
#define printf(msg) serial_write(USART2, msg, sizeof(msg) - 1)

// Packet and Command constants
#define FINGERPRINT_START_CODE_H 0xEF
#define FINGERPRINT_START_CODE_L 0x01
#define FINGERPRINT_ADDR 0xFFFFFFFF
#define FINGERPRINT_COMMANDPACKET 0x01
#define FINGERPRINT_ACKPACKET 0x07
#define FINGERPRINT_GETIMAGE 0x01
#define FINGERPRINT_IMAGE2TZ 0x02
#define FINGERPRINT_REGMODEL 0x05
#define FINGERPRINT_STORE 0x06
#define FINGERPRINT_ENROLLSTART 0x22
#define FINGERPRINT_ENROLL1 0x23
#define FINGERPRINT_ENROLL2 0x24
#define FINGERPRINT_ENROLL3 0x25
#define FINGERPRINT_TEMPLATECOUNT 0x1D
#define FINGERPRINT_VERIFYPASSWORD 0x13
#define FINGERPRINT_SEARCH 0x04
#define CHARBUFFER1 0x01
#define CHARBUFFER2 0x02

/* delay_ms
Purpose: Lazy delay with for loop, approximately in milliseconds
Arguments: 
 ms: Milliseconds to delay for
Returns: None
*/
void delay_ms(int ms) {
    for (volatile int i = 0; i < ms * 8000; i++);
}

/* print_byte_hex
Purpose: Debug helper, prints byte as hex 
Arguments: 
 byte: Byte of string
Returns: None
*/
void print_byte_hex(uint8_t byte) {
    const char hex_digits[] = "0123456789ABCDEF";
    char out[5] = {'0', 'x', hex_digits[(byte >> 4) & 0x0F], hex_digits[byte & 0x0F], ' '};
    serial_write(USART2, out, 5);
}


/* print_hex_response
Purpose: Prints data as hex bytes
Arguments: 
 data: Pointer to string
 len: Length of string
Returns: None--check serial monitor for output  
*/
void print_hex_response(char *data, int len) {
    for (int i = 0; i < len; i++) {
        print_byte_hex((uint8_t)data[i]);
    }
    serial_write(USART2, "\r\n", 2);
}

/* send_fingerprint_command
   Purpose: Sends command packet to sensor
   Arguments: 
    command: Command byte, see documentation
    args: Optional arguments, see documentation per command
    args_len: Length of arguments, see documentation per command
   Returns: None--to see if each command is successful, monitor UART1 on AD2  
*/
void send_fingerprint_command(uint8_t command, uint8_t *args, uint8_t args_len) {
    uint8_t packet[32];
    uint16_t idx = 0;
    uint16_t checksum = 0;

    packet[idx++] = FINGERPRINT_START_CODE_H;
    packet[idx++] = FINGERPRINT_START_CODE_L;
    for (int i = 0; i < 4; i++) packet[idx++] = 0xFF;
    packet[idx++] = FINGERPRINT_COMMANDPACKET;

    uint16_t payload_len = 1 + args_len + 2;
    packet[idx++] = (payload_len >> 8) & 0xFF;
    packet[idx++] = payload_len & 0xFF;
    packet[idx++] = command;

    for (uint8_t i = 0; i < args_len; i++) packet[idx++] = args[i];

    checksum = FINGERPRINT_COMMANDPACKET + (payload_len >> 8) + (payload_len & 0xFF) + command;
    for (uint8_t i = 0; i < args_len; i++) checksum += args[i];
    packet[idx++] = (checksum >> 8) & 0xFF;
    packet[idx++] = checksum & 0xFF;

    for (uint8_t i = 0; i < idx; i++) {
        serial_write(USART1, (char *)&packet[i], 1);
    }
}

/* write_only_enroll
   Purpose: Enrolls new fingerprint on scanner with specified ID
   Arguments:
        page_id: ID for new print, 0-255
   Returns: None--to see if each command is successful, monitor UART1 on AD2  
*/
void write_only_enroll(uint16_t page_id) {
    uint8_t args[4];

    // Initialization
    args[0] = (page_id >> 8) & 0xFF;
    args[1] = page_id & 0xFF;
    args[2] = 0x03;
    send_fingerprint_command(FINGERPRINT_ENROLLSTART, args, 3);
    delay_ms(200); printf("Step 1 done\n");


    // Take first image, add to template
    send_fingerprint_command(FINGERPRINT_GETIMAGE, NULL, 0);
    delay_ms(800);
    args[0] = CHARBUFFER1;
    send_fingerprint_command(FINGERPRINT_IMAGE2TZ, args, 1);
    delay_ms(200);
    send_fingerprint_command(FINGERPRINT_ENROLL1, NULL, 0);
    delay_ms(300); printf("Step 2 done\n");


    // Take second image, add to template
    send_fingerprint_command(FINGERPRINT_GETIMAGE, NULL, 0);
    delay_ms(800);
    args[0] = CHARBUFFER2;
    send_fingerprint_command(FINGERPRINT_IMAGE2TZ, args, 1);
    delay_ms(200);
    send_fingerprint_command(FINGERPRINT_ENROLL2, NULL, 0);
    delay_ms(300); printf("Step 3 done\n");


    // Take third image, add to template
    send_fingerprint_command(FINGERPRINT_GETIMAGE, NULL, 0);
    delay_ms(800);
    args[0] = CHARBUFFER1;
    send_fingerprint_command(FINGERPRINT_IMAGE2TZ, args, 1);
    delay_ms(200);
    send_fingerprint_command(FINGERPRINT_ENROLL3, NULL, 0);
    delay_ms(300); printf("Step 4 done\n");


    // Store full print on sensor
    args[0] = CHARBUFFER1;
    args[1] = (page_id >> 8) & 0xFF;
    args[2] = page_id & 0xFF;
    send_fingerprint_command(FINGERPRINT_STORE, args, 3);
    delay_ms(300); printf("Step 5 done, stored successfully.\n");
}

/* write_only_match
   Purpose: Matches finger image with database to check for valid print
   Arguments: None
   Returns: None--to see if each command is successful, monitor UART1 on AD2  
*/
void write_only_match() {
    uint8_t args[4];
    // Get print image
    send_fingerprint_command(FINGERPRINT_GETIMAGE, NULL, 0);
    delay_ms(50);
    args[0] = CHARBUFFER1;

    // Put template from image in buffer
    send_fingerprint_command(FINGERPRINT_IMAGE2TZ, args, 1);
    delay_ms(50);

    // Search for template on sensor
    args[0] = CHARBUFFER1; args[1] = 0x00; args[2] = 0x00; args[3] = 0xC8;
    send_fingerprint_command(FINGERPRINT_SEARCH, args, 4);
    delay_ms(50);
}

/* Main driver
   Purpose: Init UART and sensor, verify password, loop for matching fingerprint
   Arguments: None
   Returns: None--to see if each command is successful, monitor UART1 on AD2 
   OR run match script on WaveForms, see if output pin goes high 
*/
int main() {
    // Initialize GPIO/UART
    host_serial_init();
    gpio_config_mode(D9, OUTPUT);

    // "wake up" sensor
    uint8_t password_args[4] = {0x00, 0x00, 0x00, 0x00};
    send_fingerprint_command(FINGERPRINT_VERIFYPASSWORD, password_args, 4);

    // Check for matching finger repeatedly
    while (1) {
        serial_write(USART2, "Place finger to match...\n", 25);
        write_only_match();
        delay_ms(300);
    }

    // Enroll new fingerprint with id 12
    write_only_enroll(12);
}
