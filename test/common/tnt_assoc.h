#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

struct assoc_key {
	const char *id;
	uint32_t id_len;
};

struct assoc_val {
	struct assoc_key key;
	void *data;
};

static inline int
mh_cmp_eq(
		const struct assoc_val **lval,
		const struct assoc_val **rval,
		void *arg) {
	(void )arg;
	if ((*lval)->key.id_len != (*rval)->key.id_len) return 0;
	return !memcmp((*lval)->key.id, (*rval)->key.id, (*rval)->key.id_len);
}

static inline int
mh_cmp_key_eq(
		const struct assoc_key *key,
		const struct assoc_val **val,
		void *arg) {
	(void )arg;
	if (key->id_len != (*val)->key.id_len) return 0;
	return !memcmp(key->id, (*val)->key.id, key->id_len);
}

static inline void *
tnt_mem_calloc(size_t count, size_t size) {
	size_t sz = count * size;
	void *alloc = tnt_mem_alloc(sz);
	if (!alloc) return 0;
	memset(alloc, 0, sz);
	return alloc;

}

#define MH_INCREMENTAL_RESIZE 1

#define mh_arg_t void *

#define mh_eq(a, b, arg)      mh_cmp_eq(a, b, arg)
#define mh_eq_key(a, b, arg)  mh_cmp_key_eq(a, b, arg)
#define mh_hash(x, arg)       PMurHash32(MUR_SEED, (*x)->key.id, (*x)->key.id_len)
#define mh_hash_key(x, arg)   PMurHash32(MUR_SEED, (x)->id, (x)->id_len);

#define mh_node_t struct assoc_val *
#define mh_key_t  const struct assoc_key *

#define MH_CALLOC(x, y) tnt_mem_calloc((x), (y))
#define MH_FREE(x)      tnt_mem_free((x))

#define mh_name               _assoc
#define MUR_SEED 13
#include		      <PMurHash.h>
#include                      <mhash.h>

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */
