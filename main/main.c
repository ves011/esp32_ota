/* OTA example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "wear_levelling.h"
#include "esp_console.h"
#include "driver/gpio.h"
#include "errno.h"
#include "esp_spi_flash.h"
#include "esp_spiffs.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "mqtt_client.h"
#include "common_defines.h"
#include "external_defs.h"
#include "ntp_sync.h"
#include "cmd_system.h"
#include "mqtt_ctrl.h"
#include "utils.h"
#include "ota.h"
#include "cmd_wifi.h"
#include "tcp_log.h"

#if CONFIG_EXAMPLE_CONNECT_WIFI
#include "esp_wifi.h"
#endif

//int rw_params(int rw, int param_type, void * param_val);

#define TAG "OTA_FACT"
#define PROMPT_STR "OTA_FACT"
#define CONFIG_STORE_HISTORY 0
#define CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH	1024

#include "wifi_credentials.h"

int console_state;
int restart_in_progress;
int controller_op_registered;

esp_vfs_spiffs_conf_t conf_spiffs =
	{
	.base_path = BASE_PATH,
	.partition_label = PARTITION_LABEL,
	.max_files = 5,
	.format_if_mount_failed = true
	};


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

#if CONFIG_STORE_HISTORY
	#define MOUNT_PATH "/data"
	#define HISTORY_PATH MOUNT_PATH "/history.txt"
	static void initialize_filesystem(void)
		{
		static wl_handle_t wl_handle;
		const esp_vfs_fat_mount_config_t mount_config =
			{
			.max_files = 4,
			.format_if_mount_failed = true
			};
		esp_err_t err = esp_vfs_fat_spiflash_mount(MOUNT_PATH, "storage", &mount_config, &wl_handle);
		if (err != ESP_OK)
			{
			ESP_LOGE(TAG, "Failed to mount FATFS (%d/%s)", err, esp_err_to_name(err));
			return;
			}
		}
#endif // CONFIG_STORE_HISTORY


void app_main(void)
	{
	console_state = CONSOLE_OFF;
	setenv("TZ","EET-2EEST,M3.4.0/03,M10.4.0/04",1);
	esp_err_t ret = esp_vfs_spiffs_register(&conf_spiffs);
	if (ret != ESP_OK)
		{
        if (ret == ESP_FAIL)
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        else if (ret == ESP_ERR_NOT_FOUND)
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        else
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        esp_restart();
		}
	initialize_nvs();
	controller_op_registered = 0;
	rw_params(PARAM_READ, PARAM_CONSOLE, &console_state);
	wifi_join(DEFAULT_SSID, DEFAULT_PASS, JOIN_TIMEOUT_MS);
	tcp_log_init();
	esp_log_set_vprintf(my_log_vprintf);

	xTaskCreate(ntp_sync, "NTP_sync_task", 6134, NULL, USER_TASK_PRIORITY, &ntp_sync_task_handle);
	register_system();
	if(mqtt_start() == ESP_OK)
		register_mqtt();
#ifdef WITH_CONSOLE
	esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    repl_config.prompt = PROMPT_STR ">";
    repl_config.max_cmdline_length = CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH;


#if CONFIG_STORE_HISTORY
	initialize_filesystem();
	repl_config.history_save_path = HISTORY_PATH;
	ESP_LOGI(TAG, "Command history enabled");
#else
	ESP_LOGI(TAG, "Command history disabled");
#endif
#endif

	esp_console_register_help_command();
	register_system();
	register_wifi();
	controller_op_registered = 1;

#ifdef WITH_CONSOLE
#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
    esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    repl_config.task_stack_size = 8192;
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
   // ESP_LOGI(TAG, "console stack: %d", repl_config.task_stack_size);

#else
	#error Unsupported console type
#endif
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
#endif
	}

