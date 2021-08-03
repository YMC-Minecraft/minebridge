#include "tg/tg.h"
#include "net/net.h"
#include "rcon/rcon.h"
#include "mcin/mcin.h"
#include "common.h"
#include "net/curlutils.h"

#include <stdlib.h>
#include <stdio.h>
#include <sysexits.h>
#include <curl/curl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

void *main_mc2tg(void *e)
{
	const Environ *env = (Environ*)e;
	int r = 0;
	MCINMatcher matcher;
	r = mcin_matcher_init(&matcher);
	if(r) goto cleanup;
	CURL *curl = NULL;
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);

	char buffer[501];
read:
	if(fgets(buffer, 500, stdin) == NULL)
	{
		r = 0;
		goto cleanup;
	}
	char *data = mcin_matcher_match(&matcher, buffer);
	if(data == NULL) goto read;
	TGResp *resp;
	r = tg_send_message(curl,
			env->tg_api,
			&resp,
			env->tg_chat_raw,
			data,
			TG_SEND_MESSAGE_PARSE_MODE_NONE,
			0); // Ignore errors
	free(data);
	if(!r)
	{
		tg_message_free((TGMessage*)resp->result);
		tg_resp_free(resp);
	}
	goto read;
cleanup:
	if(curl != NULL) curl_easy_cleanup(curl);
	mcin_matcher_free(&matcher);
	pthread_exit(NULL);
	return NULL;
}

void *main_tg2mc(void *e)
{
	const Environ *env = (Environ*)e;
	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
	int r = 0;

	RconPacket pkt;
        TGResp *resp;
	const char *allowed_updates[1] = { "message" };
	int updates = 0;
	char msg[4097];
	char name[1024];
	char username[512];
	char url[512];
	char uid[32];
	int32_t max_upd_id = 0;
run:
	if(*(env->rcon_sd) == 0)
	{
		sleep(5);
		goto run;
	}
        r = tg_get_updates(curl,
                        env->tg_api,
                        &resp,
			&updates,
                        max_upd_id + 1,
                        100,
                        60,
			1,
                        allowed_updates); // Ignore errors
	if(r)
	{
		sleep(5);
		goto run;
	}
	TGUpdate *updarr = (TGUpdate*)resp->result;
	if(resp->result == NULL)
	{
		tg_resp_free(resp);
		goto run;
	}
	for(int i = 0; i < updates; i ++)
	{
		char is_command = 0;
		TGUpdate upd = updarr[i];
		if(max_upd_id < upd.update_id) max_upd_id = upd.update_id;
		if(upd.message == NULL) continue;
		if(upd.message->chat->id != env->tg_chat) continue;
		TGMessage *tgmsg = upd.message;
		if(tgmsg->text != NULL)
		{
			if(tgmsg->entities_length == 1)
			{
				const TGEntity entity = tgmsg->entities[0];
				if(entity.offset == 0 &&
						!strcmp(entity.type, "bot_command"))
				{
					char is_admin = tgmsg->from->id == env->tg_admin;
					if((is_admin && (
									!strncmp(tgmsg->text, "/whitelist ", strlen("/whitelist ")) ||
									!strncmp(tgmsg->text, "/debug ", strlen("/debug ")) ||
									!strncmp(tgmsg->text, "/data ", strlen("/data ")) ||
									!strcmp(tgmsg->text, "/reload")
							)) ||
							(
									!strcmp(tgmsg->text, "/list") ||
									!strncmp(tgmsg->text, "/player ", strlen("/player ")) ||
									!strcmp(tgmsg->text, "/tick health")
							)
					  )
					{
						is_command = 1;
						strcpy(msg, tgmsg->text + 1);
					}
				}
			}
			if(!is_command)
			{
				int len = strlen(tgmsg->text);
				if(len > 100)
				{
					strncpy(msg, tgmsg->text, 100);
					sprintf(msg + 100, "... (Remaining %d characters)", len - 100);
				}
				else
				{
					strcpy(msg, tgmsg->text);
				}
			}
		}
		else if(tgmsg->document != NULL)
		{
			sprintf(msg, "%s (Document)", tgmsg->document->file_name);
		}
		else if(tgmsg->sticker != NULL)
		{
			sprintf(msg, "%s (Sticker)", tgmsg->sticker->emoji);
		}
		else if(tgmsg->photo_length > 0)
		{
			if(tgmsg->caption == NULL)
				sprintf(msg, "(Photo)");
			else
				sprintf(msg, "%s (Photo)", tgmsg->caption);
		}

		char *cmd = NULL;
		if(is_command)
		{
			cmd = msg;
		}
		else
		{
			json_object *req_root = json_object_new_array();
			json_object *obj1 = json_object_new_object();
			json_object_array_add(req_root, obj1);
			sprintf(name, "<%s> ", tgmsg->from->first_name);
			json_object_object_add(obj1, "text", json_object_new_string(name));
			json_object *hv_event_1 = json_object_new_object();
			json_object_object_add(hv_event_1, "action", json_object_new_string("show_text"));
			json_object_object_add(obj1, "hoverEvent", hv_event_1);
			json_object *contents_1 = json_object_new_array();
			json_object_object_add(hv_event_1, "contents", contents_1);
			json_object *contents_1_text_1 = json_object_new_object();
			if(tgmsg->from->username == NULL) strcpy(username, "(No Username)\n");
			else sprintf(username, "@%s\n", tgmsg->from->username);
			json_object_object_add(contents_1_text_1, "text", json_object_new_string(username));
			json_object *contents_1_text_2 = json_object_new_object();
			sprintf(uid, "UID: %d", tgmsg->from->id);
			json_object_object_add(contents_1_text_2, "text", json_object_new_string(uid));
			json_object_array_add(contents_1, contents_1_text_1);
			json_object_array_add(contents_1, contents_1_text_2);
			json_object *obj2 = json_object_new_object();
			json_object_array_add(req_root, obj2);
			json_object_object_add(obj2, "text", json_object_new_string(msg));
			json_object *obj3 = json_object_new_object();
			json_object_array_add(req_root, obj3);
			json_object_object_add(obj3, "text", json_object_new_string("[t.me]"));
			json_object_object_add(obj3, "underlined", json_object_new_boolean(1));
			json_object_object_add(obj3, "color", json_object_new_string("dark_aqua"));
			json_object *obj3_click_event = json_object_new_object();
			json_object_object_add(obj3, "clickEvent", obj3_click_event);
			json_object_object_add(obj3_click_event, "action", json_object_new_string("open_url"));
			sprintf(url, env->tg_link_fmt, tgmsg->message_id);
			json_object_object_add(obj3_click_event, "value", json_object_new_string(url));
			json_object *obj3_hover_event = json_object_new_object();
			json_object_object_add(obj3, "hoverEvent", obj3_hover_event);
			json_object_object_add(obj3_hover_event, "action", json_object_new_string("show_text"));
			json_object *obj3_hv_contents = json_object_new_object();
			json_object_object_add(obj3_hover_event, "contents", obj3_hv_contents);
			json_object_object_add(obj3_hv_contents, "text", json_object_new_string("View in Telegram"));
			const char *json = json_object_to_json_string(req_root);
			cmd = calloc(strlen(json) + 11 /* "tellraw @a " */ + 1, sizeof(char));
			sprintf(cmd, "tellraw @a %s", json);
			json_object_put(req_root);
		}
		r = rcon_build_packet(&pkt, is_command ? tgmsg->message_id : RCON_PID, RCON_EXEC_COMMAND, cmd); // Ignore errors
		if(r)
		{
			fprintf(stderr, "Fail %s\n", cmd);
		}
		else
		{
			r = rcon_send_packet(*(env->rcon_sd), &pkt); // Ignore errors
		}
		if(!is_command) free(cmd);
		tg_message_free(upd.message);
		upd.message = NULL;
		sleep(1);
	}
        if(!r)
        {
                tg_update_free(updarr);
                tg_resp_free(resp);
        }
	goto run;
}

int start_threads(Environ *env)
{

	int r = 0;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, sizeof(double) * 1000 * 1000 + 1000000);
	pthread_t thread_tg2mc;
	pthread_t thread_mc2tg;
	r = pthread_create(&thread_tg2mc, &attr, main_tg2mc, env);
	if(r)
	{
		fprintf(stderr, _("Cannot start tg2mc thread: %d"), r);
		goto cleanup;
	}
	r = pthread_create(&thread_mc2tg, &attr, main_mc2tg, env);
	if(r)
	{
		fprintf(stderr, _("Cannot start mc2tg thread: %d"), r);
		goto cleanup;
	}
cleanup:
	pthread_attr_destroy(&attr);
	return r;
}

int truncate_updates(Environ *env, CURL *curl)
{
	int r = 0;
	TGResp *resp;
	int updates = 0;
	const char *allowed_updates[1] = { "message" };
        r = tg_get_updates(curl,
                        env->tg_api,
                        &resp,
                        &updates,
                        -1,
                        100,
                        0,
                        1,
                        allowed_updates);
	if(r) return r;
	for(int i = 0; i < updates; i ++)
	{
		if(((TGUpdate*)resp->result)[i].message != NULL)
			tg_message_free(((TGUpdate*)resp->result)[i].message);
	}
	tg_update_free((TGUpdate*)resp->result);
	tg_resp_free(resp);
	return r;
}

int main(int argc, char **argv)
{
	Environ environ;
	int r = 0;
	r = environ_read(&environ);
	if(r) return r;

	curl_global_init(CURL_GLOBAL_ALL);
	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);

	r = truncate_updates(&environ, curl);
	if(r) goto cleanup;

	RconPacket pkt = {0, 0, 0, { 0x00 }};
	r = rcon_build_packet(&pkt, RCON_PID, RCON_AUTHENTICATE, environ.rcon_passwd);
	if(r) goto cleanup;
	int sd = 0;
	char start_thread = 1;
conn:
	r = net_connect(environ.rcon_host, environ.rcon_port, &sd);
	if(r) goto cleanup;
	r = rcon_send_packet(sd, &pkt);
	if(r) goto cleanup;
	r = rcon_recv_packet(&pkt, sd);
	if(r) goto cleanup;
	if(pkt.id == -1)
	{
		fprintf(stderr, _("Incorrect rcon password.\n"));
		r = EX_NOPERM;
		goto cleanup;
	}
	environ.rcon_sd = &sd;

	if(start_thread)
	{
		r = start_threads(&environ);
		start_thread = 0;
		if(r) goto cleanup;
	}

	char id[12];
read:
	r = rcon_recv_packet(&pkt, sd);
	if(r)
	{
		close(sd);
		sd = 0;
		goto conn;
	}
	if(pkt.id == -1 || pkt.id == RCON_PID)
		goto read;

	TGResp *resp;
	sprintf(id, "%d", pkt.id);
	r = tg_send_message(curl,
			environ.tg_api,
			&resp, 
			environ.tg_chat_raw,
			pkt.data,
			TG_SEND_MESSAGE_PARSE_MODE_NONE,
			id); // Ignore errors
	if(!r)
	{
		tg_message_free((TGMessage*)resp->result);
		tg_resp_free(resp);
	}
	goto read;

	goto cleanup;
cleanup:
	if(curl != NULL) curl_easy_cleanup(curl);
	curl_global_cleanup();
	if(sd != 0) net_close(sd);
	return r;
}
