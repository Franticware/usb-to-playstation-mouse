/*
 * USB to PS1 Mouse for RPi Pico - Copyright (c) 2022 Vojtěch Salajka (franticware.com)
 *
 * Based on host_cdc_msc_hid example by Ha Thach (tinyusb.org)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/multicore.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"

#include "bsp/board.h"
#include "tusb.h"

#include "queue.h"

/*
 * Useful links:
 * https://gamesx.com/controldata/psxcont/psxcont.htm
 * https://problemkaputt.de/psx-spx.htm#controllerandmemorycardsignals
 * https://hackaday.io/project/170365-blueretro/log/186471-playstation-playstation-2-spi-interface
 * https://store.curiousinventor.com/guides/PS2/
 */

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+
extern void hid_app_task(void);

static uint32_t mouseRecv;
static int8_t mouseReport[4];

static int8_t sumX = 0;
static int8_t sumY = 0;

static uint8_t mButtons = 0;
static uint8_t mButtonsPrev = 0;

static QueueU8 leftStateQ = {0};
static QueueU8 rightStateQ = {0};

static QueueS8 deltaXQ = {0};
static QueueS8 deltaYQ = {0};

static uint8_t leftState = 0;
static uint8_t rightState = 0;

void core1_sio_irq() 
{
    // Just record the latest entry
    while (multicore_fifo_rvalid())
    {
      mouseRecv = multicore_fifo_pop_blocking();
      memcpy(mouseReport, &mouseRecv, 4);
      mButtons = mouseReport[0];
      QueuePut(deltaXQ, mouseReport[1]);
      QueuePut(deltaYQ, mouseReport[2]);
      if ((mButtons ^ mButtonsPrev) & MOUSE_BUTTON_LEFT)
      {
        QueuePut(leftStateQ, !!(mButtons & MOUSE_BUTTON_LEFT));
      }
      if ((mButtons ^ mButtonsPrev) & MOUSE_BUTTON_RIGHT)
      {
        QueuePut(rightStateQ, !!(mButtons & MOUSE_BUTTON_RIGHT));
      }
      mButtonsPrev = mButtons;
    }

    multicore_fifo_clear_irq();
}

/*
GP11 ATT
GP12 CLK
GP13 DAT out
GND  
GP14 CMD
GP15 ACK
*/

#define GP_ATT 11
#define GP_CLK 12
#define GP_DAT 13
#define GP_CMD 14
#define GP_ACK 15

#define NO_ATT 0x100

//#define gpio_set(a, b) do { if (b) gpio_set_mask((1<<a)); else gpio_clr_mask((1<<a)); } while (0)

uint16_t readCmdWriteData(uint8_t data)
{
  uint8_t ret = 0;
  for (int i = 0; i != 8; ++i)
  {
    while (gpio_get(GP_CLK)) // wait for 0
    {
      if (gpio_get(GP_ATT))
      { 
        return NO_ATT;
      }
    }
    gpio_set_dir(GP_DAT, (data & (1 << i)) ? GPIO_IN : GPIO_OUT);
    while (!gpio_get(GP_CLK)) // wait for 1
    {
      if (gpio_get(GP_ATT))
      { 
        return NO_ATT;
      }
    }
    ret |= gpio_get(GP_CMD) << i;
  }
  sleep_us(2);
  return ret;
}

uint16_t readCmd(void)
{
  uint8_t ret = 0;
  for (int i = 0; i != 8; ++i)
  {
    while (gpio_get(GP_CLK)) // wait for 0
    {
      if (gpio_get(GP_ATT))
      { 
        return NO_ATT;
      }
    }
    while (!gpio_get(GP_CLK)) // wait for 1
    {
      if (gpio_get(GP_ATT))
      { 
        return NO_ATT;
      }
    }
    ret |= gpio_get(GP_CMD) << i;
  }
  sleep_us(2);
  return ret;
}

void postAck(void)
{
  sleep_us(8);  
  gpio_set_dir(GP_ACK, GPIO_OUT);
  sleep_us(3);
  gpio_set_dir(GP_ACK, GPIO_IN);
  gpio_pull_up(GP_ACK);
}

// sum with saturation
int8_t sumSat(int8_t a, int8_t b)
{
    int32_t ret = (int32_t)a+(int32_t)b;
    if (ret < -128) ret = -128;
    if (ret > 127) ret = 127;
    return ret;
}

void core1_entry() {
    multicore_fifo_clear_irq();
    irq_set_exclusive_handler(SIO_IRQ_PROC1, core1_sio_irq);

    irq_set_enabled(SIO_IRQ_PROC1, true);

    gpio_init(GP_ATT);
    gpio_set_dir(GP_ATT, GPIO_IN);

    gpio_init(GP_CLK);
    gpio_set_dir(GP_CLK, GPIO_IN);

    gpio_init(GP_DAT);
    gpio_set_dir(GP_DAT, GPIO_IN);
    gpio_clr_mask((1<<GP_DAT));

    gpio_init(GP_CMD);
    gpio_set_dir(GP_CMD, GPIO_IN);

    gpio_init(GP_ACK);
    gpio_set_dir(GP_ACK, GPIO_IN);
    gpio_clr_mask((1<<GP_ACK));

    uint8_t buttons1 = 0;
    bool buttonsPending = false;

    while (1)
    {
      while (!gpio_get(GP_ATT)) // wait for 1
      {
        tight_loop_contents();
      }      
      
      while (gpio_get(GP_ATT)) // wait for 0
      {
        tight_loop_contents();
      }

      {
        if (!buttonsPending)
        {
          if (QueueNotEmpty(leftStateQ))
          {
            leftState = QueueGet(leftStateQ);
          }

          if (QueueNotEmpty(rightStateQ))
          {
            rightState = QueueGet(rightStateQ);
          }

          buttons1 = 3;
          
          if (leftState)
          {
            buttons1 |= 8;
            board_led_write(1);
          }
          else
          {
            board_led_write(0);
          }
          if (rightState)
          {
            buttons1 |= 4;
          }
          buttonsPending = true;
        }

        while (QueueNotEmpty(deltaXQ))
        {
          sumX = sumSat(sumX, QueueGet(deltaXQ));
        }
        while (QueueNotEmpty(deltaYQ))
        {
          sumY= sumSat(sumY, QueueGet(deltaYQ));
        }
      }

      if (readCmd() != 0x01)
      {
        continue;
      }
      postAck();

      if (readCmdWriteData(0x12) != 0x42)
      {
        continue;
      }
      postAck();

      if (readCmdWriteData(0x5A) == NO_ATT)
      {
        continue;
      }
      postAck();

      if (readCmdWriteData(0xFF) == NO_ATT)
      {
        continue;
      }
      postAck();

      if (readCmdWriteData(~buttons1) == NO_ATT)  // buttons
      {
        continue;
      }
      postAck();

      if (readCmdWriteData(sumX) == NO_ATT)  // dx
      {
        continue;
      }
      postAck();

      if (readCmdWriteData(sumY) == NO_ATT)  // dy
      {
        continue;
      }
      // no ack here!

      sumX = 0;
      sumY = 0;

      buttonsPending = false;
    }
}

/*------------- MAIN -------------*/
int main(void)
{
  board_init();

  printf("USB to PS1 mouse adapter\r\n");

  tusb_init();
  
  multicore_launch_core1(core1_entry);

  while (1)
  {
    // tinyusb host task
    tuh_task();
  }

  return 0;
}
