#ifndef __SPI_H__
#define __SPI_H__ 

#include <stdint.h>
#include <string.h>

#if __GNUC__
# if __ARM_ARCH_6__ 
#  define __ENV_32 
# else
#  define __ENV_64
# endif
#endif 

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

#define MAX_MESSAGES 16

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

struct spi_message {
    void*  data;
    size_t size;
};

struct spi_data {
    struct spi_message message[MAX_MESSAGES];

    uint32_t  speed;
    uint8_t   mode;
    uint8_t   bpw;
    int8_t    cs_fd;

    int32_t   _nqueue_messages;
};

struct spi_data* spi_open_port(int spi_device, uint8_t mode, enum clock_divider clock_divider);

void spi_add_message(struct spi_data *spi, const struct spi_message message);

int spi_close_port(struct spi_data* spi_data);
uint8_t* spi_write_read(struct spi_data* spi_data, int leave_cs_low);

#endif
