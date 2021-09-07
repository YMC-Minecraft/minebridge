#include "mc_regex.h"
#include "mcin.h"
#include "../common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int mcin_load(char str[], regex_t *arr)
{
	int r = 0;
	int line = 0;
	char *token;
	char *rest = str;
	while ((token = strtok_r(rest, ",", &rest)))
	{
		int l = line++;
		regex_t *reg = &arr[l];
		r = regcomp(reg, token, REG_EXTENDED);
		if(r)
		{
			fprintf(stderr, _("Cannot compile regex %s: %d.\n"), token, r);
			return r;
                }
	}
	return r;
}

int mcin_matcher_init(MCINMatcher *out)
{
	int r = 0;
	r = regcomp(&out->regex_master, ".*\\[([0-9][0-9]:[0-9][0-9]:[0-9][0-9])\\] \\[(.*)\\/(.*)\\]: (.*)", REG_EXTENDED);
	if(r)
	{
		fprintf(stderr, _("Cannot compile master regex: %d.\n"), r);
		return r;
	}
	char str[] = MC_REGEX;
	r = mcin_load(str, out->regex);
	if(r) return r;
	char str1[] = MC_REGEX_WARN;
	r = mcin_load(str1, out->regex_warn);
	return r;
}

void mcin_matcher_free(MCINMatcher *matcher)
{
	regfree(&matcher->regex_master);
	for(int i = 0; i < MCIN_REGEX_COUNT; i ++)
	{
		regfree(&matcher->regex[i]);
	}
	for(int i = 0; i < MCIN_REGEX_COUNT_WARN; i ++)
	{
		regfree(&matcher->regex_warn[i]);
	}
}

char *mcin_matcher_match(MCINMatcher *matcher, const char *str)
{
	regmatch_t pmatch[5];
	if(regexec(&matcher->regex_master, str, 5, pmatch, 0)) return 0;
	char *temp_str = calloc(strlen(str) + 1, sizeof(char));
	/* Message level:
	 * 0 - INFO
	 * 1 - WARN
	 */
	int level = 0;
	for(int i = 1 /* Ignore the string itself */; i < 5; i ++)
	{
		const regmatch_t match = pmatch[i];
		if(match.rm_so == -1)
		{
			// Shouldn't happen if the string is valid.
			free(temp_str);
			return NULL;
		}
		if(i != 2 && i != 3 && i != 4) continue; // We don't care.
		int length = match.rm_eo - match.rm_so;
		for(int j = 0; j < length; j ++)
		{
			temp_str[j] = str[match.rm_so + j];
		}
		temp_str[length] = '\0';
		if(i == 2) /* Tag */
		{
			if(strcmp(temp_str, "Server thread"))
			{
				free(temp_str);
				return NULL;
			}
		}
		if(i == 3) /* Level */
		{
			if(!strcmp(temp_str, "INFO"))
			{
				level = 0;
			}
			else if(!strcmp(temp_str, "WARN"))
			{
				level = 1;
			}
			else
			{
				free(temp_str);
				return NULL;
			}
		}
		if(i == 4) // Data
			break;
	}
	int count = 0;
	regex_t *arr = NULL;
	switch(level)
	{
		case 0:
			count = MCIN_REGEX_COUNT;
			arr = matcher->regex;
			break;
		case 1:
			count = MCIN_REGEX_COUNT_WARN;
			arr = matcher->regex_warn;
			break;
	}
	for(int i = 0; i < count; i ++)
	{
		if(!regexec(&arr[i], temp_str, 0, NULL, 0))
		{
			return temp_str;
		}
	}
	free(temp_str);
	return NULL;
}
