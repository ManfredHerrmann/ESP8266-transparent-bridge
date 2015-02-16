#ifndef __CONFIG_H__
#define __CONFIG_H__

#undef CONFIG_STATIC
#define CONFIG_DYNAMIC
#define CONFIG_RESET

// override in this file
#include "config_wifi.h"

#ifdef CONFIG_STATIC

#ifndef STA_SSID
#define STA_SSID	"your-wifi-ssid"
#endif
#ifndef STA_PASSWORD
#define STA_PASSWORD	"password"
#endif
#ifndef AP_SSID
#define AP_SSID		"esp8266-9"
#endif
#ifndef AP_PASSWORD
#define AP_PASSWORD	"tr@nsp@r#nt"
#endif

void config_execute(void);

#endif

#ifdef CONFIG_DYNAMIC

typedef struct config_commands {
	char *command;
	void(*function)(serverConnData *conn, uint8_t argc, char *argv[]);
} config_commands_t;


void config_parse(serverConnData *conn, char *buf, int len);

#endif

#endif /* __CONFIG_H__ */
