#define GPIO_BASE  0x20200000
#define GPIO_LEN  0xB4

#define GPSET0     7
#define GPSET1     8
#define GPCLR0    10
#define GPCLR1    11
#define GPLEV0    13
#define GPLEV1    14

#include "stdint.h"
#include "fcntl.h"
#include "unistd.h"
#include <sys/mman.h>
#include "stdio.h"

static volatile uint32_t  * gpioReg = MAP_FAILED;

uint32_t gpioRead_Bits_0_31(void) {
	return (*(gpioReg + GPLEV0));
}

void gpioWrite_Bits_0_31_Clear(uint32_t levels) {
	*(gpioReg + GPCLR0) = levels;
}

void gpioWrite_Bits_0_31_Set(uint32_t levels) {
	*(gpioReg + GPSET0) = levels;
}

static uint32_t * initMapMem(int fd, uint32_t addr, uint32_t len) {
	return (uint32_t *) mmap(0, len,
			PROT_READ|PROT_WRITE|PROT_EXEC,
			MAP_SHARED|MAP_LOCKED,
			fd, addr);
}

int main() {
	int fdMem;

	fdMem = open("/dev/mem", O_RDWR | O_SYNC) ;
	gpioReg = initMapMem(fdMem, GPIO_BASE, GPIO_LEN);
	close(fdMem);
	uint32_t pins = gpioRead_Bits_0_31();
	printf("bits 0-31 are %08X\n", pins);
	printf("decimal: %d\n", pins);

	printf("ohai\n");

	char bits[33];
	for (int s = 0; s < 32; ++s) {
		bits[31 - s] = '0';
		int b = 1 << s;
		if ((b & pins) == b) {
			bits[31 - s] = '1';
		}
	}
	bits[32] = '\0';

	printf("binary: %s\n", bits);
	
	return 0;
}


