# esp32_ota

OTA application for FW upgarde, controlled via MQTT messages
MQTT broker URL is defined in mqtt_ctrl.c
```
#define CONFIG_BROKER_URL
```
esp32_ota subscribes to "ota[xx]/ctrl" topic where [xx] is the device number defined by
```
#define CTRTL_DEV_ID //in project_specific.h
```
It requires "factory" and "ota_0" partionons to be present in partition table<br>
To perform OTA uprade of the firmware in partition "ota_0":
1. ensure the device is booted in factory mode
2. publish "ota \<URL to new FW\>" message in "ota[xx]/ctrl" topic
3. wait for "Image validation OK" log message
4. publish "boot ota_0" in "ota[xx]/ctrl" topic (set boot to "ota_0" partition)
5. publish "restart" in "ota[xx]/ctrl" topic<br>

Now the device should boot the new FW from ota_0
