#include "../common.h"
#include "curlutils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

size_t curl_callback(void *ptr, size_t size, size_t nmemb, CURLBody *userp)
{
	size_t newLen = userp->len + size * nmemb;
	userp->ptr = realloc(userp->ptr, newLen + 1);
	if(userp->ptr == NULL) {
		fprintf(stderr, _("Cannot allocate memory.\n"));
		return 0;
	}
	memcpy(userp->ptr + userp->len, ptr, size * nmemb);
	userp->ptr[newLen] = '\0';
	userp->len = newLen;
	return size * nmemb;
}

void curlbody_setup(CURLBody *body)
{
	body->len = 0;
	body->ptr = calloc(1, sizeof(char));
	body->ptr[0] = '\0';
}
