/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _SEC_INPUT_H_
#define _SEC_INPUT_H_

#include <linux/device.h>

#define input_dbg(_mode, dev, fmt, ...)	dev_dbg(dev, "[sec_input] " fmt, ##__VA_ARGS__)
#define input_info(_mode, dev, fmt, ...)	dev_info(dev, "[sec_input] " fmt, ##__VA_ARGS__)
#define input_err(_mode, dev, fmt, ...)	dev_err(dev, "[sec_input] " fmt, ##__VA_ARGS__)
#define input_raw_info(mode, dev, fmt, ...)	input_info(mode, dev, fmt, ##__VA_ARGS__)
#define input_log_fix()			do { } while (0)
#define input_raw_data_clear()		do { } while (0)

#endif
