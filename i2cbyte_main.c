/****************************************************************************
 * apps/examples/i2cbyte/i2cbyte_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2C Byte Writer — NSH built-in command.
 *
 * Usage: i2cbyte <hex-byte>
 *   Sends one byte (0x00..0xFF) to I2C slave address 0x38 on /dev/i2c0.
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nuttx/i2c/i2c_master.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define I2CBYTE_DEVPATH   "/dev/i2c0"
#define I2CBYTE_SLVADDR   0x38
#define I2CBYTE_FREQUENCY 100000   /* 100 kHz — безопасное значение по умолчанию */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i2cbyte_parse_hex
 *
 * Description:
 *   Parse a string in the form "0x00" .. "0xFF" (or "00" .. "FF") into
 *   a byte value.
 *
 * Returned Value:
 *   0 on success, negative errno on failure.
 *
 ****************************************************************************/

static int i2cbyte_parse_hex(FAR const char *arg, FAR uint8_t *out)
{
  FAR char *endptr;
  unsigned long value;

  /* Проверяем, что аргумент не NULL и не пустой */

  if (arg == NULL || *arg == '\0')
    {
      return -EINVAL;
    }

  /* Пропускаем необязательный префикс "0x" / "0X" */

  if (arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X'))
    {
      arg += 2;
    }

  /* Отклоняем пустую строку после префикса, а также знаки + и - */

  if (*arg == '\0' || *arg == '+' || *arg == '-')
    {
      return -EINVAL;
    }

  /* Отклоняем аргументы длиннее двух шестнадцатеричных цифр */

  if (strlen(arg) > 2)
    {
      return -EINVAL;
    }

  errno  = 0;
  value  = strtoul(arg, &endptr, 16);

  /* Проверяем ошибки парсинга и наличие лишних символов */

  if (errno != 0 || *endptr != '\0')
    {
      return -EINVAL;
    }

  /* Проверка диапазона: 0x00 .. 0xFF */

  if (value > 0xFF)
    {
      return -ERANGE;
    }

  *out = (uint8_t)value;
  return OK;
}

/****************************************************************************
 * Name: i2cbyte_write
 *
 * Description:
 *   Open /dev/i2c0, configure the bus frequency, and transfer one byte
 *   to slave address 0x38.
 *
 * Returned Value:
 *   0 on success, negative errno on failure.
 *
 ****************************************************************************/

static int i2cbyte_write(uint8_t byteval)
{
  struct i2c_config_s config;
  struct i2c_msg_s     msg;
  uint8_t              txbuf[1];
  int                  fd;
  int                  ret;

  /* Открываем символьное устройство I2C */

  fd = open(I2CBYTE_DEVPATH, O_WRONLY);
  if (fd < 0)
    {
      int errcode = errno;
      fprintf(stderr,
              "ERROR: Failed to open %s: %d (%s)\n",
              I2CBYTE_DEVPATH, errcode, strerror(errcode));
      return -errcode;
    }

  /* Настраиваем параметры шины I2C */

  config.frequency = I2CBYTE_FREQUENCY;
  config.address   = I2CBYTE_SLVADDR;
  config.addrlen   = 7;   /* 7-битный адрес устройства */

  /* Формируем сообщение для передачи одного байта данных */

  txbuf[0]         = byteval;
  msg.frequency    = config.frequency;
  msg.addr         = config.address;
  msg.flags        = 0;   /* Запись, с формированием STOP */
  msg.buffer       = txbuf;
  msg.length       = 1;

  /* Выполняем передачу по I2C */

  ret = ioctl(fd, I2CIOC_TRANSFER, (unsigned long)&msg);
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr,
              "ERROR: I2C transfer to 0x%02X failed: %d (%s)\n",
              I2CBYTE_SLVADDR, errcode, strerror(errcode));
      close(fd);
      return -errcode;
    }

  /* Закрываем устройство */

  close(fd);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i2cbyte_main
 *
 * Description:
 *   NSH built-in entry point.
 *
 *   argv[0] = "i2cbyte"
 *   argv[1] = hex byte value (e.g. "0x3A" or "3A")
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  uint8_t byteval;
  int     ret;

  printf("i2cbyte: sending byte to I2C address 0x%02X on %s\n",
         I2CBYTE_SLVADDR, I2CBYTE_DEVPATH);

  /* Ожидается ровно один аргумент (не считая argv[0]) */

  if (argc != 2)
    {
      fprintf(stderr, "Usage: %s <hex-byte>\n", argv[0]);
      fprintf(stderr, "  <hex-byte> must be 0x00 .. 0xFF (e.g. 0x3A or 3A)\n");
      return EXIT_FAILURE;
    }

  /* Разбираем аргумент */

  ret = i2cbyte_parse_hex(argv[1], &byteval);
  if (ret < 0)
    {
      fprintf(stderr,
              "ERROR: Invalid hex byte '%s' (expected 0x00 .. 0xFF)\n",
              argv[1]);
      return EXIT_FAILURE;
    }

  printf("i2cbyte: parsed value = 0x%02X (%u)\n", byteval, byteval);

  /* Выполняем запись по I2C */

  ret = i2cbyte_write(byteval);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Write to I2C failed\n");
      return EXIT_FAILURE;
    }

  /* Сообщение об успехе */

  printf("i2cbyte: SUCCESS — 0x%02X sent to slave 0x%02X on %s\n",
         byteval, I2CBYTE_SLVADDR, I2CBYTE_DEVPATH);

  return EXIT_SUCCESS;
}
