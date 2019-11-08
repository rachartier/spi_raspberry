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

struct spi_data {
  struct {
    int32_t cs0;
    int32_t cs1;
  } fd;

  uint32_t   speed;
  uint8_t   mode;
  uint8_t   bpw;
};

static struct spi_data* g_spi_data;

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

int spi_open_port(int spi_device, uint8_t mode, uint32_t speed) {
  if(g_spi_data) {
    free(g_spi_data);
  }
  assert(g_spi_data == NULL);

  g_spi_data = malloc(sizeof(struct spi_data));
  assert(g_spi_data != NULL);

  int* cs_fd = NULL;

  g_spi_data->mode = mode;
  g_spi_data->speed = speed;
  g_spi_data->bpw = BITS_PER_WORD;

  cs_fd = (spi_device)
    ? &g_spi_data->fd.cs1
    : &g_spi_data->fd.cs0;

  *cs_fd = (spi_device)
    ? open("/dev/spidev0.1", O_RDWR)
    : open("/dev/spidev0.0", O_RDWR);

  if(*cs_fd < 0) {
    perror("Could not open SPI device.");
    return STATUS_OPEN_SPI_DEVICE;
  }

  int status_value = 0;

#define SET_SPI_MODE(mode, val, errmsg, retcode)   \
  status_value = ioctl(*cs_fd, mode, val);         \
  if(status_value < 0) {                           \
    perror(errmsg);                                \
    spi_close_port(spi_device);                    \
    return retcode;                                \
  }                                                \

  SET_SPI_MODE(SPI_IOC_WR_MODE,          &g_spi_data->mode,  "Could not set SPI mode to WR",   STATUS_SET_MODE_WR);
  SET_SPI_MODE(SPI_IOC_RD_MODE,          &g_spi_data->mode,  "Could not set SPI mode to RD",   STATUS_SET_MODE_RD);
  SET_SPI_MODE(SPI_IOC_WR_BITS_PER_WORD, &g_spi_data->bpw,   "Could not set SPI bpw to WR",    STATUS_SET_BPW_WR);
  SET_SPI_MODE(SPI_IOC_RD_BITS_PER_WORD, &g_spi_data->bpw,   "Could not set SPI bpw to RD",    STATUS_SET_BPW_RD);
  SET_SPI_MODE(SPI_IOC_WR_MAX_SPEED_HZ,  &g_spi_data->speed, "Could not set SPI speed to WR", STATUS_SET_SPEED_WR);
  SET_SPI_MODE(SPI_IOC_RD_MAX_SPEED_HZ,  &g_spi_data->speed, "Could not set SPI speed to RD", STATUS_SET_SPEED_RD);

#undef SET_SPI_MODE

  return status_value;
}

int spi_write_read(int spi_device, uint8_t* tx_data, uint8_t* rx_data, size_t length, int leave_cs_low) {
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
  spi_open_port(SPI_CS0, SPI_MODE_0, 1000000);

  uint8_t* rx = malloc(100);
  uint8_t  tx[] = {64,65,66,67,68};

  spi_write_read(SPI_CS0, tx, rx, 2, 1);

  printf("rx: %s\n", rx);

  spi_close_port(SPI_CS0);
  free(rx);

  return 0;
}

