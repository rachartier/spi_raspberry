#include "spi.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline uint32_t _clock_speed(enum clock_divider clock_divider) {
    uint32_t freq_max = 250000000U;

    if(clock_divider == CLOCK_DIVIDER_FACTOR_1 || clock_divider == CLOCK_DIVIDER_FACTOR_65536)
        return freq_max;

    return (freq_max/clock_divider);
}

struct spi_data* spi_open_port(int spi_device, uint8_t mode, enum clock_divider clock_divider) {
    return spi_open_port_fixed_clock(spi_device, mode, _clock_speed(clock_divider));
}

struct spi_data* spi_open_port_fixed_clock(int spi_device, uint8_t mode, uint32_t clockspeed) {
    struct spi_data* spi_data = malloc(sizeof(struct spi_data));
    assert(spi_data != NULL);

    spi_data->mode = mode;
    spi_data->speed = clockspeed;
    spi_data->bpw = BITS_PER_WORD;

    spi_data->cs_fd = (spi_device)
        ? open("/dev/spidev0.1", O_RDWR)
        : open("/dev/spidev0.0", O_RDWR);

    if(spi_data->cs_fd < 0) {
        perror("Could not open SPI device");
        free(spi_data);

        return NULL;
    }

#define SET_SPI_MODE(mode, val, errmsg, retcode)        \
    if(ioctl(spi_data->cs_fd, mode, val) < 0) {         \
        perror(errmsg);                                 \
        spi_close_port(spi_data);                       \
        return NULL;                                    \
    }                                                   \

    SET_SPI_MODE(SPI_IOC_WR_MODE,          &spi_data->mode,  "Could not set SPI mode to WR",  STATUS_SET_MODE_WR)
    SET_SPI_MODE(SPI_IOC_RD_MODE,          &spi_data->mode,  "Could not set SPI mode to RD",  STATUS_SET_MODE_RD)
    SET_SPI_MODE(SPI_IOC_WR_BITS_PER_WORD, &spi_data->bpw,   "Could not set SPI bpw to WR",   STATUS_SET_BPW_WR)
    SET_SPI_MODE(SPI_IOC_RD_BITS_PER_WORD, &spi_data->bpw,   "Could not set SPI bpw to RD",   STATUS_SET_BPW_RD)
    SET_SPI_MODE(SPI_IOC_WR_MAX_SPEED_HZ,  &spi_data->speed, "Could not set SPI speed to WR", STATUS_SET_SPEED_WR)
    SET_SPI_MODE(SPI_IOC_RD_MAX_SPEED_HZ,  &spi_data->speed, "Could not set SPI speed to RD", STATUS_SET_SPEED_RD)

#undef SET_SPI_MODE

        return spi_data;
}

int spi_close_port(struct spi_data* spi_data) {
    assert(spi_data != NULL);

    int  status_value;

    status_value = close(spi_data->cs_fd);

    if(status_value < 0) {
        perror("Could not close SPI device");
        return STATUS_CLOSE_SPI_DEVICE;
    }

    free(spi_data);

    return status_value;
}

inline void spi_add_message(struct spi_data *spi, const struct spi_message message) {
    spi->message[spi->_nqueue_messages] = message;

    spi->_nqueue_messages++;
    assert(spi->_nqueue_messages < MAX_MESSAGES);
}

/*
static uint8_t* _alloc_rx(struct spi_data* spi_data) {
    uint32_t number_messages = spi_data->_nqueue_messages;
    uint32_t total_msg_size = 0U;

    for(uint32_t i = 0; i < number_messages; ++i) {
        total_msg_size += spi_data->message[i].size;
    }

    uint8_t *rx = malloc(total_msg_size * sizeof(*rx));
    assert(rx != NULL);

    return rx;
}
*/

void* spi_write_read(struct spi_data* spi_data, size_t rx_buf_size, int leave_cs_low) {
    assert(spi_data != NULL);

    uint32_t number_messages = spi_data->_nqueue_messages;
    uint32_t total_msg_sent_size = 0U;

    void* rx_data = malloc(rx_buf_size);//_alloc_rx(spi_data);
    assert(rx_data != NULL);

    struct spi_ioc_transfer spi[number_messages];

    for(uint32_t i = 0; i < number_messages; ++i) {
        struct spi_message* message = &spi_data->message[i];

        struct spi_ioc_transfer data = {
#ifdef __ENV_32
            .tx_buf = (uint32_t)message->data,
            .rx_buf = (uint32_t)(rx_data + total_msg_sent_size),
#else
            .tx_buf = (uint64_t)message->data,
            .rx_buf = (uint64_t)(rx_data + total_msg_sent_size),
#endif
            .len    = message->size,

            .delay_usecs   = 0,
            .speed_hz      = spi_data->speed,
            .bits_per_word = spi_data->bpw,
            .cs_change     = leave_cs_low
        };

        spi[i] = data;
        total_msg_sent_size += message->size; 
    }

    int status_value = ioctl(spi_data->cs_fd, SPI_IOC_MESSAGE(number_messages), &spi);
    if(status_value < 0) {
        perror("Problem transmitting spi data");
        return NULL;
    }

    spi_data->_nqueue_messages = 0U;

    return rx_data;
}

void* spi_write_read_string(struct spi_data* spi_data, const uint8_t* str, size_t chars, size_t rx_buf_size, int leave_cs_low) {
    assert(spi_data != NULL);

    void* rx_data = malloc(rx_buf_size);
    assert(rx_data != NULL);

    struct spi_ioc_transfer data = {
#ifdef __ENV_32
        .tx_buf = (uint32_t)str,
        .rx_buf = (uint32_t)(rx_data),
#else
        .tx_buf = (uint64_t)str,
        .rx_buf = (uint64_t)rx_data,
#endif
        .len    = chars,

        .delay_usecs   = 0,
        .speed_hz      = spi_data->speed,
        .bits_per_word = spi_data->bpw,
        .cs_change     = leave_cs_low
    };

    int status_value = ioctl(spi_data->cs_fd, SPI_IOC_MESSAGE(1), &data);
    if(status_value < 0) {
        perror("Problem transmitting spi data");
        return NULL;
    }

    return rx_data;
}
