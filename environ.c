#include "environ.h"
#include "common.h"

#include <sysexits.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>

int environ_read(Environ *out)
{
	out->tg_api = getenv("TG_API");
	out->tg_chat_raw = getenv("TG_CHAT");
	char *tg_admin_raw = getenv("TG_ADMIN");
	out->tg_link_fmt = getenv("TG_LINK_FMT");
	out->rcon_host = getenv("RCON_HOST");
	out->rcon_port = getenv("RCON_PORT");
	out->rcon_passwd = getenv("RCON_PASSWD");
	if(out->tg_api == NULL ||
			tg_admin_raw == NULL ||
			out->tg_chat_raw == NULL ||
			out->tg_link_fmt == NULL ||
			out->rcon_host == NULL ||
			out->rcon_port == NULL)
	{
		fprintf(stderr, _("Required environment variables are missing.\n"));
		return EX_USAGE;
	}
	char *endptr;
	intmax_t num = strtoimax(out->tg_chat_raw, &endptr, 10);
	if(strcmp(endptr, "") || (num == INTMAX_MAX && errno == ERANGE))
	{
		fprintf(stderr, _("TG_CHAT is invalid\n"));
		return EX_USAGE;
	}
	if(num > INT64_MAX || num < INT64_MIN)
	{
		fprintf(stderr, _("TG_CHAT is invalid\n"));
		return EX_USAGE;
	}
	out->tg_chat = (int64_t)num;
	uintmax_t unum = strtoumax(tg_admin_raw, &endptr, 10);
	if(strcmp(endptr, "") || (unum == UINTMAX_MAX && errno == ERANGE))
	{
		fprintf(stderr, _("TG_ADMIN is invalid\n"));
		return EX_USAGE;
	}
	if(unum > UINT32_MAX || unum < 0)
	{
		fprintf(stderr, _("TG_ADMIN is invalid\n"));
		return EX_USAGE;
	}
	out->tg_admin = (uint32_t)unum;
	return 0;
}
