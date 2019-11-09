#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BITS_PER_WORD 8

#define STATUS_OPEN_SPI_DEVICE        -1
#define STATUS_CLOSE_SPI_DEVICE       -2
#define STATUS_SET_MODE_WR            -3
#define STATUS_SET_MODE_RD            -4
#define STATUS_SET_BPW_WR             -5
#define STATUS_SET_BPW_RD             -6
#define STATUS_SET_SPEED_WR           -7
#define STATUS_SET_SPEED_RD           -8
#define SPI_PROBLEM_TRANSMITTING_DATA -9

#define SPI_CS0   0
#define SPI_CS1   1

static int spierror = 0;

enum clock_divider {
    CLOCK_DIVIDER_FACTOR_1 = 1 << 0,
    CLOCK_DIVIDER_FACTOR_2 = 1 << 1,
    CLOCK_DIVIDER_FACTOR_4 = 1 << 2,
    CLOCK_DIVIDER_FACTOR_8 = 1 << 3,
    CLOCK_DIVIDER_FACTOR_16 = 1 << 4,
    CLOCK_DIVIDER_FACTOR_32 = 1 << 5,
    CLOCK_DIVIDER_FACTOR_64 = 1 << 6,
    CLOCK_DIVIDER_FACTOR_128 = 1 << 7,
    CLOCK_DIVIDER_FACTOR_256 = 1 << 8,
    CLOCK_DIVIDER_FACTOR_512 = 1 << 9,
    CLOCK_DIVIDER_FACTOR_1024 = 1 << 10,
    CLOCK_DIVIDER_FACTOR_2048 = 1 << 11,
    CLOCK_DIVIDER_FACTOR_4096 = 1 << 12,
    CLOCK_DIVIDER_FACTOR_8192 = 1 << 13,
    CLOCK_DIVIDER_FACTOR_16384 = 1 << 14,
    CLOCK_DIVIDER_FACTOR_32768 = 1 << 15,
    CLOCK_DIVIDER_FACTOR_65536 = 0
};

struct spi_data {
    int cs_fd;

    uint32_t  speed;
    uint8_t   mode;
    uint8_t   bpw;
};

static uint32_t clock_speed(enum clock_divider clock_divider) {
    uint32_t freq_max = 250000000U;

    if(clock_divider == CLOCK_DIVIDER_FACTOR_1 || clock_divider == CLOCK_DIVIDER_FACTOR_65536)
        return freq_max;

    return (freq_max/clock_divider);
}

int spi_close_port(struct spi_data* spi_data) {
    assert(spi_data != NULL);

    int  status_value = -1;

    status_value = close(spi_data->cs_fd);

    if(status_value < 0) {
        perror("Could not close SPI device");
        return STATUS_CLOSE_SPI_DEVICE;
    }

    free(spi_data);

    return status_value;
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
        spierror = STATUS_OPEN_SPI_DEVICE;
        free(spi_data);

        return NULL;
    }

    int status_value = 0;

#define SET_SPI_MODE(mode, val, errmsg, retcode)        \
    status_value = ioctl(spi_data->cs_fd, mode, val);   \
    if(status_value < 0) {                              \
        perror(errmsg);                                 \
        spi_close_port(spi_data);                       \
        spierror = retcode;                             \
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

int spi_write_read(struct spi_data* spi_data, void* tx_data, void* rx_data, size_t length, int leave_cs_low) {
    assert(spi_data != NULL);

    struct spi_ioc_transfer spi = {
        .tx_buf = (uint64_t)tx_data,
        .rx_buf = (uint64_t)rx_data,
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

int main(void) {
    struct spi_data* spi = spi_open_port(SPI_CS0, 0, CLOCK_DIVIDER_FACTOR_16);

    char tx[] = "Hello World!";
    char rx[256];

    spi_write_read(spi, tx, rx, strlen(tx) + 1, 1);

    printf("rx: %s", rx);

    spi_close_port(spi);
    return 0;
}

