/*
 * project_specific.h
 *
 *  Created on: Dec 29, 2025
 *      Author: viorel_serbu
 */

#ifndef MAIN_PROJECT_SPECIFIC_H_
#define MAIN_PROJECT_SPECIFIC_H_


#define TEST_BUILD (0)
#if(TEST_BUILD == 1)
	#define WITH_CONSOLE
	#define TEST1
	#define CTRL_DEV_ID					(99)
	#define LOG_PORT_DEV				8081
	#define LOG_SERVER_DEV				"proxy.gnet"
#else
	#define CTRL_DEV_ID					(1)
#endif
#define WITH_CONSOLE
#define COMM_PROTO						MQTT_PROTO
#define OTA_SUPPORT

#define WIFI_STA_ON 					(1)
#define MQTT_PUBLISH					(1)
#define PD_USER							"ota"
#define DEV_NAME						"ESP32 OTA utility"
#define ACTIVE_CONTROLLER				OTA_CONTROLLER
#define PROMPT_STR 						"OTA"

#define CONFIG_STORE_HISTORY 1
#define CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH	1024

/*
Message definitions for device monitor queue
*/
#define MSG_WIFI			1	// wifi connect (.val = 1)/disconnect (.val = 0) event 
#define MSG_BAT				2	// battery level .val = ADC battery measurement * 1000
#define MSG_LED_FLASH		3	// nw state and remote state flashing
#define NW_STATE_CHANGE		4	// nw connected (.val = 1) / disconnected (.val = 0)
#define REMOTE_STATE_CHANGE	5	// remote connected (.val = 1) / disconnected (.val = 0)
#define INIT_COMPLETE		6	// init completed


#endif /* MAIN_PROJECT_SPECIFIC_H_ */
