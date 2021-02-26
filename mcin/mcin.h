#ifndef _MCIN_H
#define _MCIN_H

#include <regex.h>

#define MCIN_REGEX_COUNT 51

typedef struct mcin_matcher {
	regex_t regex_master;
	regex_t regex[MCIN_REGEX_COUNT];
} MCINMatcher;

int mcin_matcher_init(MCINMatcher *out);
void mcin_matcher_free(MCINMatcher *matcher);
char *mcin_matcher_match(MCINMatcher *matcher, const char *str);

#endif // _MCIN_H
