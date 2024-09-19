/* OTA example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_netif.h"
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
#include "spi_flash_mmap.h"
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

#define TAG "OTA_FACT"
#define PROMPT_STR "OTA_FACT"
#define CONFIG_STORE_HISTORY 1
#define CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH	1024

#include "wifi_credentials.h"

console_state_t console_state;
int restart_in_progress;
int controller_op_registered;


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
	console_state = CONSOLE_OFF;
	setenv("TZ","EET-2EEST,M3.4.0/03,M10.4.0/04",1);
	spiffs_storage_check();
	initialize_nvs();
	controller_op_registered = 0;
	tsync = 0;
	wifi_join(DEFAULT_SSID, DEFAULT_PASS, JOIN_TIMEOUT_MS);
	rw_params(PARAM_READ, PARAM_CONSOLE, &console_state);
	tcp_log_evt_queue = NULL;
	tcp_log_init();
	esp_log_set_vprintf(my_log_vprintf);
	sync_NTP_time();

	if(mqtt_start() != ESP_OK)
		esp_restart();
#ifdef WITH_CONSOLE
	esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    repl_config.prompt = PROMPT_STR ">";
    repl_config.max_cmdline_length = CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH;


#if CONFIG_STORE_HISTORY
	//initialize_filesystem();
	repl_config.history_save_path = BASE_PATH HISTORY_FILE;
	ESP_LOGI(TAG, "Command history enabled");
#else
	repl_config.history_save_path = NULL;
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
    repl_config.task_stack_size = 8192;
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

