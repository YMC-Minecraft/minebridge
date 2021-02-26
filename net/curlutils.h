#ifndef _CURLUTILS_H
#define _CURLUTILS_H

#include <stddef.h>

typedef struct curlbody {
	char *ptr;
	size_t len;
} CURLBody;

size_t curl_callback(void *ptr, size_t size, size_t nmemb, CURLBody *userp);
void curlbody_setup(CURLBody *body);

#endif // _CURLUTILS_H
