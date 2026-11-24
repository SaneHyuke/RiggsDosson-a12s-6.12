# SPDX-License-Identifier: GPL-2.0-only
obj-$(CONFIG_EXYNOS_DPU30) += dpu.o
dpu-y += decon_core.o decon_dsi.o decon_wb.o helper.o win_update.o cursor.o fence.o
dpu-y += event_log.o format.o dpp_drv.o dsim_drv.o
dpu-$(CONFIG_EXYNOS_DPU30_3830) += cal_3830/dsim_reg.o cal_3830/dpp_reg.o cal_3830/decon_reg.o
dpu-$(CONFIG_EXYNOS_COMMON_PANEL) += panels/common_panel_drv.o panels/common_mipi_lcd.o
