#include "spi.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static int errnospi = 0;

static uint32_t clock_speed(enum clock_divider clock_divider) {
    uint32_t freq_max = 250000000U;

    if(clock_divider == CLOCK_DIVIDER_FACTOR_1 || clock_divider == CLOCK_DIVIDER_FACTOR_65536)
        return freq_max;

    return (freq_max/clock_divider);
}

struct spi_data* spi_open_port(int spi_device, uint8_t mode, enum clock_divider clock_divider) {
    struct spi_data* spi_data = malloc(sizeof(struct spi_data));
    assert(spi_data != NULL);

    spi_data->mode = mode;
    spi_data->speed = clock_speed(clock_divider);
    spi_data->bpw = BITS_PER_WORD;

    spi_data->cs_fd = (spi_device)
        ? open("/dev/spidev0.1", O_RDWR)
        : open("/dev/spidev0.0", O_RDWR);

    if(spi_data->cs_fd < 0) {
        perror("Could not open SPI device");
        errnospi = STATUS_OPEN_SPI_DEVICE;
        free(spi_data);

        return NULL;
    }

    int status_value = 0;

#define SET_SPI_MODE(mode, val, errmsg, retcode)        \
    status_value = ioctl(spi_data->cs_fd, mode, val);   \
    if(status_value < 0) {                              \
        perror(errmsg);                                 \
        spi_close_port(spi_data);                       \
        errnospi = retcode;                             \
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


int spi_write_read(struct spi_data* spi_data, void* tx_data, void* rx_data, size_t length, int leave_cs_low) {
    assert(spi_data != NULL);

    struct spi_ioc_transfer spi = {
#ifdef __ENV_32
        .tx_buf = (uint32_t)tx_data,
        .rx_buf = (uint32_t)rx_data,
#else
        .tx_buf = (uint64_t)tx_data,
        .rx_buf = (uint64_t)rx_data,
#endif
        .len    = length,

        .delay_usecs   = 0,
        .speed_hz      = spi_data->speed,
        .bits_per_word = spi_data->bpw,
        .cs_change     = leave_cs_low
    };

    int status_value;

    status_value = ioctl(spi_data->cs_fd, SPI_IOC_MESSAGE(1), &spi);

    if(status_value < 0) {
        perror("Problem transmitting spi data");
        return SPI_PROBLEM_TRANSMITTING_DATA;
    }

    return status_value;
}
