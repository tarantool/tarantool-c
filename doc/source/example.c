#define TARANTOOL_URI "localhost:3301"
#define BUFFER_SIZE   256*1024

#define log_error(fmt, ...)  fprintf(stderr, "%s:%d E>" fmt, __func__, __LINE__, ##__VA_ARGS__)

#include <assert.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdio.h>
#include <inttypes.h>

#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include <tarantool/tnt_opt.h>
#include <msgpuck.h>

int
tnt_request_set_sspace(struct tnt_stream *tnt, struct tnt_request *req,
		       const char *space)
{
	assert(tnt && space);
	int32_t sno = tnt_get_spaceno(tnt, space, strlen(space));
	if (sno == -1)
		return -1;
	return tnt_request_set_space(req, sno);
}

int
tnt_request_set_sindex(struct tnt_stream *tnt, struct tnt_request *req,
		       const char *index)
{
	assert(tnt && index && req->space_id);
	int32_t ino = tnt_get_indexno(tnt, req->space_id, index, strlen(index));
	if (ino == -1)
		return -1;
	return tnt_request_set_index(req, ino);
}

int profile_space_no;

struct msg_profile {
	int id;
	char *user;
	int like;
	char *msg;
};

struct tnt_stream *tarantool_connection(const char *uri) {
	struct tnt_stream *connection = tnt_net(NULL);
	if (tnt_set(connection, TNT_OPT_URI, uri) == -1)
		goto rollback;
	tnt_set(connection, TNT_OPT_SEND_BUF, BUFFER_SIZE);
	tnt_set(connection, TNT_OPT_RECV_BUF, BUFFER_SIZE);
	if (tnt_connect(connection) == -1) {
		log_error("Failed to connect (%s)", tnt_strerror(connection));
		goto rollback;
	}
	return connection;
rollback:
	tnt_stream_free(connection);
	return NULL;
}

void msg_profile_free(struct msg_profile *msg) {
	if (msg) {
		free(msg->msg);
		free(msg->user);
		free(msg);
	}
}

int msg_profile_decode(struct tnt_reply *rpl, struct msg_profile **rv) {
	uint32_t str_len = 0;
	if (rpl->code != 0) {
		log_error("Query error %d: %.*s", (int )TNT_REPLY_ERR(rpl),
			  (int )(rpl->error_end - rpl->error), rpl->error);
		return -1;
	}
	if (mp_typeof(*rpl->data) != MP_ARRAY) {
		log_error("Bad reply format");
		return -1;
	} else if (mp_decode_array(&rpl->data) == 0) {
		return 0;
	} else if (mp_decode_array(&rpl->data) > 0) {
		log_error("Bad reply format");
		return -1;
	}
	struct msg_profile *profile = malloc(sizeof(struct msg_profile));
	if (profile == NULL) {
		log_error("OOM");
		return -1;
	}
	const char *data = rpl->data;
	if (mp_typeof(*data) != MP_ARRAY) {
		log_error("Bad reply format");
		goto rollback;
	}
	uint32_t len = mp_decode_array(&data);
	if (len < 2) {
		log_error("Bad reply format");
		goto rollback;
	}
	if (mp_typeof(*data) != MP_UINT) {
		log_error("Bad reply format");
		goto rollback;
	}
	profile->id = mp_decode_uint(&data);
	if (mp_typeof(*data) != MP_STR) {
		log_error("Bad reply format");
		goto rollback;
	}
	profile->user = (char *)mp_decode_str(&data, &str_len);
	profile->user = strndup(profile->user, str_len);
	if (profile->user == NULL) {
		log_error("OOM");
		goto rollback;
	}
	if (mp_typeof(*data) != MP_UINT) {
		log_error("Bad reply format");
		goto rollback;
	}
	profile->like = mp_decode_uint(&data);
	if (mp_typeof(*data) != MP_STR) {
		log_error("Bad reply format");
		goto rollback;
	}
	profile->msg = (char *)mp_decode_str(&data, &str_len);
	profile->msg = strndup(profile->msg, str_len);
	if (profile->msg == NULL) {
		log_error("OOM");
		goto rollback;
	}
	len -= 4;
	while (len > 0) mp_next(&data);
	rpl->data = data;
	*rv = profile;
	return 0;
rollback:
	msg_profile_free(profile);
	*rv = NULL;
	return -1;
}

int msg_profile_get(struct tnt_stream *tnt, int id, struct msg_profile **rv) {
	static struct tnt_stream *obj = NULL;
	static struct tnt_reply  *rpl = NULL;
	if (!obj) {
		obj = tnt_object(NULL);
		if (!obj) return -1;
	}
	if (!rpl) {
		rpl = tnt_reply_init(NULL);
		if (!rpl) return -1;
	}
	tnt_object_reset(obj);
	tnt_object_add_array(obj, 1);
	tnt_object_add_int(obj, id);
	if (tnt_select(tnt, profile_space_no, 0, UINT32_MAX, 0, 0, obj) == -1) {
		log_error("Failed to append request");
		return -1;
	}
	if (tnt_flush(tnt) == -1) {
		log_error("Failed to send request (%s)", tnt_strerror(tnt));
		return -1;
	}
	if (tnt->read_reply(tnt, rpl) == -1) {
		log_error("Failed to recv/parse result");
		if (tnt_error(tnt)) log_error("%s", tnt_strerror(tnt));
		return -1;
	}
	return msg_profile_decode(rpl, rv);
}

int msg_profile_get_multi(struct tnt_stream *tnt, int *id, size_t cnt,
			  struct msg_profile ***rv) {
	static struct tnt_request *req = NULL;
	if (!req) {
		req = tnt_request_select(req);
		tnt_request_set_sspace(tnt, req, "messages");
		tnt_request_set_sindex(tnt, req, "primary");
		if (!req) return -1;
	}
	for (int i = 0; i < cnt; ++i) {
		tnt_request_set_key_format(req, "[%d]", id[i]);
		if (tnt_request_compile(tnt, req) == -1) {
			log_error("OOM");
			return -1;
		}
	}
	if (tnt_flush(tnt) == -1) {
		log_error("Failed to send result (%s)", tnt_strerror(tnt));
		return -1;
	}
	struct msg_profile **retval = calloc(cnt + 1, sizeof(struct msg_profile *));
	size_t rcvd = 0; retval[cnt] = NULL;
	struct tnt_iter it; tnt_iter_reply(NULL, tnt);
	while (tnt_next(&it)) {
		int rv = msg_profile_decode(TNT_IREPLY_PTR(&it), &retval[rcvd]);
		if (retval[rcvd] != NULL) ++rcvd;
		if (rv == -1) {
			for (int i = 0; i < rcvd; i++)
				msg_profile_free(retval[rcvd]);
			free(retval);
			while (tnt_next(&it));
			return -1;
		}
	}
	*rv = retval;
	return rcvd;
}

int msg_profile_like_add(struct tnt_stream *tnt, int id) {
	static struct tnt_request *req = NULL;
	static struct tnt_stream  *obj = NULL;
	static struct tnt_reply   *rpl = NULL;
	size_t str_len = 0;
	if (!obj) {
		obj = tnt_update_container(NULL);
		if (obj == NULL) {
			log_error("OOM");
			return -1;
		}
		/* update like counter and prepend 'liked ' to msg */
		if (tnt_update_arith_int(obj, 2, '+', 1) == -1 ||
		    tnt_update_splice(obj, 3, 0, 0, "liked ", 6) == -1) {
			log_error("OOM");
			return -1;
		}
		tnt_update_container_close(obj);
	}
	if (!req) {
		req = tnt_request_update(req);
		tnt_request_set_sspace(tnt, req, "messages");
		tnt_request_set_sindex(tnt, req, "primary");
		tnt_request_set_tuple(req, obj);
		if (!req) return -1;
	}
	if (!rpl) {
		rpl = tnt_reply_init(NULL);
		if (!rpl) return -1;
	}
	tnt_request_set_key_format(req, "[%d]", id);
	if (tnt_request_compile(tnt, req) == -1) {
		log_error("OOM");
		return -1;
	}
	if (tnt_flush(tnt) == -1) {
		log_error("Failed to send request (%s)", tnt_strerror(tnt));
		return -1;
	}
	if (tnt->read_reply(tnt, rpl) == -1) {
		log_error("Failed to recv/parse result");
		if (tnt_error(tnt)) log_error("%s", tnt_strerror(tnt));
		return -1;
	}
	if (rpl->code) {
		log_error("Query error %d: %.*s", (int )TNT_REPLY_ERR(rpl),
			  (int )(rpl->error_end - rpl->error), rpl->error);
		return -1;
	}
	return 0;
}

int msg_profile_create(struct tnt_request *req, int id, const char *usr,
		       size_t usr_len, int likes, const char *msg, size_t msg_len) {
	if (tnt_request_set_tuple_format(req, "[%d%.*s%d%.*s]", id, usr_len,
					 usr, likes, msg_len, msg) == -1) {
		return -1;
	}
	return 0;
}

int msg_profile_create_alt(struct tnt_stream *obj, int id, const char *usr,
			   size_t usr_len, int likes, const char *msg,
			   size_t msg_len) {
	if (tnt_object_add_array(obj, 4) == -1 ||
	    tnt_object_add_int(obj, id) == -1 ||
	    tnt_object_add_str(obj, usr, usr_len) == -1 ||
	    tnt_object_add_int(obj, likes) == -1 ||
	    tnt_object_add_str(obj, msg, msg_len) == -1) {
		log_error("OOM");
		return -1;
	}
	return 0;
}

int msg_profile_put(struct tnt_stream *tnt, int id, const char *usr,
		    size_t usr_len, int likes, const char *msg, size_t msg_len) {
	static struct tnt_request *req = NULL;
	static struct tnt_reply   *rpl = NULL;
	if (!req) {
		req = tnt_request_update(req);
		if (!req) return -1;
		if (tnt_request_set_sspace(tnt, req, "messages") == -1) {
			log_error("failed to find space 'messages'");
			return -1;
		}
		if (tnt_request_set_sindex(tnt, req, "primary") == -1) {
			log_error("failed to find index 'primary' in space "
				  "'message'");
			return -1;
		}
	}
	if (msg_profile_create(req, id, usr, usr_len, likes, msg, msg_len) == -1) {
		log_error("OOM");
		return -1;
	}
	if (tnt_request_set_tuple_format(req, "[%d%.*s%d%.*s]", id, usr_len,
					 usr, likes, msg_len, msg) == -1) {
		log_error("OOM");
		return -1;
	}
	if (tnt_request_compile(tnt, req) == -1) {
		log_error("OOM");
		return -1;
	}
	if (tnt_flush(tnt) == -1) {
		log_error("Failed to send request (%s)", tnt_strerror(tnt));
		return -1;
	}
	if (tnt->read_reply(tnt, rpl) == -1) {
		log_error("Failed to recv/parse result");
		if (tnt_error(tnt)) log_error("%s", tnt_strerror(tnt));
		return -1;
	}
	if (rpl->code) {
		log_error("Query error %d: %.*s", (int )TNT_REPLY_ERR(rpl),
			  (int )(rpl->error_end - rpl->error), rpl->error);
		return -1;
	}
	return 0;
}

/* To build this example use next command:
 * gcc example.c -I ../../include/ -L ../../tnt/ -ltarantool */
int main() {
	struct tnt_stream *tnt = tarantool_connection(TARANTOOL_URI);
	profile_space_no = tnt_get_spaceno(tnt, "messages", strlen("messages"));
	tnt_stream_free(tnt);
}
