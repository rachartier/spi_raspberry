#include "spi.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

void spi_test_message_str(struct spi_data *spi) {
    const struct spi_message message[] = {
#define CONSTRUCT_MSG_STR(str) \
        {str, strlen(str)}

        CONSTRUCT_MSG_STR("Hello World"),
        CONSTRUCT_MSG_STR("Hello World 2"),
        CONSTRUCT_MSG_STR("Hello World 3"),
        CONSTRUCT_MSG_STR("Hello World 4")
#undef CONSTRUCT_MSG_STR
    };

    for(int i = 0; i < 4; ++i) {
        spi_add_message(spi, message[i]);
    }

    uint8_t* data_read = spi_write_read(spi, 1);
    printf("string rx: %s\n", data_read);

    free(data_read);
}

void spi_test_message_byte(struct spi_data *spi) {
    uint8_t bytes[] = {65,66,67,68,69,70};

    const struct spi_message message[] = {
        {&bytes[0], sizeof(uint8_t)},
        {&bytes[1], sizeof(uint8_t)},
        {&bytes[2], sizeof(uint8_t)},
        {&bytes[3], sizeof(uint8_t)},
        {&bytes[4], sizeof(uint8_t)},
        {&bytes[5], sizeof(uint8_t)}
    };

    for(int i = 0; i < 6; ++i) {
        spi_add_message(spi, message[i]);
    }

    uint8_t* data_read = spi_write_read(spi, 1);

    printf("\nbytes rx:\n");
    for(int i = 0; i < 6; ++i) {
        printf("\t[%d] %d\n", i, data_read[i]);
    }

    free(data_read);
}

struct obj_test {int x,y,z; char name[32];};

void spi_test_message_obj(struct spi_data *spi) {
    struct obj_test obj_test = {
        .x = 10,
        .y = -5,
        .z = 20,
        .name = "cube"
    };

    const struct spi_message message = {
        &obj_test, 
        sizeof(struct obj_test)
    };

    spi_add_message(spi, message);

    uint8_t* data_read = spi_write_read(spi, 1);
    struct obj_test obj_received;

    memcpy(&obj_received, data_read, sizeof(struct obj_test));

    printf("\nobj rx: x:%d y:%d z:%d name:%s\n", 
        obj_received.x, 
        obj_received.y, 
        obj_received.z, 
        obj_received.name);

    free(data_read);
}

int main(void) {
    struct spi_data* spi = spi_open_port(SPI_CS0, 0, CLOCK_DIVIDER_FACTOR_16);

    spi_test_message_str(spi);
    spi_test_message_byte(spi);
    spi_test_message_obj(spi);

    spi_close_port(spi);

    return 0;
}