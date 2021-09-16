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
	int r = 0;

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
		r = EX_USAGE;
		goto cleanup;
	}
	char *endptr;
	intmax_t num = strtoimax(out->tg_chat_raw, &endptr, 10);
	if(strcmp(endptr, "") || (num == INTMAX_MAX && errno == ERANGE))
	{
		fprintf(stderr, _("TG_CHAT is invalid\n"));
		r = EX_USAGE;
		goto cleanup;
	}
	if(num > INT64_MAX || num < INT64_MIN)
	{
		fprintf(stderr, _("TG_CHAT is invalid\n"));
		r = EX_USAGE;
		goto cleanup;
	}
	out->tg_chat = (int64_t)num;
	char *token = strtok(tg_admin_raw, ",");
	out->tg_admins_size = 0;
	out->tg_admins = NULL;
	while(token != NULL)
	{
		out->tg_admins_size ++;
		if(out->tg_admins == NULL)
		{
			out->tg_admins = calloc(out->tg_admins_size, sizeof(uint32_t));
			if(out->tg_admins == NULL)
			{
				r = errno;
				fprintf(stderr, _("Cannot allocate memory: %s.\n"), strerror(r));
				goto cleanup;
			}
		}
		else
		{
			if((out->tg_admins = realloc(out->tg_admins, out->tg_admins_size * sizeof(uint32_t))) == NULL)
			{
				r = errno;
				fprintf(stderr, _("Cannot allocate memory: %s.\n"), strerror(r));
				free(out->tg_admins);
				goto cleanup;
			}
		}
		uintmax_t unum = strtoumax(token, &endptr, 10);
		if(strcmp(endptr, "") || (unum == UINTMAX_MAX && errno == ERANGE))
		{
			fprintf(stderr, _("TG_ADMIN is invalid\n"));
			r = EX_USAGE;
			goto cleanup;
		}
		if(unum > UINT32_MAX || unum < 0)
		{
			fprintf(stderr, _("TG_ADMIN is invalid\n"));
			r = EX_USAGE;
			goto cleanup;
		}
		out->tg_admins[out->tg_admins_size - 1] = (uint32_t)unum;
		token = strtok(NULL, ",");
	}
cleanup:
	if(r)
	{
		environ_free(out);
	}
	return 0;
}

void environ_free(Environ *env)
{
	if(env->tg_admins != NULL)
	{
		free(env->tg_admins);
		env->tg_admins = NULL;
	}
}
