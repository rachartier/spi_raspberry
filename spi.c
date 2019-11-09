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
    struct {
        int32_t cs0;
        int32_t cs1;
    } fd;

    uint32_t  speed;
    uint8_t   mode;
    uint8_t   bpw;
};

static struct spi_data* g_spi_data;

static uint32_t clock_speed(enum clock_divider clock_divider) {
    uint32_t freq_max = 125000000U;

    if(clock_divider == CLOCK_DIVIDER_FACTOR_1 || clock_divider == CLOCK_DIVIDER_FACTOR_65536)
        return freq_max;

    return (freq_max/clock_divider);
}

int spi_close_port(int spi_device) {
    assert(g_spi_data != NULL);

    int  status_value = -1;
    int* cs_fd;

    cs_fd = (spi_device)
        ? &g_spi_data->fd.cs1
        : &g_spi_data->fd.cs0;

    status_value = close(*cs_fd);

    if(status_value < 0) {
        perror("Could not close SPI device");
        return STATUS_CLOSE_SPI_DEVICE;
    }

    return status_value;
}

int spi_open_port(int spi_device, uint8_t mode, enum clock_divider clock_divider) {
    if(g_spi_data) {
        free(g_spi_data);
    }
    assert(g_spi_data == NULL);

    g_spi_data = malloc(sizeof(struct spi_data));
    assert(g_spi_data != NULL);

    int* cs_fd = NULL;

    g_spi_data->mode = mode;
    g_spi_data->speed = clock_speed(clock_divider);
    g_spi_data->bpw = BITS_PER_WORD;

    cs_fd = (spi_device)
        ? &g_spi_data->fd.cs1
        : &g_spi_data->fd.cs0;

    *cs_fd = (spi_device)
        ? open("/dev/spidev0.1", O_RDWR)
        : open("/dev/spidev0.0", O_RDWR);

    if(*cs_fd < 0) {
        perror("Could not open SPI device");
        return STATUS_OPEN_SPI_DEVICE;
    }

    int status_value = 0;

#define SET_SPI_MODE(mode, val, errmsg, retcode)   \
    status_value = ioctl(*cs_fd, mode, val);       \
    if(status_value < 0) {                         \
        perror(errmsg);                            \
        spi_close_port(spi_device);                \
        return retcode;                            \
    }                                              \

    SET_SPI_MODE(SPI_IOC_WR_MODE,          &g_spi_data->mode,  "Could not set SPI mode to WR",   STATUS_SET_MODE_WR)
        SET_SPI_MODE(SPI_IOC_RD_MODE,          &g_spi_data->mode,  "Could not set SPI mode to RD",   STATUS_SET_MODE_RD)
        SET_SPI_MODE(SPI_IOC_WR_BITS_PER_WORD, &g_spi_data->bpw,   "Could not set SPI bpw to WR",    STATUS_SET_BPW_WR)
        SET_SPI_MODE(SPI_IOC_RD_BITS_PER_WORD, &g_spi_data->bpw,   "Could not set SPI bpw to RD",    STATUS_SET_BPW_RD)
        SET_SPI_MODE(SPI_IOC_WR_MAX_SPEED_HZ,  &g_spi_data->speed, "Could not set SPI speed to WR", STATUS_SET_SPEED_WR)
        SET_SPI_MODE(SPI_IOC_RD_MAX_SPEED_HZ,  &g_spi_data->speed, "Could not set SPI speed to RD", STATUS_SET_SPEED_RD)

#undef SET_SPI_MODE

        return status_value;
}

int spi_write_read(int spi_device, void* tx_data, void* rx_data, size_t length, int leave_cs_low) {
    assert(g_spi_data != NULL);

    struct spi_ioc_transfer spi;

    int* cs_fd;
    int  status_value;

    cs_fd = (spi_device)
        ? &g_spi_data->fd.cs1
        : &g_spi_data->fd.cs0;


    memset(&spi, 0, sizeof(spi)); 

    spi.tx_buf = (uint64_t)tx_data;
    spi.rx_buf = (uint64_t)rx_data;
    spi.len    = length;

    spi.delay_usecs   = 0;
    spi.speed_hz      = g_spi_data->speed;
    spi.bits_per_word = g_spi_data->bpw;
    spi.cs_change     = leave_cs_low;

    status_value = ioctl(*cs_fd, SPI_IOC_MESSAGE(1), &spi);

    if(status_value < 0) {
        perror("Problem transmitting spi data");
        return SPI_PROBLEM_TRANSMITTING_DATA;
    }

    return status_value;
}

int main(void) {
    spi_open_port(SPI_CS0, 0, CLOCK_DIVIDER_FACTOR_4);

    char tx[] = "Hello World!";
    char rx[256];

    spi_write_read(SPI_CS0, tx, rx, strlen(tx) + 1, 1);

    printf("rx: %s", rx);

    spi_close_port(SPI_CS0);
}
