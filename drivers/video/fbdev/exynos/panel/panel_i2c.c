// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>

#include "panel_drv.h"
#include "panel_i2c.h"

#define PANEL_I2C_DRIVER_NAME "panel_i2c"

#define MAX_TX_SIZE 16
#define MAX_RX_SIZE 16

int panel_i2c_tx(struct panel_i2c_dev *i2c_dev, u8 *arr, u32 arr_len)
{
	int ret = 0;
	int tx_len = i2c_dev->tx_len;
	int tx_cnt = 0;
	int i = 0;
	u8 tx_buf[MAX_TX_SIZE] = {0, };

	struct i2c_msg msg[] = {
		{
			.addr = 0,
			.flags = 0,
			.len = tx_len,
			.buf = tx_buf,
		}
	};

	if (unlikely(!i2c_dev->client)) {
		pr_err("%s: failed, i2c Client is NULL.\n",__func__);
		ret = -EINVAL;
		return ret;
	}

	msg[0].addr = i2c_dev->client->addr;
	mutex_lock(&i2c_dev->lock);

	//pr_info("%s ++ [0]:0x%02x, len:%03d\n", __func__, arr[0], arr_len);

	if (!arr || tx_len <= 0 || tx_len > MAX_TX_SIZE || arr_len % tx_len) {
		pr_err("%s invalid tx length: %d\n", __func__, arr_len);
		ret = -EINVAL;
		goto tx_ret;
	}

	tx_cnt = arr_len / tx_len;

	for (i = 0; i < tx_cnt; i++) {
		memcpy(tx_buf, arr + (i * tx_len), tx_len);

		ret = i2c_transfer(i2c_dev->client->adapter, msg, 1);
		if (unlikely(ret != 1)) {
			ret = ret < 0 ? ret : -EIO;
			pr_err("%s: i2c tx failed. [0x%02x]... : [0x%02x]... error %d\n",
					__func__,  msg[0].buf[0], msg[0].buf[1], ret);
			goto tx_ret;
		}
	}

	ret = 0;
tx_ret:
	pr_info("%s --\n", __func__);
	mutex_unlock(&i2c_dev->lock);

	return ret;
}

int panel_i2c_rx(struct panel_i2c_dev *i2c_dev, u8 *arr, u32 arr_len)
{
	int tx_len = i2c_dev->tx_len;
	int rx_cnt = 0;
	int i = 0;
	int j = 0;
	int printout_pos = 0;
	int ret = 0;
	char printout_buf[128] = {0, };
	u8 addr_buf[MAX_TX_SIZE] = {0, };
	u8 rx_buf[MAX_RX_SIZE] = {0, };

	struct i2c_msg msg[] = {
		{
			.addr = 0,
			.flags = 0,
			.len = i2c_dev->addr_len,
			.buf = addr_buf,
		},
		{
			.addr = 0,
			.flags = I2C_M_RD,
			.len = i2c_dev->data_len,
			.buf = rx_buf,
		},
	};

	if (unlikely(!i2c_dev->client)) {
		pr_err("%s: failed, i2c Client is NULL.\n",__func__);
		ret = -EINVAL;
		return ret;
	}

	msg[0].addr = i2c_dev->client->addr;
	mutex_lock(&i2c_dev->lock);

	if (!arr || i2c_dev->addr_len <= 0 || i2c_dev->addr_len > MAX_TX_SIZE ||
			i2c_dev->data_len <= 0 || i2c_dev->data_len > MAX_RX_SIZE ||
			i2c_dev->tx_len <= 0 || arr_len % i2c_dev->tx_len) {
		pr_err("%s invalid rx length: %d\n", __func__, arr_len);
		ret = -EINVAL;
		goto rx_ret;
	}

	rx_cnt = arr_len / i2c_dev->tx_len;
	msg[1].addr = i2c_dev->client->addr;

	pr_info("========= %s =========\n", __func__);
	for (i = 0; i < rx_cnt; i++) {
		memcpy(addr_buf, arr + (i * tx_len), i2c_dev->addr_len);
		memset(printout_buf, 0x00, ARRAY_SIZE(printout_buf));
		printout_pos = 0;

		ret = i2c_transfer(i2c_dev->client->adapter, msg, 2);
		if (unlikely(ret != 2)) {
			ret = ret < 0 ? ret : -EIO;
			pr_err("%s: i2c rx failed at [%d][0x%02x] error %d\n",
					__func__,  i, msg[0].buf[0], ret);
			goto rx_ret;
		}

		if ((i2c_dev->addr_len + i2c_dev->data_len) * 6 + 2 <
				sizeof(printout_buf)) {
			for (j = 0; j < i2c_dev->addr_len; j++) {
				snprintf(printout_buf + printout_pos, 7, "[0x%02x]", addr_buf[j]);
				printout_pos += 6;
			}

			snprintf(printout_buf + printout_pos, 3, "=>");
			printout_pos += 2;

			for (j = 0; j < i2c_dev->data_len; j++) {
				snprintf(printout_buf + printout_pos, 7, "[0x%02x]", rx_buf[j]);
				printout_pos += 6;
			}
		}

		pr_info("%s\n", printout_buf);
	}

	ret = 0;
rx_ret:
	mutex_unlock(&i2c_dev->lock);

	return ret;
}

static struct i2c_drv_ops panel_i2c_drv_ops = {
	.tx = panel_i2c_tx,
	.rx = panel_i2c_rx,
};

static int panel_i2c_probe(struct i2c_client *client)
{
	const struct i2c_device_id *id = i2c_client_get_device_id(client);
	struct panel_device *panel;

	if (!id || !id->driver_data)
		return -ENODEV;

	panel = (struct panel_device *)id->driver_data;
	panel->panel_i2c_dev.client = client;
	i2c_set_clientdata(client, panel);

	dev_info(&client->dev, "%s: (%s)(%s)\n", __func__, dev_name(&client->adapter->dev),
						of_node_full_name(client->dev.of_node));

	return 0;
}

static void panel_i2c_remove(struct i2c_client *client)
{
	struct panel_device *panel = i2c_get_clientdata(client);

	if (panel)
		panel->panel_i2c_dev.client = NULL;
}

static struct i2c_device_id panel_i2c_id[] = {
	{"panel_i2c", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, panel_i2c_id);

static const struct of_device_id panel_i2c_dt_ids[] = {
	{ .compatible = "panel_i2c" },
	{ }
};

MODULE_DEVICE_TABLE(of, panel_i2c_dt_ids);

static struct i2c_driver panel_i2c_driver = {
	.driver = {
		.name	= PANEL_I2C_DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table	= of_match_ptr(panel_i2c_dt_ids),
	},
	.id_table = panel_i2c_id,
	.probe = panel_i2c_probe,
	.remove = panel_i2c_remove,
};

int panel_i2c_drv_probe(struct panel_device *panel, struct i2c_data *i2c_data)
{
	struct panel_i2c_dev *i2c_dev;
	int ret = 0;

	dev_info(panel->dev, "+ %s\n", __func__);

	if (!i2c_data || i2c_data->addr_len <= 0 ||
			i2c_data->addr_len > MAX_TX_SIZE || i2c_data->data_len <= 0 ||
			i2c_data->data_len > MAX_RX_SIZE) {
		dev_err(panel->dev, "invalid panel I2C lengths\n");
		return -EINVAL;
	}

	i2c_dev = &panel->panel_i2c_dev;
	i2c_dev->addr_len = i2c_data->addr_len;
	i2c_dev->data_len = i2c_data->data_len;
	i2c_dev->tx_len = i2c_data->addr_len + i2c_data->data_len;
	i2c_dev->ops = &panel_i2c_drv_ops;
	mutex_init(&i2c_dev->lock);

	panel_i2c_id->driver_data = (kernel_ulong_t)panel;
	ret = i2c_add_driver(&panel_i2c_driver);
	if (ret)
		panel_err("%s failed to register for i2c device\n", __func__);

	dev_info(panel->dev, "- %s\n", __func__);
	return ret;
}

MODULE_DESCRIPTION("i2c driver for panel");
MODULE_LICENSE("GPL");
