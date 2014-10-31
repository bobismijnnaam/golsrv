#define SYST_BASE  0x20003000
#define DMA_BASE   0x20007000
#define CLK_BASE   0x20101000
#define GPIO_BASE  0x20200000
#define UART0_BASE 0x20201000
#define PCM_BASE   0x20203000
#define SPI0_BASE  0x20204000
#define I2C0_BASE  0x20205000
#define PWM_BASE   0x2020C000
#define UART1_BASE 0x20215000
#define I2C1_BASE  0x20804000
#define I2C2_BASE  0x20805000
#define DMA15_BASE 0x20E05000

#define DMA_LEN   0x1000 /* allow access to all channels */
#define CLK_LEN   0xA8
#define GPIO_LEN  0xB4
#define SYST_LEN  0x1C
#define PCM_LEN   0x24
#define PWM_LEN   0x28
#define I2C_LEN   0x1C
