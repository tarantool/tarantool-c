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

#define mh_arg_t void *

#define mh_eq(a, b, arg)      mh_cmp_eq(a, b, arg)
#define mh_eq_key(a, b, arg)  mh_cmp_key_eq(a, b, arg)
#define mh_hash(x, arg)       PMurHash32(MUR_SEED, (*x)->key.id, (*x)->key.id_len)
#define mh_hash_key(x, arg)   PMurHash32(MUR_SEED, (x)->id, (x)->id_len);

/* type for hash value */
#define mh_node_t struct assoc_val *
/* type for hash key */
#define mh_key_t  const struct assoc_key *

#define MH_CALLOC(x, y) tnt_mem_calloc((x), (y))
#define MH_FREE(x)      tnt_mem_free((x))

#define mh_name               _assoc
#define MUR_SEED 13
#include		      <PMurHash.h>
#include                      <mhash.h>

/*!
 * \file tnt_assoc.h
 */

/*! \struct mh_assoc_t */

/*! \fn void mh_assoc_clear(struct mh_assoc_t *h)
 * \brief Clear an associate array
 * \param h associate array
 */

/*! \fn void mh_assoc_delete(struct mh_assoc_t *h)
 * \brief Free an associate array
 * \param h associate array
 */

/*! \fn void mh_assoc_reserve(struct mh_assoc_t *h, uint32_t size, void *arg)
 * \brief Reserve place for elements
 * \param h associate array
 * \param size count of elements
 * \param arg  context for (eq/hash functions) (must be NULL)
 */

/*! \fn mh_node_t *mh_assoc_node(struct mh_assoc_t *h, uint32_t x)
 * \brief  Access value in specified slot
 * \param  h associate array
 * \param  x slot number
 * \retval value in slot
 */

/*! \fn uint32_t mh_assoc_find(struct mh_assoc_t *h, mh_key_t key, void *arg)
 * \brief  Search for element by key
 * \param  h    associate array
 * \param  key  key to search for
 * \param  arg  context for (eq/hash functions) (must be NULL)
 *
 * \retval != mh_end()   slot number, where element is contained
 * \retval    mh_end()   not found
 *
 * See also: \see mh_assoc_get or \see mh_assoc_find
 */

/*! \fn uint32_t mh_assoc_get(struct mh_assoc_t *h, mh_node_t *node, void *arg)
 * \brief Search for element by value
 * \param  h    associate array
 * \param  node node to search for
 * \param  arg  context for (eq/hash functions) (must be NULL)
 *
 * \retval != mh_end()   slot number, where element is contained
 * \retval    mh_end()   not found
 *
 * See also: \see mh_assoc_get or \see mh_assoc_find
 */

/*! \fn uint32_t mh_assoc_random(struct mh_assoc_t *h, uint32_t rnd)
 * \brief get random slot with existing element
 * \param h   associate array
 * \param rnd random number
 *
 * \retval != mh_end()   pos of the random node
 * \retval    mh_end()   last one
 */

/*! \fn uint32_t mh_assoc_put(struct mh_assoc_t *h, const mh_node_t *node,
 * 			      mh_node_t **ret, void *arg)
 * \brief put element into hash
 * \param h[in]    associate array
 * \param node[in] node to insert
 * \param ret[out] node, that's replaced
 * \param arg[in]  context for (eq/hash functions) (must be NULL)
 *
 * Find a node in the hash and replace it with a new value.
 * Save the old node in ret pointer, if it is provided.
 * If the old node didn't exist, just insert the new node.
 *
 * \retval != mh_end()   pos of the new node, ret is either NULL
 *                       or copy of the old node
 * \retval    mh_end()   out of memory, ret is unchanged.
 */

/*! \fn void mh_assoc_del(struct mh_assoc_t *h, uint32_t x, void *arg)
 * \brief delete element from specified slot
 * \param  h    associate array
 * \param  x    slot to delete element from
 * \param  arg  context for (eq/hash functions) (must be NULL)
 */

/*! \fn void mh_assoc_remove(struct mh_assoc_t *h, mh_node_t node, void *arg)
 * \brief delete specified element from assoc hash
 * \param  h    associate array
 * \param  node node to delete
 * \param  arg  context for (eq/hash functions) (must be NULL)
 */

/*! \fn struct mh_assoc_t *mh_assoc_new()
 * \brief allocate and initialize new associate array
 * \retval !NULL new associate array
 * \retval NULL  memory allocation error
 */

/*! \def mh_first(hash) */
/*! \def mh_next(hash) */
/*! \def mh_end(hash) */
/*! \def mh_foreach(hash, iter) */

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */
