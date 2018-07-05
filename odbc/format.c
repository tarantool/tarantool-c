#include <string>


struct column_def {
	int id;
	int type;
	int is_nullable;
	char *name;
	int index_type;
};

struct format {
	uint32_t count;
	struct column_def *items;
};

enum keys_id {
	TYPE=0,
	ISNULL,
	NAME,

};

struct keys_pairs {
	const char *name;
	int val;
};

int
read_pair(const char** data, struct column_def *col, const char* key, size_t len)
{
	for(const char **name = keys; *name; name++) {
	}

}

int
read_column_def(const char** data, struct column_def *col, int num)
{
	col->id = num;
	if (mp_typeof(**data) != MP_MAP)
		return FAIL;
	int count = mp_decode_map(data);
	for (int i = 0; i < count; ++i) {
		if (mp_typeof(**data) != MP_STR)
			return FAIL;
		size_t key_len;
		const char *key = mp_decode_str(data, &key_len);
		if (read_pair(data, col, key, key_len) == FAIL)
			return FAIL;
	}
	return OK;
}

int
read_format(const char* data, struct format *root)
{
	if (mp_typeof(*data) != MP_ARRAY)
		return FAIL;
	root->count = mp_decode_array(&data);

	if (root->count == 0)
		return OK;
	format->items = (struct format*) malloc(sizeof(struct type_map*)*root->count);
	if (!format->items)
		return FAIL;
	for(int i = 0; i < field_count ; ++i)
		format->items[i] = 0;

	for(int i = 0; i < field_count ; ++i) {
		format->items[i] = (struct column_def*) malloc(sizeof(struct column_def));
		if (!format->items[i])
			goto error;
		if (read_column_def(&data, format->items[i], i) == FAIL)
			goto error;
	}
	return OK;
error:
	free_format(root);
	return FAIL;
}
