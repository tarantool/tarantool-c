#include "test.h"

#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include <tarantool/tnt_opt.h>
#include <msgpuck.h>

#include "common.h"

const char template_msgpuck[] = "[{\"m\": {\"m\": [], \"m\": [], \"m\": [], \"m\": [], \"m\": {\"m\": [\"a\", \"a\"]}}}]";
static size_t alloc_size = 0;

static void *
custom_realloc(void *ptr, size_t size)
{
	void *new_ptr;
	if (ptr == NULL) {
		if (size != 0) {
			new_ptr = calloc(1, size + sizeof(size_t *));
			if (new_ptr != NULL) {
				*((size_t *)new_ptr) = size;
				alloc_size += size;
				return (size_t *)new_ptr + 1;
			}
		}
		return NULL;
	}
	if (size) {
		new_ptr = (size_t *)ptr - 1;
		size_t psize = *((size_t *)new_ptr);
		new_ptr = realloc(new_ptr, size + sizeof(size_t *));
		if (new_ptr != NULL) {
			*((size_t *)new_ptr) = size;
			alloc_size += (size - psize);
			return (size_t *)new_ptr + 1;
		}
		return NULL;
	}
	new_ptr = (size_t *)ptr - 1;
	alloc_size -= *((size_t *)new_ptr);
	free(new_ptr);
	return NULL;
}

static int
test_leak(void)
{
	plan(2);
	tnt_mem_init(custom_realloc);
	char data[sizeof(template_msgpuck)] = { 0 };
	struct tnt_stream * stream = tnt_object(NULL);
	tnt_object_add_array(stream, 1);
	tnt_object_add_map(stream, 1);
	tnt_object_add_str(stream, "m", 1);
	tnt_object_add_map(stream, 5);
	tnt_object_add_str(stream, "m", 1);
	tnt_object_add_array(stream, 0);
	tnt_object_add_str(stream, "m", 1);
	tnt_object_add_array(stream, 0);
	tnt_object_add_str(stream, "m", 1);
	tnt_object_add_array(stream, 0);
	tnt_object_add_str(stream, "m", 1);
	tnt_object_add_array(stream, 0);
	tnt_object_add_str(stream, "m", 1);
	tnt_object_add_map(stream, 1);
	tnt_object_add_str(stream, "m", 1);
	tnt_object_add_array(stream, 2);
	tnt_object_add_str(stream, "a", 1);
	tnt_object_add_str(stream, "a", 1);
	mp_snprint(data, sizeof(data), TNT_SBUF_DATA(stream));
	ok(!strcmp(data, template_msgpuck), "Check the created msgpuck");
	tnt_stream_free(stream);
	ok(alloc_size == 0, "Check memory leak absence");
	return check_plan();
}

int main()
{
	plan(1);
	test_leak();
	return check_plan();
}
