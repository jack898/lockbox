#include "stm32l432xx.h"
#include <stdbool.h>

// Initialize the serial port
void host_serial_init();

// Very basic function: send a character string to the UART, one byte at a time.
// Spin wait after each byte until the UART is ready for the next byte.
//void serial_write(USART_TypeDef *USARTx, const char *buffer, int len);

// Spin wait until we have a byte.
//char serial_read(USART_TypeDef *USARTx);

void USART_Init (USART_TypeDef *USARTx, bool tx_en, bool rx_en,int baud);
void UART2_GPIO_Init(void);
void USART_Delay(uint32_t us);
void set_gpio_alt_func (GPIO_TypeDef *gpio,unsigned int pin,unsigned int func);
void UART2_GPIO_Init(void);
void UART_write_byte (USART_TypeDef *USARTx, char data);
void gpio_enable_port (GPIO_TypeDef *gpio);