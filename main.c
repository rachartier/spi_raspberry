#include "spi.h"

#include <stdio.h>
#include <stdint.h>

int main(void) {
    struct spi_data* spi = spi_open_port(SPI_CS0, 0, CLOCK_DIVIDER_FACTOR_16);

    char tx[] = "Hello World!";
    char rx[256];

    spi_write_read(spi, tx, rx, strlen(tx) + 1, 1);

    printf("rx: %s", rx);

    spi_close_port(spi);
    return 0;
}

