#include "tg.h"
#include "../net/curlutils.h"
#include "../common.h"

#include <string.h>
#include <stdlib.h>
#include <sysexits.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#include <curl/curl.h>

void _tg_entity_parse(TGEntity *entity, json_object *json)
{
	entity->type = NULL;
	entity->offset = 0;
	entity->length = 0;

	json_object *obj;
	if(json_object_object_get_ex(json, "type", &obj))
		entity->type = json_object_get_string(obj);
	if(json_object_object_get_ex(json, "offset", &obj))
		entity->offset = json_object_get_int(obj);
	if(json_object_object_get_ex(json, "length", &obj))
		entity->length = json_object_get_int(obj);
}

void _tg_photo_parse(TGPhoto *photo, json_object *json)
{
}

TGSticker *_tg_sticker_parse(json_object *json)
{
	TGSticker *sticker = malloc(sizeof(TGSticker));
	sticker->emoji = NULL;

	json_object *obj;
	if(json_object_object_get_ex(json, "emoji", &obj))
		sticker->emoji = json_object_get_string(obj);

	return sticker;
}

TGDocument *_tg_document_parse(json_object *json)
{
	TGDocument *document = malloc(sizeof(TGDocument));
	document->file_name = NULL;

	json_object *obj;
	if(json_object_object_get_ex(json, "file_name", &obj))
		document->file_name = json_object_get_string(obj);

	return document;
}
TGChat *_tg_chat_parse(json_object *json)
{
	TGChat *chat = malloc(sizeof(TGChat));
	chat->id = 0;

	json_object *obj;
	if(json_object_object_get_ex(json, "id", &obj))
		chat->id = json_object_get_int64(obj);

	return chat;
}

TGUser *_tg_user_parse(json_object *json)
{
	TGUser *user = malloc(sizeof(TGUser));
	user->id = 0;
	user->first_name = NULL;
	user->last_name = NULL;
	user->username = NULL;

	json_object *obj;
	if(json_object_object_get_ex(json, "id", &obj))
		user->id = json_object_get_int64(obj);
	if(json_object_object_get_ex(json, "first_name", &obj))
		user->first_name = json_object_get_string(obj);
	if(json_object_object_get_ex(json, "last_name", &obj))
		user->last_name = json_object_get_string(obj);
	if(json_object_object_get_ex(json, "username", &obj))
		user->username = json_object_get_string(obj);

	return user;
}

TGMessage *_tg_message_parse(json_object *json)
{
	TGMessage *message = malloc(sizeof(TGMessage));
	message->message_id = 0;
	message->from = NULL;
	message->chat = NULL;
	message->text = NULL;
	message->sticker = NULL;
	message->document = NULL;
	message->photo_length = 0;
	message->photo = NULL;
	message->caption = NULL;
	message->entities_length = 0;
	message->entities = NULL;

	json_object *obj;
	if(json_object_object_get_ex(json, "message_id", &obj))
		message->message_id = json_object_get_int(obj);
	if(json_object_object_get_ex(json, "from", &obj))
		message->from = _tg_user_parse(obj);
	if(json_object_object_get_ex(json, "chat", &obj))
		message->chat = _tg_chat_parse(obj);
	if(json_object_object_get_ex(json, "text", &obj))
		message->text = json_object_get_string(obj);
	if(json_object_object_get_ex(json, "sticker", &obj))
		message->sticker = _tg_sticker_parse(obj);
	if(json_object_object_get_ex(json, "document", &obj))
		message->document = _tg_document_parse(obj);
	if(json_object_object_get_ex(json, "photo", &obj))
	{
		int length = json_object_array_length(obj);
		message->photo_length = length;
		message->photo = calloc(length, sizeof(TGPhoto));
		for(int i = 0; i < length; i ++)
		{
			json_object *childobj = json_object_array_get_idx(obj, i);
			if(childobj == NULL) continue;
			_tg_photo_parse(&message->photo[i], childobj);
		}
	}
	if(json_object_object_get_ex(json, "caption", &obj))
		message->caption = json_object_get_string(obj);
	if(json_object_object_get_ex(json, "entities", &obj))
	{
		int length = json_object_array_length(obj);
		message->entities_length = length;
		message->entities = calloc(length, sizeof(TGEntity));
		for(int i = 0; i < length; i ++)
		{
			json_object *childobj = json_object_array_get_idx(obj, i);
			if(childobj == NULL) continue;
			_tg_entity_parse(&message->entities[i], childobj);
		}
	}

	return message;
}

void _tg_update_parse(TGUpdate *update, json_object *json)
{
	update->update_id = 0;
	update->message = NULL;

	json_object *obj;
	if(json_object_object_get_ex(json, "update_id", &obj))
		update->update_id = json_object_get_int(obj);
	if(json_object_object_get_ex(json, "message", &obj))
		update->message = _tg_message_parse(obj);
}

#define _TG_BASE_URL "https://api.telegram.org/bot"

int _tg_api_call(
		CURL *curl,
		const char *tg_api,
		const char parse_body,
		TGResp **out,
		const char *method,
		const int query_arg_count,
		const char **query_arg_keys,
		const char **query_arg_values)
{
	curl_mime *form = curl_mime_init(curl);
	curl_mimepart *field = NULL;
	for(int i = 0; i < query_arg_count; i ++)
	{
		if(query_arg_keys[i] == NULL ||
				query_arg_values[i] == NULL) continue;
		field = curl_mime_addpart(form);
		curl_mime_name(field, query_arg_keys[i]);
		curl_mime_data(field, query_arg_values[i], CURL_ZERO_TERMINATED);
	}
	int url_size = strlen(_TG_BASE_URL) + strlen(tg_api) + 1 /* / */ +
		strlen(method) + 1 /* \0 */;
	char *url = calloc(url_size, sizeof(char));
	strcpy(url, _TG_BASE_URL);
	strcat(url, tg_api);
	strcat(url, "/");
	strcat(url, method);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
	CURLBody body;
	curlbody_setup(&body);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
	int r = 0;
	r = curl_easy_perform(curl);
	if(r != CURLE_OK) goto cleanup;
	long http_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	if(http_code != 200)
	{
		fprintf(stderr, _("Error while %s(): %lu (%s)\n"), method, http_code, body.ptr);
		r = EX_IOERR;
		goto cleanup;
	}
	if(parse_body)
	{
		json_object *json = json_tokener_parse(body.ptr);
	        json_object *obj;
		TGResp *resp = malloc(sizeof(TGResp));
		resp->json = json;
		resp->result = NULL;
		if(json_object_object_get_ex(json, "result", &obj))
			resp->result_json = obj;
		*out = resp;
	}
cleanup:
	free(url);
	free(body.ptr);
	curl_easy_setopt(curl, CURLOPT_MIMEPOST, NULL);
	curl_mime_free(form);
	return r;
}

int tg_send_message(
		const CURL *curl,
		const char *tg_key,
		TGResp **out,
		const char *chat_id,
		const char *text,
		const int parse_mode,
		const char *reply_to_message_id)
{
	TGResp *resp;
	int r;
	const char *q_arg[5] = {
		"chat_id",
		"text",
		"parse_mode",
		"allow_sending_without_reply",
		"reply_to_message_id"
	};
	char *parse_mode_raw;
	switch(parse_mode)
	{
		case TG_SEND_MESSAGE_PARSE_MODE_NONE:
			q_arg[2] = NULL;
			parse_mode_raw = NULL;
			break;
		case TG_SEND_MESSAGE_PARSE_MODE_HTML:
			parse_mode_raw = "HTML";
			break;
		case TG_SEND_MESSAGE_PARSE_MODE_MARKDOWN:
			parse_mode_raw = "Markdown";
			break;
		case TG_SEND_MESSAGE_PARSE_MODE_MARKDOWN_V2:
			parse_mode_raw = "MarkdownV2";
			break;
		default:
			parse_mode_raw = "HTML";
			break;
	}
	const char *q_val[5] = {
		chat_id,
		text,
		parse_mode_raw,
		"True",
		reply_to_message_id
	};
	r = _tg_api_call((CURL*)curl, 
			tg_key,
			1,
			&resp,
			"sendMessage",
			5,
			q_arg,
			q_val);
	if(r) return r;
	resp->result = _tg_message_parse(resp->result_json);
	*out = resp;
	return r;
}

int tg_get_updates(
		const CURL *curl,
		const char *tg_key,
		TGResp **out,
		int *out_updates,
		const int offset,
		const int limit,
		const int timeout,
		const int allowed_updates_count,
		const char **allowed_updates)
{
	TGResp *resp;
	char parse_body = 1;
	int r;
	const char *q_arg[4] = {
		"offset",
		"limit",
		"timeout",
		"allowed_updates"
	};
	char offset_str[12];
	sprintf(offset_str, "%d", offset);
	char limit_str[12];
	sprintf(limit_str, "%d", limit);
	char timeout_str[12];
	sprintf(timeout_str, "%d", timeout);
	json_object *allowed_updates_arr = json_object_new_array();
	for(int i = 0; i < allowed_updates_count; i ++)
	{
		json_object *obj = json_object_new_string(allowed_updates[i]);
		json_object_array_add(allowed_updates_arr, obj);
	}
	const char *q_val[4] = {
		offset_str,
		limit_str,
		timeout_str,
		json_object_to_json_string(allowed_updates_arr)
	};
	r = _tg_api_call((CURL*)curl,
			tg_key,
			parse_body,
			&resp,
			"getUpdates",
			4,
			q_arg,
			q_val);
	json_object_put(allowed_updates_arr);
	if(r) return r;
	if(parse_body)
	{
		json_object *arr = resp->result_json;
		int length = json_object_array_length(arr);
		TGUpdate *updarr = calloc(length, sizeof(TGUpdate));
		for(int i = 0; i < length; i ++)
		{
			json_object *obj = json_object_array_get_idx(arr, i);
			if(obj == NULL) continue;
			_tg_update_parse(&updarr[i], obj);
		}
		resp->result = updarr;
		*out = resp;
		*out_updates = length;
	}
	return r;
}

void tg_message_free(TGMessage *obj)
{
	if(obj->from != NULL)
	{
		tg_user_free(obj->from);
		obj->from = NULL;
	}
	if(obj->chat != NULL)
	{
		tg_chat_free(obj->chat);
		obj->chat = NULL;
	}
	if(obj->document != NULL)
	{
		tg_document_free(obj->document);
		obj->document = NULL;
	}
	if(obj->sticker != NULL)
	{
		tg_sticker_free(obj->sticker);
		obj->sticker = NULL;
	}
	if(obj->photo != NULL)
	{
		free(obj->photo);
		obj->photo = NULL;
	}
	if(obj->entities != NULL)
	{
		free(obj->entities);
		obj->entities = NULL;
	}

	free(obj);
}

void tg_user_free(TGUser *obj)
{
	free(obj);
}

void tg_chat_free(TGChat *obj)
{
	free(obj);
}

void tg_resp_free(TGResp *obj)
{
	json_object_put(obj->json);
	free(obj);
}

void tg_update_free(TGUpdate *arr)
{
	free(arr);
}

void tg_sticker_free(TGSticker *obj)
{
	free(obj);
}

void tg_document_free(TGDocument *obj)
{
	free(obj);
}
