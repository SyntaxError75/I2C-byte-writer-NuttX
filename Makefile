############################################################################
# nuttx-app-i2cbyte/Makefile
#
# SPDX-License-Identifier: Apache-2.0
############################################################################

include $(APPDIR)/Make.defs

# Информация о встроенном приложении
PROGNAME  = i2cbyte
PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = 2048
MODULE    = $(CONFIG_EXAMPLES_I2CBYTE)

# Главный файл с функцией main()
MAINSRC = i2cbyte_main.c

include $(APPDIR)/Application.mk
