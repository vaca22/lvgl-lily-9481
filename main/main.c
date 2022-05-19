/* LVGL Example project */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"

// Littlevgl 头文件
#include "lvgl/lvgl.h"          // LVGL头文件
#include "lvgl_helpers.h"       // 助手 硬件驱动相关
#include "lv_examples/src/lv_demo_widgets/lv_demo_widgets.h"


#define TAG " LittlevGL Demo"
#define LV_TICK_PERIOD_MS 10
SemaphoreHandle_t xGuiSemaphore;		// 创建一个GUI信号量

static void lv_tick_task(void *arg);	// LVGL 时钟任务
void guiTask(void *pvParameter);		// GUI任务

// LVGL 时钟任务
static void lv_tick_task(void *arg) {
	(void) arg;
	lv_tick_inc(LV_TICK_PERIOD_MS);
}


void guiTask(void *pvParameter) {
	
	(void) pvParameter;
	xGuiSemaphore = xSemaphoreCreateMutex();	// 创建GUI信号量
	lv_init();									// 初始化LittlevGL
	lvgl_driver_init();							// 初始化液晶SPI驱动 触摸芯片SPI/IIC驱动

	// 初始化缓存
	static lv_color_t buf1[DISP_BUF_SIZE];
	static lv_color_t buf2[DISP_BUF_SIZE];
	static lv_disp_buf_t disp_buf;
	uint32_t size_in_px = DISP_BUF_SIZE;
	lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);

	// 添加并注册触摸驱动
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.flush_cb = disp_driver_flush;
	disp_drv.buffer = &disp_buf;
	lv_disp_drv_register(&disp_drv);

	// 添加并注册触摸驱动
	ESP_LOGI(TAG,"Add Register Touch Drv");
	lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.read_cb = touch_driver_read;
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	lv_indev_drv_register(&indev_drv);

	// 定期处理GUI回调
	const esp_timer_create_args_t periodic_timer_args = {
		.callback = &lv_tick_task,
		.name = "periodic_gui"
	};
	esp_timer_handle_t periodic_timer;
	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));




	///////////////////////////////////////////////////
	///////////////line 线控件/////////////////////////
	///////////////////////////////////////////////////
	// 先创建一个折线所有点坐标的二维数组
	static lv_point_t line_points[] = { {5, 5}, {70, 70}, {120, 10}, {180, 60}, {240, 10} };

	static lv_style_t style_line;											// 创建一个风格
	lv_style_init(&style_line);												// 初始化风格
	lv_style_set_line_width(&style_line, LV_STATE_DEFAULT, 8);				// 设置线宽度
	lv_style_set_line_color(&style_line, LV_STATE_DEFAULT, LV_COLOR_BLUE);	// 设置线颜色
	lv_style_set_line_rounded(&style_line, LV_STATE_DEFAULT, true);			// 设置线头为圆角

	lv_obj_t * line1 = lv_line_create(lv_scr_act(), NULL);					// 在屏幕上创建一个线控件
	lv_line_set_points(line1, line_points, 5);								// 设置线的途经点数组，数量
	lv_obj_add_style(line1, LV_LINE_PART_MAIN, &style_line);				// 应用上面创建的线的风格
	lv_obj_align(line1, NULL, LV_ALIGN_CENTER, 0, 0);						// 居中显示


//	lv_demo_widgets();
	
	while (1) {
		vTaskDelay(1);
		// 尝试锁定信号量，如果成功，调用处理LVGL任务
		if (xSemaphoreTake(xGuiSemaphore, (TickType_t)10) == pdTRUE) {
			lv_task_handler();					// 处理LVGL任务
			xSemaphoreGive(xGuiSemaphore);		// 释放信号量
		}
	}
	vTaskDelete(NULL);							// 删除任务
}

// 主函数
void app_main() {
	ESP_LOGI(TAG, "\r\nAPP is start!~\r\n");
	
	// 如果要使用任务创建图形，则需要创建固定任务,否则可能会出现诸如内存损坏等问题
	xTaskCreatePinnedToCore(guiTask, "gui", 4096*2, NULL, 0, NULL, 1);
}




