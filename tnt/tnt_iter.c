
/*
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <msgpuck.h>

#include <tarantool/tnt_mem.h>
#include <tarantool/tnt_proto.h>
#include <tarantool/tnt_reply.h>
#include <tarantool/tnt_stream.h>
#include <tarantool/tnt_buf.h>
#include <tarantool/tnt_iter.h>

static struct tnt_iter *tnt_iter_init(struct tnt_iter *i) {
	int alloc = (i == NULL);
	if (alloc) {
		i = tnt_mem_alloc(sizeof(struct tnt_iter));
		if (i == NULL)
			return NULL;
	}
	memset(i, 0, sizeof(struct tnt_iter));
	i->status = TNT_ITER_OK;
	i->alloc = alloc;
	return i;
}

static int tnt_iter_array_next(struct tnt_iter *i) {
	struct tnt_iter_array *itr = TNT_IARRAY(i);
	itr->cur_index++;
	if ((uint32_t)itr->cur_index >= itr->elem_count) {
		i->status = TNT_ITER_FAIL;
		return 0;
	}
	if (itr->cur_index == 0)
		itr->elem = itr->first_elem;
	else
		itr->elem = itr->elem_end;
	itr->elem_end = itr->elem;
	mp_next(&itr->elem_end);
	return 1;
}

static void tnt_iter_array_rewind(struct tnt_iter *i) {
	struct tnt_iter_array *itr = TNT_IARRAY(i);
	itr->cur_index = -1;
	itr->elem = NULL;
	itr->elem_end = NULL;
	i->status = TNT_ITER_OK;
}

struct tnt_iter *
tnt_iter_array_object(struct tnt_iter *i, struct tnt_stream *data)
{
	return tnt_iter_array(i, TNT_SBUF_DATA(data), TNT_SBUF_SIZE(data));
}

struct tnt_iter *
tnt_iter_array(struct tnt_iter *i, const char *data, size_t size)
{
	const char *tmp_data = data;
	if (mp_check(&tmp_data, data + size) != 0)
		return NULL;
	if (!data || !size || mp_typeof(*data) != MP_ARRAY)
		return NULL;
	i = tnt_iter_init(i);
	if (i == NULL)
		return NULL;
	i->type = TNT_ITER_ARRAY;
	i->next = tnt_iter_array_next;
	i->rewind = tnt_iter_array_rewind;
	i->free = NULL;
	struct tnt_iter_array *itr = TNT_IARRAY(i);
	itr->data = data;
	itr->first_elem = data;
	itr->elem_count = mp_decode_array(&itr->first_elem);
	itr->cur_index = -1;
	return i;
}


static int tnt_iter_map_next(struct tnt_iter *i) {
	struct tnt_iter_map *itr = TNT_IMAP(i);
	itr->cur_index++;
	if ((uint32_t)itr->cur_index >= itr->pair_count) {
		i->status = TNT_ITER_FAIL;
		return 0;
	}
	if (itr->cur_index == 0)
		itr->key = itr->first_key;
	else
		itr->key = itr->value_end;
	itr->key_end = itr->key;
	mp_next(&itr->key_end);
	itr->value = itr->key_end;
	itr->value_end = itr->value;
	mp_next(&itr->value_end);
	return 1;

}

static void tnt_iter_map_rewind(struct tnt_iter *i) {
	struct tnt_iter_map *itr = TNT_IMAP(i);
	itr->cur_index = -1;
	itr->key = NULL;
	itr->key_end = NULL;
	itr->value = NULL;
	itr->value_end = NULL;
	i->status = TNT_ITER_OK;
}

struct tnt_iter *
tnt_iter_map_object(struct tnt_iter *i, struct tnt_stream *data)
{
	return tnt_iter_map(i, TNT_SBUF_DATA(data), TNT_SBUF_SIZE(data));
}

struct tnt_iter *
tnt_iter_map(struct tnt_iter *i, const char *data, size_t size)
{
	const char *tmp_data = data;
	if (mp_check(&tmp_data, data + size) != 0)
		return NULL;
	if (!data || !size || mp_typeof(*data) != MP_MAP)
		return NULL;
	i = tnt_iter_init(i);
	if (i == NULL)
		return NULL;
	i->type = TNT_ITER_MAP;
	i->next = tnt_iter_map_next;
	i->rewind = tnt_iter_map_rewind;
	i->free = NULL;
	struct tnt_iter_map *itr = TNT_IMAP(i);
	itr->data = data;
	itr->first_key = data;
	itr->pair_count = mp_decode_map(&itr->first_key);
	itr->cur_index = -1;
	return i;
}

static int tnt_iter_reply_next(struct tnt_iter *i) {
	struct tnt_iter_reply *ir = TNT_IREPLY(i);
	tnt_reply_free(&ir->r);
	tnt_reply_init(&ir->r);
	int rc = ir->s->read_reply(ir->s, &ir->r);
	if (rc == -1) {
		i->status = TNT_ITER_FAIL;
		return 0;
	}
	return (rc == 1 /* finish */ ) ? 0 : 1;
}

static void tnt_iter_reply_free(struct tnt_iter *i) {
	struct tnt_iter_reply *ir = TNT_IREPLY(i);
	tnt_reply_free(&ir->r);
}

struct tnt_iter *tnt_iter_reply(struct tnt_iter *i, struct tnt_stream *s) {
	i = tnt_iter_init(i);
	if (i == NULL)
		return NULL;
	i->type = TNT_ITER_REPLY;
	i->next = tnt_iter_reply_next;
	i->rewind = NULL;
	i->free = tnt_iter_reply_free;
	struct tnt_iter_reply *ir = TNT_IREPLY(i);
	ir->s = s;
	tnt_reply_init(&ir->r);
	return i;
}

/*static int tnt_iter_request_next(struct tnt_iter *i) {
	struct tnt_iter_request *ir = TNT_IREQUEST(i);
	tnt_request_free(&ir->r);
	tnt_request_init(&ir->r);
	int rc = ir->s->read_request(ir->s, &ir->r);
	if (rc == -1) {
		i->status = TNT_ITER_FAIL;
		return 0;
	}
	return (rc == 1) ? 0 : 1;
}

static void tnt_iter_request_free(struct tnt_iter *i) {
	struct tnt_iter_request *ir = TNT_IREQUEST(i);
	tnt_request_free(&ir->r);
}*/

/*struct tnt_iter *tnt_iter_request(struct tnt_iter *i, struct tnt_stream *s) {
	i = tnt_iter_init(i);
	if (i == NULL)
		return NULL;
	i->type = TNT_ITER_REQUEST;
	i->next = tnt_iter_request_next;
	i->rewind = NULL;
	i->free = tnt_iter_request_free;
	struct tnt_iter_request *ir = TNT_IREQUEST(i);
	ir->s = s;
	tnt_request_init(&ir->r);
	return i;
}

static int tnt_iter_storage_next(struct tnt_iter *i) {
	struct tnt_iter_storage *is = TNT_ISTORAGE(i);
	tnt_tuple_free(&is->t);
	tnt_tuple_init(&is->t);

	int rc = is->s->read_tuple(is->s, &is->t);
	if (rc == -1) {
		i->status = TNT_ITER_FAIL;
		return 0;
	}
	return (rc == 1) ? 0 : 1;
}

static void tnt_iter_storage_free(struct tnt_iter *i) {
	struct tnt_iter_storage *is = TNT_ISTORAGE(i);
	tnt_tuple_free(&is->t);
}*/

/*
 * tnt_iter_storage()
 *
 * initialize tuple storage iterator;
 * create and initialize storage iterator;
 *
 * i - tuple storage iterator pointer, maybe NULL
 * s - stream pointer
 *
 * if stream iterator pointer is NULL, then new stream
 * iterator will be created.
 *
 * return stream iterator pointer, or NULL on error.
*/
/*struct tnt_iter *tnt_iter_storage(struct tnt_iter *i, struct tnt_stream *s) {
	i = tnt_iter_init(i);
	if (i == NULL)
		return NULL;
	i->type = TNT_ITER_STORAGE;
	i->next = tnt_iter_storage_next;
	i->rewind = NULL;
	i->free = tnt_iter_storage_free;
	struct tnt_iter_storage *is = TNT_ISTORAGE(i);
	is->s = s;
	tnt_tuple_init(&is->t);
	return i;
}*/

void tnt_iter_free(struct tnt_iter *i) {
	if (i->free)
		i->free(i);
	if (i->alloc)
		tnt_mem_free(i);
}

int tnt_next(struct tnt_iter *i) {
	return i->next(i);
}

void tnt_rewind(struct tnt_iter *i) {
	i->status = TNT_ITER_OK;
	if (i->rewind)
		i->rewind(i);
}
