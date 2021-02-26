#ifndef _ENVIRON_H
#define _ENVIRON_H

#include <stdint.h>

typedef struct environ {
	char *tg_api;
	char *tg_chat_raw;
	int64_t tg_chat;
	uint32_t tg_admin;
	char *tg_link_fmt;
	char *rcon_host;
	char *rcon_port;
	char *rcon_passwd;
	int *rcon_sd;
} Environ;

int environ_read(Environ *out);

#endif // _ENVIRON_H
