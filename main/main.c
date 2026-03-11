/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_system.h"
//#include "esp_spi_flash.h"
#include "spi_flash_mmap.h"
#include "esp_spiffs.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "esp_console.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "wear_levelling.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "linenoise/linenoise.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include "esp_pm.h"
#include "driver/i2c_master.h"
#include "project_specific.h"
#include "common_defines.h"
#include "cmd_wifi.h"
#include "cmd_system.h"
#include "utils.h"
#include "tcp_log.h"
#include "mqtt_client.h"
#include "mqtt_ctrl.h"
#include "ntp_sync.h"
#include "esp_ota_ops.h"

#include "external_defs.h"
#include "wifi_credentials.h"

console_state_t console_state;
int restart_in_progress;
int controller_op_registered;
QueueHandle_t dev_mon_queue = NULL;
#define TAG "OTA"


static void initialize_nvs(void)
	{
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
		{
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
		}
	ESP_ERROR_CHECK(err);
	}


void app_main(void)
	{
	gpio_install_isr_service(0);
#if 0
	int bp_ctrl = GPIO_BOOT_CONTROL
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << bp_ctrl);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    /*
     * if BOOT_CTRL_PIN is 0 at boot restart with esp32_ota
     */
    if(gpio_get_level(bp_ctrl) == 0)
    	{
    	const esp_partition_t *sbp = NULL;;
    	esp_partition_iterator_t pit = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
    	while(pit)
    		{
    		sbp = esp_partition_get(pit);
    		if(sbp)
    			{
   				if(!strcmp(sbp->label, OTA_PART_NAME))
    				break;
    			}
    		pit = esp_partition_next(pit);
    		}
    	if(sbp)
			{
			int err = esp_ota_set_boot_partition(sbp);
			if(err == ESP_OK)
				esp_restart();
    		}
    	}
	gpio_reset_pin(bp_ctrl);
#endif
	restart_in_progress = 0;
	console_state = CONSOLE_OFF;
	setenv("TZ","EET-2EEST,M3.4.0/03,M10.4.0/04",1);
	ESP_LOGI(TAG, "main 1");
	spiffs_storage_check();
	initialize_nvs();
	rw_dev_config(PARAM_READ);
	controller_op_registered = 0;

	//tsync = 0;
	wifi_join(DEFAULT_SSID, DEFAULT_PASS, JOIN_TIMEOUT_MS);
	//if(rw_console_state(PARAM_READ, &console_state) == ESP_FAIL)
	//	console_state = CONSOLE_ON;
	esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
	//tcp_log_task_handle = NULL;
    //tcp_log_evt_queue = NULL;
	sync_NTP_time();

	if(mqtt_start() != ESP_OK)
		esp_restart();
	
	tcp_log_init();
	esp_log_set_vprintf(my_log_vprintf);


	/* Register commands */
	esp_console_register_help_command();
	register_system();
	register_wifi();

controller_op_registered = 1;

#ifdef WITH_CONSOLE
	esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
	repl_config.prompt = PROMPT_STR ">";
    repl_config.max_cmdline_length = CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH;
	#if CONFIG_STORE_HISTORY
		//initialize_filesystem();
		repl_config.history_save_path = BASE_PATH HISTORY_FILE;
		ESP_LOGI(TAG, "Command history enabled");
	#else
		ESP_LOGI(TAG, "Command history disabled");
	#endif
	repl_config.task_stack_size = 8192;
#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
    esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
    //ESP_LOGI(TAG, "console stack: %d", repl_config.task_stack_size);

#else
	#error Unsupported console type
#endif

	ESP_ERROR_CHECK(esp_console_start_repl(repl));
#endif
	}
