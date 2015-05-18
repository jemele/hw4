// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
// more appropriately called 
#include <stdlib.h>
#include <stdio.h>
#include "platform.h"
#include "gpio.h"

// Initialize gpio.
int gpio_initialize(gpio_t *gpio)
{
    gpio->config = XGpioPs_LookupConfig(XPAR_PS7_GPIO_0_DEVICE_ID);
    if (!gpio->config) {
        printf("XGpioPs_LookupConfig failed\n");
        return XST_FAILURE;
    }
    printf("gpio initialization\n");
    int status = XGpioPs_CfgInitialize(&gpio->device, gpio->config,
            gpio->config->BaseAddr);
    if (status != XST_SUCCESS) {
        printf("XGpioPs_CfgInitialize failed %d\n", status);
        return status;
    }
    printf("gpio selftest\n");
    status = XGpioPs_SelfTest(&gpio->device);
    if (status != XST_SUCCESS) {
        printf("XGpioPs_SelfTest %d\n", status);
        return status;
    }
    printf("lighting up mio7\n");
    XGpioPs_SetDirectionPin(&gpio->device, 7, 1);
    XGpioPs_SetOutputEnablePin(&gpio->device, 7, 1);
    XGpioPs_WritePin(&gpio->device, 7, 1);
    return 0;
}

// Initialize axi gpio.
int gpio_axi_initialize(gpio_axi_t *gpio)
{
    int status;

    printf("gpio initialization\n");
    status = XGpio_Initialize(&gpio->device, gpio->id);
    if (status) {
        printf("XGpio_Initialize failed %d\n", status);
        return status;
    }

    printf("gpio selftest\n");
    status = XGpio_SelfTest(&gpio->device);
    if (status != XST_SUCCESS) {
        printf("XGpio_SelfTest %d\n", status);
        return status;
    }

    return 0;
}

// Read from the buttons axi gpio on channel 1.
// When data is available, return the read value.
u32 gpio_axi_blocking_read(gpio_axi_t *gpio)
{
    const unsigned channel = 1;

	u32 value;
    do {
        value = XGpio_DiscreteRead(&gpio->device, channel);
	} while (!value);
	return value;
}

