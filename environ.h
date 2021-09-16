#ifndef _ENVIRON_H
#define _ENVIRON_H

#include <stdint.h>

typedef struct environ {
	char *tg_api;
	char *tg_chat_raw;
	int64_t tg_chat;
	unsigned int tg_admins_size;
	uint32_t *tg_admins;
	char *tg_link_fmt;
	char *rcon_host;
	char *rcon_port;
	char *rcon_passwd;
	int *rcon_sd;
} Environ;

int environ_read(Environ *out);
void environ_free(Environ *env);

#endif // _ENVIRON_H
