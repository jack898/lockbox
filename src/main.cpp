#include "Adafruit_Fingerprint.h"
#include "ee14lib.h"
//#include <cstring>
#include <cstdio>

#define printf(msg) serial_write(USART2, msg, sizeof(msg)-1)
#define FINGERPRINT_START_CODE_H 0xEF
#define FINGERPRINT_START_CODE_L 0x01
#define FINGERPRINT_ADDR 0xFFFFFFFF

#define FINGERPRINT_COMMANDPACKET 0x01
#define FINGERPRINT_ACKPACKET     0x07

#define FINGERPRINT_GETIMAGE        0x01
#define FINGERPRINT_IMAGE2TZ       0x02
#define FINGERPRINT_REGMODEL       0x05
#define FINGERPRINT_STORE          0x06

#define CHARBUFFER1 0x01
#define CHARBUFFER2 0x02

void send_packet_verify_password();
// Instantiate fingerprint object on USART2
Adafruit_Fingerprint finger(USART1, 0x00000000);  // default password


// This function is called by printf() to handle the text string
// We want it to be sent over the serial terminal, so we just delegate to that function
int _write(int file, char *data, int len) {
    serial_write(USART2, data, len);
    return len;
}

void print_hex_response(char *data, int len);


// Command codes from datasheet
#define FINGERPRINT_ENROLLSTART 0x22
#define FINGERPRINT_ENROLL1 0x23
#define FINGERPRINT_ENROLL2 0x24
#define FINGERPRINT_ENROLL3 0x25
#define FINGERPRINT_CAPTUREIMAGE 0x01
#define FINGERPRINT_IMAGETOTZ 0x02
// Result codes
#define FINGERPRINT_OK 0x00
#define FINGERPRINT_PACKETRECEIVER 0x01
#define FINGERPRINT_NOFINGER 0x02
#define FINGERPRINT_ENROLLMISMATCH 0x0A
#define FINGERPRINT_EMPTYLIBRARY 0x0D

void send_fingerprint_command(uint8_t command, uint8_t *args, uint8_t args_len);
bool read_fingerprint_response(char *resp, int expected_len);
uint8_t read_fingerprint_response_with_status(char *resp, int expected_len);



/* Send a command packet to the fingerprint sensor
 * Arguments:
 *  command The command byte
 *  args Pointer to argument bytes
 *  args_len Length of the arguments*/
void send_fingerprint_command(uint8_t command, uint8_t *args, uint8_t args_len) {
    uint8_t packet[32];
    uint16_t idx = 0;
    uint16_t checksum = 0;

    // Header
    packet[idx++] = 0xEF;
    packet[idx++] = 0x01;
    packet[idx++] = 0xFF;
    packet[idx++] = 0xFF;
    packet[idx++] = 0xFF;
    packet[idx++] = 0xFF;

    packet[idx++] = 0x01;  // Command packet

    // Payload length = instruction + args + checksum (2 bytes)
    uint16_t payload_len = 1 + args_len + 2;

    packet[idx++] = (payload_len >> 8) & 0xFF;
    packet[idx++] = payload_len & 0xFF;

    // Instruction (command)
    packet[idx++] = command;

    // Args (if any)
    for (uint8_t i = 0; i < args_len; i++) {
        packet[idx++] = args[i];
    }

    // Calculate checksum (from packet type through last arg)
    checksum = 0x01 + ((payload_len >> 8) & 0xFF) + (payload_len & 0xFF) + command;
    for (uint8_t i = 0; i < args_len; i++) {
        checksum += args[i];
    }

    // Append checksum
    packet[idx++] = (checksum >> 8) & 0xFF;
    packet[idx++] = checksum & 0xFF;

    // Flush UART and send packet
    finger.uart_flush_tx(USART1);
    for (uint8_t i = 0; i < idx; i++) {
        serial_write(USART1, (char *)&packet[i], 1);
        for (volatile int d = 0; d < 4000; d++);  // ~100us inter-byte delay
    }
    finger.uart_flush_tx(USART1);
}


// NOT WORKING
bool read_fingerprint_response(char *resp, int expected_len) {
    for (int i = 0; i < expected_len; i++) {
        resp[i] = serial_read(USART1);
    }

    print_hex_response(resp, expected_len);

    return (resp[0] == (char)0xEF &&
            resp[1] == (char)0x01 &&
            resp[6] == 0x07);  // ACK packet
}

// NOT WORKING
uint8_t read_fingerprint_response_with_status(char *resp, int max_len) {
    // Wait for the real start of packet (0xEF 0x01)
    while (1) {
        char b0 = serial_read(USART1);
        if ((uint8_t)b0 == 0xEF) {
            char b1 = serial_read(USART1);
            if ((uint8_t)b1 == 0x01) {
                // Aligned — store and break
                resp[0] = b0;
                resp[1] = b1;
                break;
            }
        }
    }

    // Read remaining 7 bytes of header
    for (int i = 2; i < 9; i++) {
        resp[i] = serial_read(USART1);
    }

    // Verify it's an ACK packet
    if ((uint8_t)resp[6] != 0x07) {
        printf("Not an ACK packet\n");
        return 0xFF;
    }

    // Parse payload length
    uint16_t len = ((uint8_t)resp[7] << 8) | (uint8_t)resp[8];
    int total_len = 9 + len;
    if (total_len > max_len) total_len = max_len;

    // Read the payload + checksum
    for (int i = 9; i < total_len; i++) {
        resp[i] = serial_read(USART1);
    }

    // Debug output
    printf("RECEIVING PACKET: ");
    print_hex_response(resp, total_len);

    return (uint8_t)resp[9];  // Confirmation code
}


void print_byte_hex(uint8_t byte) {
    const char hex_digits[] = "0123456789ABCDEF";

    char out[5];
    out[0] = '0';
    out[1] = 'x';
    out[2] = hex_digits[(byte >> 4) & 0x0F];  // High nibble
    out[3] = hex_digits[byte & 0x0F];         // Low nibble
    out[4] = ' ';

    serial_write(USART2, out, 5);
}

void print_hex_response(char *data, int len) {
    for (int i = 0; i < len; i++) {
        print_byte_hex((uint8_t)data[i]);
    }
    serial_write(USART2, "\r\n", 2);
}

#define CHARBUFFER1 0x01
#define CHARBUFFER2 0x02
#define CMD_ENROLL_START 0x22
#define CMD_ENROLL1      0x23
#define CMD_ENROLL2      0x24
#define CMD_ENROLL3      0x25
#define CMD_GET_IMAGE    0x01
#define CMD_IMAGE2TZ     0x02
#define CMD_STORE        0x06
#define FINGERPRINT_TEMPLATECOUNT 0x1D



void delay_ms(int ms) {
    for (volatile int i = 0; i < ms * 8000; i++);  // crude ~1ms at 80MHz
}

void write_only_enroll(uint16_t page_id) {
    uint8_t args[4];

    // === Step 1: EnrollStart ===
    args[0] = (page_id >> 8) & 0xFF;
    args[1] = page_id & 0xFF;
    args[2] = 0x03;  // 3 samples
    send_fingerprint_command(FINGERPRINT_ENROLLSTART, args, 3);
    delay_ms(200);  // give time to process

    printf("Step 1 done\n");

    // === Step 2: First fingerprint ===
    send_fingerprint_command(FINGERPRINT_GETIMAGE, NULL, 0);
    delay_ms(800);
    args[0] = CHARBUFFER1;
    send_fingerprint_command(FINGERPRINT_IMAGE2TZ, args, 1);
    delay_ms(200);
    send_fingerprint_command(FINGERPRINT_ENROLL1, NULL, 0);
    delay_ms(300);

    printf("Step 2 done\n");

    // === Step 3: Second fingerprint ===
    send_fingerprint_command(FINGERPRINT_GETIMAGE, NULL, 0);
    delay_ms(800);
    args[0] = CHARBUFFER2;
    send_fingerprint_command(FINGERPRINT_IMAGE2TZ, args, 1);
    delay_ms(200);
    send_fingerprint_command(FINGERPRINT_ENROLL2, NULL, 0);
    delay_ms(300);

    printf("Step 3 done\n");

    // === Step 4: Third fingerprint ===
    send_fingerprint_command(FINGERPRINT_GETIMAGE, NULL, 0);
    delay_ms(800);
    args[0] = CHARBUFFER1;
    send_fingerprint_command(FINGERPRINT_IMAGE2TZ, args, 1);
    delay_ms(200);
    send_fingerprint_command(FINGERPRINT_ENROLL3, NULL, 0);
    delay_ms(300);

    printf("Step 4 done\n");

    // === Step 5: Store to flash ===
    args[0] = CHARBUFFER1;
    args[1] = (page_id >> 8) & 0xFF;
    args[2] = page_id & 0xFF;
    send_fingerprint_command(FINGERPRINT_STORE, args, 3);
    delay_ms(300);

    printf("Step 5 done, stored successfully.\n");
}

void write_only_match() {
    uint8_t args[4];

    // Step 1: Get image
    send_fingerprint_command(FINGERPRINT_GETIMAGE, NULL, 0);
    for (volatile int i = 0; i < 1000000; i++);  // wait ~1s

    // Step 2: Convert to char buffer 1
    args[0] = CHARBUFFER1;
    send_fingerprint_command(FINGERPRINT_IMAGE2TZ, args, 1);
    for (volatile int i = 0; i < 500000; i++);  // wait ~0.5s

    // Step 3: Search template DB
    args[0] = CHARBUFFER1;  // buffer
    args[1] = 0x00;         // start page high
    args[2] = 0x00;         // start page low
    args[3] = 0xC8;         // page count = 200 (0xC8)
    send_fingerprint_command(0x04, args, 4);  // 0x04 = Search
    for (volatile int i = 0; i < 500000; i++);
}


int main() {
    // Initialize system
    host_serial_init();
    gpio_config_mode(D9, OUTPUT);
    serial_write(USART2, "getting here1\n", 14);

    // Give sensor time to boot
    for (volatile int i = 0; i < 800000; i++);  // ~0.5s at 80 MHz


    
    serial_write(USART2, "getting here2\n", 14);
    uint8_t password_args[4] = { 0x00, 0x00, 0x00, 0x00 };  // default password
    send_fingerprint_command(FINGERPRINT_VERIFYPASSWORD, password_args, 4);
    char resp[32];
    // NOT WORKING: read response, but on AD2 response is correct
    if (read_fingerprint_response(resp, sizeof(resp))) {
            serial_write(USART2, "Sensor found and password verified!\r\n", 37);
    } else {
            serial_write(USART2, "Fingerprint sensor not found :(\r\n", 33);
    }

    // Try to trigger the capture image command repeatedly
    // while (1) {
    //     send_fingerprint_command(FINGERPRINT_GETIMAGE, NULL, 0);
    //     for (volatile int i = 0; i < 1000000; i++);  // crude ~1s delay
    // }
    //send_fingerprint_command(0x1D, NULL, 0);

    // Loop to match fingerprint without reading any response packets
    while (1) {
        serial_write(USART2, "Place finger to match...\n", 25);
        write_only_match();
        for (volatile int i = 0; i < 3000000; i++);  // ~3s wait
    }
    
    // Enrolls print of id 12 without reading any response packets
    write_only_enroll(12);


    // NOT WORKING
    // char resp2[12];
    // uint8_t status1 = read_fingerprint_response_with_status(resp2, 12);
    // print_hex_response((char*)&status1, 1);
    // while(1);

    // USART_Delay(2000);

    // // Get parameters
    // if (finger.getParameters() == FINGERPRINT_OK) {
    //     char msg[128];
    //     // snprintf(msg, sizeof(msg),
    //     //          "Status: 0x%X\r\nSys ID: 0x%X\r\nCapacity: %u\r\nBaud: %u\r\n",
    //     //          finger.status_reg,
    //     //          finger.system_id,
    //     //          finger.capacity,
    //     //          finger.baud_rate);
    //     serial_write(USART2, msg, strlen(msg));
    // }

    // // Main loop (blink LED? wait for finger?)
    // while (1) {
    //     uint8_t result = finger.getImage();
    //     if (result == FINGERPRINT_OK) {
    //         serial_write(USART2, "Image taken\r\n", 13);

    //         result = finger.image2Tz();
    //         if (result == FINGERPRINT_OK) {
    //             serial_write(USART2, "Image converted to template\r\n", 30);
    //         }
    //     }
    //     USART_Delay(500000);  // 0.5s delay between attempts
    // }

    // return 0;
}

