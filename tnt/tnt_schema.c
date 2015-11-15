#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <inttypes.h>
#include <assert.h>
#include <stdint.h>

#include <msgpuck/msgpuck.h>

#include <tarantool/tnt_reply.h>
#include <tarantool/tnt_stream.h>
#include <tarantool/tnt_buf.h>
#include <tarantool/tnt_object.h>
#include <tarantool/tnt_proto.h>
#include <tarantool/tnt_schema.h>
#include <tarantool/tnt_mem.h>
#include <tarantool/tnt_select.h>

#include "tnt_assoc.h"

static inline void
tnt_schema_fval_free(struct tnt_schema_fval *val) {
	tnt_mem_free(val->field_name);
}

static inline void
tnt_schema_ival_free(struct tnt_schema_ival *val) {
	if (val) {
		tnt_mem_free(val->index_name);
		int i = 0;
		for (i = val->index_parts_len; i > 0; --i)
			tnt_schema_fval_free(&(val->index_parts[i - 1]));
		tnt_mem_free(val->index_parts);
	}
	tnt_mem_free(val);
}

static inline void
tnt_schema_index_free(struct mh_assoc_t *schema) {
	mh_int_t pos = 0;
	mh_int_t index_slot = 0;
	mh_foreach(schema, pos) {
		struct tnt_schema_ival *ival =
			(*mh_assoc_node(schema, pos))->data;
		struct assoc_val *av1 = NULL, *av2 = NULL;
		do {
			struct assoc_key key_number = {
				(void *)&(ival->index_number),
				sizeof(uint32_t)
			};
			index_slot = mh_assoc_find(schema, &key_number, NULL);
			if (index_slot == mh_end(schema))
				break;
			av1 = *mh_assoc_node(schema, index_slot);
			mh_assoc_del(schema, index_slot, NULL);
		} while (0);
		do {
			struct assoc_key key_string = {
				ival->index_name,
				ival->index_name_len
			};
			index_slot = mh_assoc_find(schema, &key_string, NULL);
			if (index_slot == mh_end(schema))
				break;
			av2 = *mh_assoc_node(schema, index_slot);
			mh_assoc_del(schema, index_slot, NULL);
		} while (0);
		tnt_schema_ival_free(ival);
		if (av1) tnt_mem_free((void *)av1);
		if (av2) tnt_mem_free((void *)av2);
	}
}

static inline void
tnt_schema_sval_free(struct tnt_schema_sval *val) {
	if (val) {
		tnt_mem_free(val->space_name);
		int i = 0;
		for (i = val->schema_list_len; i > 0; --i)
			tnt_schema_fval_free(&(val->schema_list[i - 1]));
		tnt_mem_free(val->schema_list);
		if (val->index_hash) {
			tnt_schema_index_free(val->index_hash);
			mh_assoc_delete(val->index_hash);
		}
	}
	tnt_mem_free(val);
}

static inline void
tnt_schema_space_free(struct mh_assoc_t *schema) {
	mh_int_t pos = 0;
	mh_int_t space_slot = 0;
	mh_foreach(schema, pos) {
		struct tnt_schema_sval *sval = NULL;
		sval = (*mh_assoc_node(schema, pos))->data;
		struct assoc_val *av1 = NULL, *av2 = NULL;
		do {
			struct assoc_key key_number = {
				(void *)&(sval->space_number),
				sizeof(uint32_t)
			};
			space_slot = mh_assoc_find(schema, &key_number, NULL);
			if (space_slot == mh_end(schema))
				break;
			av1 = *mh_assoc_node(schema, space_slot);
			mh_assoc_del(schema, space_slot, NULL);
		} while (0);
		do {
			struct assoc_key key_string = {
				sval->space_name,
				sval->space_name_len
			};
			space_slot = mh_assoc_find(schema, &key_string, NULL);
			if (space_slot == mh_end(schema))
				break;
			av2 = *mh_assoc_node(schema, space_slot);
			mh_assoc_del(schema, space_slot, NULL);
		} while (0);
		tnt_schema_sval_free(sval);
		if (av1) tnt_mem_free((void *)av1);
		if (av2) tnt_mem_free((void *)av2);
	}
}

static inline int
tnt_schema_add_space(
		struct mh_assoc_t *schema,
		const char **data) {
	struct tnt_schema_sval *space = NULL;
	struct assoc_val *space_string = NULL, *space_number = NULL;
	const char *tuple = *data;
	if (mp_typeof(*tuple) != MP_ARRAY) goto error;
	uint32_t tuple_len = mp_decode_array(&tuple);
	if (tuple_len < 6) goto error;
	space = tnt_mem_alloc(sizeof(struct tnt_schema_sval));
	if (!space) goto error;
	memset(space, 0, sizeof(struct tnt_schema_sval));
	if (mp_typeof(*tuple) != MP_UINT) goto error;
	space->space_number = mp_decode_uint(&tuple);
	/* skip owner id */
	mp_next(&tuple);
	if (mp_typeof(*tuple) != MP_STR) goto error;
	const char *space_name_tmp = mp_decode_str(&tuple,
						   &space->space_name_len);
	space->space_name = tnt_mem_alloc(space->space_name_len);
	if (!space->space_name) goto error;
	memcpy(space->space_name, space_name_tmp, space->space_name_len);
	/* skip engine name */
	mp_next(&tuple);
	/* skip field count */
	mp_next(&tuple);
	/* skip format */
	mp_next(&tuple);
	/* skip format */
	mp_next(&tuple);
	/* parse format
	if (mp_typeof(*tuple) != MP_ARRAY) goto error;
	uint32_t fmt_len = mp_decode_array(&tuple);
	if (fmt_len) {
		space->schema_list_len = fmt_len;
		space->schema_list =
			tnt_mem_alloc(fmt_len * sizeof(struct tnt_schema_fval));
		if (!space->schema_list) goto error;
		while (fmt_len-- > 0) {
			struct tnt_schema_fval *val = &(space->schema_list[
				(space->schema_list_len - fmt_len - 1)]);
			if (mp_typeof(*tuple) != MP_MAP) goto error;
			uint32_t arrsz = mp_decode_map(&tuple);
			while (arrsz-- > 0) {
				uint32_t sfield_len = 0;
				if (mp_typeof(*tuple) != MP_STR) goto error;
				const char *sfield = mp_decode_str(&tuple,
								&sfield_len);
				if (memcmp(sfield, "name", sfield_len) == 0) {
					if (mp_typeof(*tuple) != MP_STR) goto error;
					sfield = mp_decode_str(&tuple,
							&val->field_name_len);
					val->field_name = tnt_mem_alloc(val->field_name_len);
					if (!val->field_name) goto error;
					memcpy(val->field_name, sfield,
					val->field_name_len);
				} else if (memcmp(sfield, "type", sfield_len) == 0) {
					if (mp_typeof(*tuple) != MP_STR) goto error;
					sfield = mp_decode_str(&tuple, &sfield_len);
					switch(*sfield) {
					case ('s'):
					case ('S'):
						val->field_type = FT_STR;
						break;
					case ('n'):
					case ('N'):
						val->field_type = FT_NUM;
						break;
					default:
						val->field_type = FT_OTHER;
					}
				}
			}
		}
	} */
	space->index_hash = mh_assoc_new();
	if (!space->index_hash) goto error;
	space_string = tnt_mem_alloc(sizeof(struct assoc_val));
	if (!space_string) goto error;
	space_string->key.id     = space->space_name;
	space_string->key.id_len = space->space_name_len;
	space_string->data = space;
	space_number = tnt_mem_alloc(sizeof(struct assoc_val));
	if (!space_number) goto error;
	space_number->key.id = (void *)&(space->space_number);
	space_number->key.id_len = sizeof(space->space_number);
	space_number->data = space;
	mh_assoc_put(schema, (const struct assoc_val **)&space_string,
		     NULL, NULL);
	mh_assoc_put(schema, (const struct assoc_val **)&space_number,
		     NULL, NULL);
	*data = tuple;
	return 0;
error:
	tnt_schema_sval_free(space);
	if (space_string) tnt_mem_free(space_string);
	if (space_number) tnt_mem_free(space_number);
	return -1;
}

int tnt_schema_add_spaces(struct tnt_schema *schema_obj, const char *data,
		          uint32_t size) {
	struct mh_assoc_t *schema = schema_obj->space_hash;
	const char *tuple = data;
	if (mp_check(&tuple, tuple + size))
		return -1;
	tuple = data;
	if (mp_typeof(*tuple) != MP_ARRAY)
		return -1;
	uint32_t space_count = mp_decode_array(&tuple);
	while (space_count-- > 0) {
		if (tnt_schema_add_space(schema, &tuple))
			return -1;
	}
	return 0;
}

static inline int
tnt_schema_add_index(struct mh_assoc_t *schema, const char **data) {
	const struct tnt_schema_sval *space = NULL;
	struct tnt_schema_ival *index = NULL;
	struct assoc_val *index_number = NULL, *index_string = NULL;
	const char *tuple = *data;
	if (mp_typeof(*tuple) != MP_ARRAY) goto error;
	int64_t tuple_len = mp_decode_array(&tuple);
	if (tuple_len < 6) goto error;
	uint32_t space_number = mp_decode_uint(&tuple);
	if (mp_typeof(*tuple) != MP_UINT) goto error;
	struct assoc_key space_key = {
		(void *)&(space_number),
		sizeof(uint32_t)
	};
	mh_int_t space_slot = mh_assoc_find(schema, &space_key, NULL);
	if (space_slot == mh_end(schema))
		return -1;
	space = (*mh_assoc_node(schema, space_slot))->data;
	index = tnt_mem_alloc(sizeof(struct tnt_schema_ival));
	if (!index) goto error;
	memset(index, 0, sizeof(struct tnt_schema_ival));
	if (mp_typeof(*tuple) != MP_UINT) goto error;
	index->index_number = mp_decode_uint(&tuple);
	if (mp_typeof(*tuple) != MP_STR) goto error;
	const char *index_name_tmp = mp_decode_str(&tuple,
			                           &index->index_name_len);
	index->index_name = tnt_mem_alloc(index->index_name_len);
	if (!index->index_name) goto error;
	memcpy(index->index_name, index_name_tmp, index->index_name_len);
	/* skip index type */
	mp_next(&tuple);
	/* skip unique flag */
	mp_next(&tuple);
	/* skip fields */
	mp_next(&tuple);
	/*
	uint32_t part_count = mp_decode_uint(&tuple);
	if (mp_typeof(*tuple) != MP_UINT) goto error;
	uint32_t rpart_count = part_count;
	index->index_parts = tnt_mem_alloc(part_count *
					   sizeof(struct tnt_schema_fval));
	if (!index->index_parts) goto error;
	memset(index->index_parts, 0,
	       part_count * sizeof(struct tnt_schema_fval));
	if (tuple_len - part_count * 2 != 6) goto error;
	while (part_count--) {
		struct tnt_schema_fval *val =
			&(index->index_parts[rpart_count - part_count - 1]);
		if (mp_typeof(*tuple) != MP_UINT) goto error;
		val->field_number = mp_decode_uint(&tuple);
		uint32_t sfield_len = 0;
		if (mp_typeof(*tuple) != MP_STR) goto error;
		const char *sfield = mp_decode_str(&tuple, &sfield_len);
		switch(*sfield) {
			case ('s'):
			case ('S'):
				val->field_type = FT_STR;
				break;
			case ('n'):
			case ('N'):
				val->field_type = FT_NUM;
				break;
			default:
				val->field_type = FT_OTHER;
		}
		index->index_parts_len++;
	} */
	index_string = tnt_mem_alloc(sizeof(struct assoc_val));
	if (!index_string) goto error;
	index_string->key.id     = index->index_name;
	index_string->key.id_len = index->index_name_len;
	index_string->data = index;
	index_number = tnt_mem_alloc(sizeof(struct assoc_val));
	if (!index_number) goto error;
	index_number->key.id     = (void *)&(index->index_number);
	index_number->key.id_len = sizeof(uint32_t);
	index_number->data = index;
	mh_assoc_put(space->index_hash,
		     (const struct assoc_val **)&index_string,
		     NULL, NULL);
	mh_assoc_put(space->index_hash,
		     (const struct assoc_val **)&index_number,
		     NULL, NULL);
	*data = tuple;
	return 0;
error:
	if (index_string) tnt_mem_free(index_string);
	if (index_number) tnt_mem_free(index_number);
	tnt_schema_ival_free(index);
	return -1;
}

int tnt_schema_add_indexes(struct tnt_schema *schema_obj, const char *data,
		           uint32_t size) {
	struct mh_assoc_t *schema = schema_obj->space_hash;
	const char *tuple = data;
	if (mp_check(&tuple, tuple + size))
		return -1;
	tuple = data;
	if (mp_typeof(*tuple) != MP_ARRAY)
		return -1;
	uint32_t space_count = mp_decode_array(&tuple);
	while (space_count-- > 0) {
		if (tnt_schema_add_index(schema, &tuple))
			return -1;
	}
	return 0;
}

int32_t tnt_schema_stosid(
		struct tnt_schema *schema_obj,
		const char *space_name, uint32_t space_name_len) {
	struct mh_assoc_t *schema = schema_obj->space_hash;
	struct assoc_key space_key = {space_name, space_name_len};
	mh_int_t space_slot = mh_assoc_find(schema, &space_key, NULL);
	if (space_slot == mh_end(schema))
		return -1;
	const struct tnt_schema_sval *space =
		(*mh_assoc_node(schema, space_slot))->data;
	return space->space_number;
}

int32_t tnt_schema_stoiid(
		struct tnt_schema *schema_obj, uint32_t sid,
		const char *index_name, uint32_t index_name_len) {
	struct mh_assoc_t *schema = schema_obj->space_hash;
	struct assoc_key space_key = {(void *)&sid, sizeof(uint32_t)};
	mh_int_t space_slot = mh_assoc_find(schema, &space_key, NULL);
	if (space_slot == mh_end(schema))
		return -1;
	const struct tnt_schema_sval *space =
		(*mh_assoc_node(schema, space_slot))->data;
	struct assoc_key index_key = {index_name, index_name_len};
	mh_int_t index_slot = mh_assoc_find(space->index_hash, &index_key, NULL);
	if (index_slot == mh_end(space->index_hash))
		return -1;
	const struct tnt_schema_ival *index =
		(*mh_assoc_node(space->index_hash, index_slot))->data;
	return index->index_number;
}

struct tnt_schema *tnt_schema_new(struct tnt_schema *s) {
	int alloc = (s == NULL);
	if (!s) {
		s = tnt_mem_alloc(sizeof(struct tnt_schema));
		if (!s) return NULL;
	}
	s->space_hash = mh_assoc_new();
	s->alloc = alloc;
	return s;
}

void tnt_schema_flush(struct tnt_schema *obj) {
	tnt_schema_space_free(obj->space_hash);
}

void tnt_schema_delete(struct tnt_schema *obj) {
	tnt_schema_space_free(obj->space_hash);
	mh_assoc_delete(obj->space_hash);
}

ssize_t
tnt_get_spaces(struct tnt_stream *s, char *name, uint32_t name_len)
{
	struct tnt_stream *obj = tnt_object(NULL);
	tnt_object_add_array(obj, 1);
	tnt_object_add_str(obj, name, name_len);
	ssize_t retval = tnt_select(s, tnt_vsp_space, tnt_vin_name,
				    UINT32_MAX, 0, TNT_ITER_EQ, obj);
	tnt_stream_free(obj);
	return retval;
}

ssize_t
tnt_get_index(struct tnt_stream *s, char *name, uint32_t name_len)
{
	struct tnt_stream *obj = tnt_object(NULL);
	tnt_object_add_array(obj, 1);
	tnt_object_add_str(obj, name, name_len);
	ssize_t retval = tnt_select(s, tnt_vsp_index, tnt_vin_name,
				    UINT32_MAX, 0, TNT_ITER_EQ, obj);
	tnt_stream_free(obj);
	return retval;
}
