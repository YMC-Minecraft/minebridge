#ifndef _TG_H
#define _TG_H

#include "../environ.h"
#include <json-c/json_object.h>
#include <curl/curl.h>

#define TG_SEND_MESSAGE_PARSE_MODE_NONE		0
#define TG_SEND_MESSAGE_PARSE_MODE_HTML		1
#define TG_SEND_MESSAGE_PARSE_MODE_MARKDOWN	2
#define TG_SEND_MESSAGE_PARSE_MODE_MARKDOWN_V2	3

typedef struct tg_entity {
	const char *type;
	int32_t offset;
	int32_t length;
} TGEntity;

typedef struct tg_resp {
        json_object *json;
        void *result;
	json_object *result_json; // Ownership: json
} TGResp;

typedef struct tg_user {
	int32_t id;
	const char *first_name; // Ownership: json
	const char *last_name; // Ownership: json
	const char *username; // Ownership: json
} TGUser;

typedef struct tg_chat {
	int64_t id;
} TGChat;

typedef struct tg_sticker {
	const char *emoji; // Ownership: json
} TGSticker;

typedef struct tg_document {
	const char *file_name; // Ownership: json
} TGDocument;

typedef struct tg_photo {
} TGPhoto;

typedef struct tg_message {
	int32_t message_id;
	TGUser *from;
	TGChat *chat;
	const char *text;
	TGDocument *document;
	TGSticker *sticker;
	int photo_length;
	TGPhoto *photo; // Array.
	const char *caption;
	int entities_length;
	TGEntity *entities; // Array.
} TGMessage;

typedef struct tg_update {
	int32_t update_id;
	TGMessage *message;
} TGUpdate;

int tg_send_message(
		const CURL *curl,
		const char *tg_key,
		TGResp **out,
		const char *chat_id,
		const char *text,
		const int parse_mode,
		const char *reply_to_message_id);

int tg_get_updates(
		const CURL *curl,
		const char *tg_key,
		TGResp **out, // Array of structs.
		int *out_updates,
		const int offset,
		const int limit,
		const int timeout,
		const int allowed_updates_count,
		const char **allowed_updates);

void tg_update_free(TGUpdate *arr);
void tg_message_free(TGMessage *obj);
void tg_user_free(TGUser *obj);
void tg_chat_free(TGChat *obj);
void tg_resp_free(TGResp *obj);
void tg_sticker_free(TGSticker *obj);
void tg_document_free(TGDocument *obj);
void tg_photo_free(TGPhoto *obj);

#endif // _TG_H
