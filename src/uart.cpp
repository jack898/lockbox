/* Helper functions for EE 14 Lab 1
 * www.ece.tufts.edu/ee/14
 * Steven Bell <sbell@ece.tufts.edu>, based almost entirely on work from Joel Grodstein
 * January 2025
 * 
 * Modified April 2025 by Jack Burton to enable USART1, other minor changes
 */
#include "stm32l432xx.h"
#include <stdbool.h>
#include "ee14lib.h"



// The Nucleo 432 wires PA2 to the ST-Link's VCP_TX pin via AF7, and PA3 to
// VCP_RX via AF7.
// In this function, we set up those pins.
// The alternate-function designation presumably sets most on the GPIO pin's
// internals. However, we still set them here to high-speed, pullup-only,
// push-pull drive.
// MODIFIED: Enables USART1 and USART2
static void UART2_GPIO_Init(void) {
        set_gpio_alt_func(GPIOA, 2, 7);  // USART2_TX
        set_gpio_alt_func(GPIOA, 3, 7);  // USART2_RX
    
        GPIOA->OSPEEDR |= 0x3<<(2*2) | 0x3<<(2*3);      // Very high speed
        GPIOA->PUPDR   &= ~((0x3<<(2*2)) | (0x3<<(2*3)));
        GPIOA->PUPDR   |=   (0x1<<(2*2)) | (0x1<<(2*3)); // Pull-up
        GPIOA->OTYPER  &= ~((0x3<<(2*2)) | (0x3<<(2*3))); // Push-pull
    
        // --- USART1 (PA9 = TX, PA10 = RX) ---
        set_gpio_alt_func(GPIOA, 9, 7);  // USART1_TX (D1)
        set_gpio_alt_func(GPIOA,10, 7);  // USART1_RX (D0)
    
        GPIOA->OSPEEDR |= 0x3<<(2*9) | 0x3<<(2*10);      // Very high speed
        GPIOA->PUPDR   &= ~((0x3<<(2*9)) | (0x3<<(2*10)));
        GPIOA->PUPDR   |=   (0x1<<(2*9)) | (0x1<<(2*10)); // Pull-up
        GPIOA->OTYPER  &= ~((0x3<<(2*9)) | (0x3<<(2*10))); // Push-pull
}


// Set for 8 data bits, 1 start & 1 stop bit, 16x oversampling, 9600 baud.
// And by default, we also get no parity, no hardware flow control (USART_CR3),
// asynch mode (USART_CR2).
static void USART_Init (USART_TypeDef *USARTx, bool tx_en, bool rx_en,int baud){
    // Disable the USART.
    USARTx->CR1 &= ~USART_CR1_UE;  // Disable USART

    // The "M" field is two bits, M1 and M0. We're setting it to 00 (which
    // is the reset value anyway), to use 8-bit words and one start bit.
    USARTx->CR1 &= ~USART_CR1_M;

    // Configure stop bits to 1 stop bit (which is the default). Other
    // choices are .5, 1.5 and 2 stop bits.
    USARTx->CR2 &= ~USART_CR2_STOP;   

    // Set baudrate as desired. This is done by dividing down the APB1 clock.
    // E.g., 80MHz/9600 = 8333 = 0x208D.
    // (We're oversampling by 16; the calculation would be slightly
    // different if we were 8x mode).
    extern uint32_t SystemCoreClock;
    uint32_t val = SystemCoreClock / baud;
    USARTx->BRR  = val;

    // Configure oversampling mode: Oversampling by 16 (which is the
    // default). This means that our Rx runs at 16x the nominal baud rate.
    // If we're not enabling the Rx anyway, this step is moot (but harmless).
    USARTx->CR1 &= ~USART_CR1_OVER8;

    // Turn on transmitter and receiver enables. Note that the entire USART
    // is still disabled, though. Turning on the Rx enable kicks off the Rx
    // looking for a stop bit.
    if (tx_en)
	USARTx->CR1  |= USART_CR1_TE;
    if (rx_en)
	USARTx->CR1  |= USART_CR1_RE;
	
    // We originally turned off the USART -- now turn it back on.
    // Note that page 1202 says to turn this on *before* asserting TE and/or RE.
    USARTx->CR1  |= USART_CR1_UE; // USART enable                 
	
    // Verify that the USART is ready to transmit...
    if (tx_en)
	while ( (USARTx->ISR & USART_ISR_TEACK) == 0)
	    ;
    // ... and to receive.
    if (rx_en)
	while ( (USARTx->ISR & USART_ISR_REACK) == 0)
	    ;
}


void UART_write_byte (USART_TypeDef *USARTx, char data) {
    // spin-wait until the TXE (TX empty) bit is set
    while (!(USARTx->ISR & USART_ISR_TXE));

    // Writing USART data register automatically clears the TXE flag 	
    USARTx->TDR = data & 0xFF;

    // Wait 300us or so, to let the HW clear TXE.
    USART_Delay (300);
}

// Assume that each usec of delay is about 13 times around the NOP loop.
// That's probably about right at 80 MHz (maybe a bit too slow).
void USART_Delay(uint32_t us) {
    uint32_t time = 100*us/7;    
    while(--time);   
}


// Turn on the clock for a GPIO port.
static void gpio_enable_port (GPIO_TypeDef *gpio) {
    unsigned long field;
    if (gpio==GPIOA)      field=RCC_AHB2ENR_GPIOAEN;
    else if (gpio==GPIOB) field=RCC_AHB2ENR_GPIOBEN;
    else if (gpio==GPIOC) field=RCC_AHB2ENR_GPIOCEN;
    else 		  field=RCC_AHB2ENR_GPIOHEN;
    RCC->AHB2ENR |= field;			// Turn on the GPIO clock
}

// Set a given GPIO pin to be a particular alternate-function.
// Params:
//	gpio: which port; one of GPIOA, GPIOB, ... GPIOH.
//	pin:  0-15, for which GPIO pin in the port.
//	func: which of the 15 alternate functions to use.
void set_gpio_alt_func (GPIO_TypeDef *gpio, unsigned int pin, unsigned int func){
    gpio_enable_port (gpio);			// Turn on the GPIO-port clock.

    // Mode Register (MODER). Two bits of mode for each of the 16 pins/port.
    // And 10 -> alternate function.
    gpio->MODER &= ~(3UL << (2*pin));		// Clear the appropriate field.
    gpio->MODER |= 2UL << (2*pin);		// And set to binary 10.

    // AFRL sets the alternate function for pins 0-7; AFRH for pins 8-15.
    // Each register is just eight four-bit fields (one for each pin).
    // The .h file calls the two registers AFR[0] and AFR[1], but then names
    // the bits with the H and L suffixes!
    int idx = (pin>=8);
    int afr_pin = pin - 8*idx;
    gpio->AFR[idx] &= ~(0xFUL << (4*afr_pin));
    gpio->AFR[idx] |=  (func  << (4*afr_pin));

    // Output Speed Register (OSPEEDR). Two bits for each of the 16 pins/port.
    // And 00 -> low speed.
    gpio->OSPEEDR &= ~(3UL<<(2*pin));		// GPIO output speed=slow

    // Pull Up/Pull Down Register (PUPDR). Two bits for each of the 16
    // pins/port. And 00 -> no pullup or pulldown.
    gpio->PUPDR &= ~(3UL <<(2*pin));		// No PUP or PDN
}

// MODIFIED: Initializes USART1 and USART2
void host_serial_init() {
    int baud=9600;
    int fingerprint_baud = 57600;

    // Enable USART 2 clock
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;  

    // Enable USART1 clock
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    // Select SYSCLK as the USART2 clock source. The reset default is PCLK1;
    // we usually set both SYSCLK and PCLK1 to 80MHz anyway.
    RCC->CCIPR &= ~RCC_CCIPR_USART2SEL;
    RCC->CCIPR |=  RCC_CCIPR_USART2SEL_0;

    // Select SYSCLK as USART1 clock srouce
    RCC->CCIPR &= ~RCC_CCIPR_USART1SEL;   
    RCC->CCIPR |=  RCC_CCIPR_USART1SEL_0; 

    // Set alternate function for PA9 = TX (AF7), PA10 = RX (AF7)
    set_gpio_alt_func(GPIOA, 9, 7);   // USART1_TX
    set_gpio_alt_func(GPIOA, 10, 7);  // USART1_RX

     // Pull-up on both PA9, PA10 (01)
     GPIOA->PUPDR &= ~((0x3 << (2 * 9)) | (0x3 << (2 * 10)));
     GPIOA->PUPDR |=  (0x1 << (2 * 9)) | (0x1 << (2 * 10));

    // Connect the I/O pins to the serial peripheral
    UART2_GPIO_Init();

    USART_Init (USART2, 1, 1, baud);	// Set 9600 for serial montior USART
    USART_Init (USART1, 1, 1, fingerprint_baud); // Set fingerprint USART baud rate to 57600
}

// Very basic function: send a character string to the UART, one byte at a time.
// Spin wait after each byte until the UART is ready for the next byte.
void serial_write (USART_TypeDef *USARTx, const char *buffer, int len) {
    // The main flag we use is Tx Empty (TXE). The HW sets it when the
    // transmit data register (TDR) is ready for more data. TXE is then
    // cleared when we write new data in (by a write to the USART_DR reg).
    // When the HW transfers the TDR into the shift register, it sets TXE=1.
    for (unsigned int i = 0; i < len; i++) {
	    UART_write_byte (USARTx, buffer[i]);
    }

    // RM0394 page 1203 says that you must wait for ISR.TC=1 before you shut
    // off the USART. We never shut off the USART... but we'll wait anyway.
    while (!(USARTx->ISR & USART_ISR_TC));
    USARTx->ISR &= ~USART_ISR_TC;
}


// MODIFICATIONS: Added 1000 tick timeout to prevent hanging in serial_read
char serial_read (USART_TypeDef *USARTx) {
    for (int i = 0; !(USARTx->ISR & USART_ISR_RXNE) && i < 1000; i++);

    // Reading USART_DR automatically clears the RXNE flag 
    return ((char)(USARTx->RDR & 0xFF));
}

