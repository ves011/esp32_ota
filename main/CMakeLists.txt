# Embed the server root certificate into the final binary
idf_build_get_property(project_dir PROJECT_DIR)
idf_component_register(
	SRCS 			"main.c"
					"../../esp32_common/cmds/cmd_system.c"
					"../../esp32_common/cmds/cmd_wifi.c"
					"../../esp32_common/tcp/ntp_sync.c"
					"../../esp32_common/tcp/mqtt_ctrl.c"
					"../../esp32_common/tcp/tcp_log.c"
					"../../esp32_common/utils/utils.c"
					"../../esp32_common/utils/ota.c"
    INCLUDE_DIRS 	"."
    				"../certs"
    				"../../esp32_common"
    				"../../esp32_common/cmds"
    				"../../esp32_common/tcp"
    				"../../esp32_common/utils"
    EMBED_TXTFILES	${project_dir}/certs/ca.crt
    EMBED_TXTFILES	${project_dir}/certs/client.crt
    EMBED_TXTFILES	${project_dir}/certs/client.key)
