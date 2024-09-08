#include <stdio.h>
#include <string.h>

#include "pico/bootrom.h"
#include "pico/multicore.h"
#include "pico/mutex.h"
#include "pico/stdlib.h"
#include "tusb.h"

auto_init_mutex(mtx);

static int8_t gSumX = 0;
static int8_t gSumY = 0;
static bool gL = 0;
static bool gR = 0;

// sum with saturation
int8_t sumSat(int8_t a, int8_t b) {
  int16_t ret = (int16_t)a + (int16_t)b;
  if (ret < -128)
    ret = -128;
  if (ret > 127)
    ret = 127;
  return ret;
}

void mouse_cb(const int8_t o[4])
{
  mutex_enter_blocking(&mtx);
  gSumX = sumSat(gSumX, o[1]);
  gSumY = sumSat(gSumY, o[2]);
  gL = o[0] & 1;
  gR = o[0] & 2;
  mutex_exit(&mtx);
}

void core1_main() {
  sleep_ms(10);

  tusb_init();

  while (true) {
    tuh_task();
  }
}

#define GP_ATT 11
#define GP_CLK 12
#define GP_DAT 13
#define GP_CMD 14
#define GP_ACK 15

#define GP_LED 25

#define NO_ATT 0x100

static inline uint8_t noAtt(void) {
  if (gpio_get(GP_ATT)) {
    return 1;
  } else {
    return 0;
  }
}

static inline void setBus(uint gpio) {
  gpio_set_dir(gpio, GPIO_IN);
  gpio_set_mask(1 << gpio);
}

static inline void clrBus(uint gpio) {
  gpio_clr_mask(1 << gpio);
  gpio_set_dir(gpio, GPIO_OUT);
}

uint16_t readCmdWriteData(uint8_t data) {
  uint8_t ret = 0;
  for (int i = 0; i != 8; ++i) {
    while (gpio_get(GP_CLK)) // wait for 0
    {
      tight_loop_contents();
    }
    if (noAtt()) {
      setBus(GP_DAT);
      return NO_ATT;
    }

    if (data & (1 << i)) {
      setBus(GP_DAT);
    } else {
      clrBus(GP_DAT);
    }

    while (!gpio_get(GP_CLK)) // wait for 1
    {
      tight_loop_contents();
    }
    if (noAtt()) {
      setBus(GP_DAT);
      return NO_ATT;
    }
    ret |= gpio_get(GP_CMD) << i;
  }
  sleep_us(2);
  setBus(GP_DAT);
  return ret;
}

uint16_t readCmd(void) {
  uint8_t ret = 0;
  for (int i = 0; i != 8; ++i) {
    while (gpio_get(GP_CLK)) // wait for 0
    {
      tight_loop_contents();
    }
    if (noAtt()) {
      return NO_ATT;
    }

    while (!gpio_get(GP_CLK)) // wait for 1
    {
      tight_loop_contents();
    }
    if (noAtt()) {
      return NO_ATT;
    }

    ret |= gpio_get(GP_CMD) << i;
  }
  sleep_us(2);
  return ret;
}

void postAck(void) {
  sleep_us(11);
  clrBus(GP_ACK);
  sleep_us(3);
  setBus(GP_ACK);
}

void core0_main(void) {
  gpio_init(GP_ATT);
  gpio_set_dir(GP_ATT, GPIO_IN);

  gpio_init(GP_CLK);
  gpio_set_dir(GP_CLK, GPIO_IN);

  gpio_init(GP_DAT);
  gpio_set_slew_rate(GP_DAT, GPIO_SLEW_RATE_SLOW);
  gpio_set_dir(GP_DAT, GPIO_IN);
  gpio_clr_mask(1 << GP_DAT);

  gpio_init(GP_CMD);
  gpio_set_dir(GP_CMD, GPIO_IN);

  gpio_init(GP_ACK);
  gpio_set_slew_rate(GP_ACK, GPIO_SLEW_RATE_SLOW);
  gpio_set_dir(GP_ACK, GPIO_IN);
  gpio_clr_mask(1 << GP_ACK);

  gpio_init(GP_LED);
  gpio_set_slew_rate(GP_LED, GPIO_SLEW_RATE_SLOW);
  gpio_clr_mask(1 << GP_LED);
  gpio_set_dir(GP_LED, GPIO_OUT);

  bool updateLED = false;
  bool buttonL = false;
  bool buttonR = false;

  for (;;) {
    while (!noAtt()) // wait to finish current attention cycle
    {
      tight_loop_contents();
    }

    if (updateLED) {
      if (buttonL)
      {
        gpio_set_mask(1 << GP_LED);
      }
      else
      {
        gpio_clr_mask(1 << GP_LED);
      }
      updateLED = false;
    }

    while (noAtt()) // wait for new attention signal
    {
      tight_loop_contents();
    }

    if (readCmd() != 0x01) {
      continue;
    }
    postAck();

    if (readCmdWriteData(0x12) != 0x42) {
      continue;
    }
    postAck();

    if (readCmdWriteData(0x5A) == NO_ATT) {
      continue;
    }
    postAck();

    if (readCmdWriteData(0xFF) == NO_ATT) {
      continue;
    }
    postAck();

    mutex_enter_blocking(&mtx);
    int8_t sumX = gSumX;
    gSumX = 0;
    int8_t sumY = gSumY;
    gSumY = 0;
    buttonL = gL;
    buttonR = gR;
    mutex_exit(&mtx);

    uint8_t buttons1 = 3;
    if (buttonL) {
      buttons1 |= 8;
    }
    if (buttonR) {
      buttons1 |= 4;
    }

    if (readCmdWriteData(~buttons1) == NO_ATT) // buttons
    {
      continue;
    }
    postAck();

    if (readCmdWriteData(sumX) == NO_ATT) // dx
    {
      continue;
    }
    postAck();

    if (readCmdWriteData(sumY) == NO_ATT) // dy
    {
      continue;
    }
    // no ack here!
    updateLED = true;
  }
}

int main() {

  //stdio_usb_init();

  sleep_ms(10);

  multicore_reset_core1();
  // all USB tasks run in core1
  multicore_launch_core1(core1_main);

  core0_main();
}
