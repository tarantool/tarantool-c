#ifndef TNT_ITER_H_INCLUDED
#define TNT_ITER_H_INCLUDED

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

/**
 * \file tnt_iter.h
 * \brief Custom iterator types (msgpack/reply)
 */

/*!
 * iterator types
 */
enum tnt_iter_type {
	TNT_ITER_ARRAY,
	TNT_ITER_MAP,
	TNT_ITER_REPLY,
//	TNT_ITER_REQUEST,
//	TNT_ITER_STORAGE
};

/*!
 * \brief msgpack array iterator
 */
struct tnt_iter_array {
	const char *data; /*!< pointer to the beginning of array */
	const char *first_elem; /*!< pointer to the first element of array */
	const char *elem; /*!< pointer to current element of array */
	const char *elem_end; /*!< pointer to current element end of array */
	uint32_t elem_count; /*!< number of elements in array */
	int cur_index; /*!< index of current element */
};

/* msgpack array iterator accessors */

/**
 * \brief access msgpack array iterator
 */
#define TNT_IARRAY(I) (&(I)->data.array)

/**
 * \brief access current element form iterator
 */
#define TNT_IARRAY_ELEM(I) TNT_IARRAY(I)->elem

/**
 * \brief access end of current element from iterator
 */
#define TNT_IARRAY_ELEM_END(I) TNT_IARRAY(I)->elem_end

/*!
 * \brief msgpack map iterator
 */
struct tnt_iter_map {
	const char *data; /*!< pointer to the beginning of map */
	const char *first_key; /*!< pointer to the first key of map */
	const char *key; /*!< pointer to current key of map */
	const char *key_end; /*!< pointer to current key end */
	const char *value; /*!< pointer to current value of map */
	const char *value_end; /*!< pointer to current value end */
	uint32_t pair_count; /*!< number of key-values pairs in array */
	int cur_index; /*!< index of current pair */
};

/* msgpack array iterator accessors */

/**
 * \brief access msgpack map iterator
 */
#define TNT_IMAP(I) (&(I)->data.map)

/**
 * \brief access current key from iterator
 */
#define TNT_IMAP_KEY(I) TNT_IMAP(I)->key

/**
 * \brief access current key end from iterator
 */
#define TNT_IMAP_KEY_END(I) TNT_IMAP(I)->key_end

/**
 * \brief access current value from iterator
 */
#define TNT_IMAP_VAL(I) TNT_IMAP(I)->value

/**
 * \brief access current value end from iterator
 */
#define TNT_IMAP_VAL_END(I) TNT_IMAP(I)->value_end

/*!
 * \brief reply iterator
 */
struct tnt_iter_reply {
	struct tnt_stream *s; /*!< stream pointer */
	struct tnt_reply r;   /*!< current reply */
};

/* reply iterator accessors */

/**
 * \brief access reply iterator
 */
#define TNT_IREPLY(I) (&(I)->data.reply)

/**
 * \brief access current reply form iterator
 */
#define TNT_IREPLY_PTR(I) &TNT_IREPLY(I)->r

/* request iterator */
// struct tnt_iter_request {
// 	struct tnt_stream *s; /* stream pointer */
// 	struct tnt_request r; /* current request */
// };

/* request iterator accessors */
// #define TNT_IREQUEST(I) (&(I)->data.request)
// #define TNT_IREQUEST_PTR(I) &TNT_IREQUEST(I)->r
// #define TNT_IREQUEST_STREAM(I) TNT_IREQUEST(I)->s

/* storage iterator */
// struct tnt_iter_storage {
// 	struct tnt_stream *s; /* stream pointer */
// 	struct tnt_tuple t;   /* current fetched tuple */
// };

/* storage iterator accessors */
// #define TNT_ISTORAGE(I) (&(I)->data.storage)
// #define TNT_ISTORAGE_TUPLE(I) &TNT_ISTORAGE(I)->t
// #define TNT_ISTORAGE_STREAM(I) TNT_ISTORAGE(I)->s

/**
 * \brief iterator status
 */
enum tnt_iter_status {
	TNT_ITER_OK, /*!< iterator is ok */
	TNT_ITER_FAIL /*!< error or end of iteration */
};

/**
 * \brief Common iterator object
 */
struct tnt_iter {
	enum tnt_iter_type type; /*!< iterator type
				  * \sa enum tnt_iter_type
				  */
	enum tnt_iter_status status; /*!< iterator status
				      * \sa enum tnt_iter_status
				      */
	int alloc; /*!< allocation mark */
	/* interface callbacks */
	int  (*next)(struct tnt_iter *iter); /*!< callback for next element */
	void (*rewind)(struct tnt_iter *iter); /*!< callback for rewind */
	void (*free)(struct tnt_iter *iter); /*!< callback for free of custom iter type */
	/* iterator data */
	union {
		struct tnt_iter_array array; /*!< msgpack array iterator */
		struct tnt_iter_map map; /*!< msgpack map iterator */
		struct tnt_iter_reply reply; /*!< reply iterator */
//		struct tnt_iter_request request;
//		struct tnt_iter_storage storage;
	} data;
};

/**
 * \brief create msgpack array iterator from object
 *
 * if iterator pointer is NULL, then new iterator will be created.
 *
 * \param i pointer to allocated structure
 * \param s tnt_object/tnt_buf instance with array to traverse
 *
 * \returns iterator pointer
 * \retval  NULL on error.
 */
struct tnt_iter *
tnt_iter_array_object(struct tnt_iter *i, struct tnt_stream *s);

/**
 * \brief create msgpack array iterator from pointer
 *
 * if iterator pointer is NULL, then new iterator will be created.
 *
 * \param i pointer to allocated structure
 * \param data pointer to data with array
 * \param size size of data (may be more, it won't go outside)
 *
 * \returns iterator pointer
 * \retval  NULL on error.
 */
struct tnt_iter *
tnt_iter_array(struct tnt_iter *i, const char *data, size_t size);

/**
 * \brief create msgpack map iterator from object
 *
 * if iterator pointer is NULL, then new iterator will be created.
 *
 * \param i pointer to allocated structure
 * \param s tnt_object/tnt_buf instance with map to traverse
 *
 * \returns iterator pointer
 * \retval  NULL error.
 */
struct tnt_iter *
tnt_iter_map_object(struct tnt_iter *i, struct tnt_stream *s);

/**
 * \brief create msgpack map iterator from pointer
 *
 * if iterator pointer is NULL, then new iterator will be created.
 *
 * \param i pointer to allocated structure
 * \param data pointer to data with map
 * \param size size of data (may be more, it won't go outside)
 *
 * \returns iterator pointer
 * \retval  NULL error.
 */
struct tnt_iter *
tnt_iter_map(struct tnt_iter *i, const char *data, size_t size);

/**
 * \brief create and initialize tuple reply iterator;
 *
 * \param i pointer to allocated structure
 * \param s tnt_net stream pointer
 *
 * if stream iterator pointer is NULL, then new stream
 * iterator will be created.
 *
 * \returns stream iterator pointer
 * \retval NULL error.
*/
struct tnt_iter *
tnt_iter_reply(struct tnt_iter *i, struct tnt_stream *s);

// struct tnt_iter *tnt_iter_request(struct tnt_iter *i, struct tnt_stream *s);
// struct tnt_iter *tnt_iter_storag(struct tnt_iter *i, struct tnt_stream *s);

/**
 * \brief free iterator.
 *
 * \param i iterator pointer
 */
void
tnt_iter_free(struct tnt_iter *i);

/**
 * \brief iterate to next element in tuple
 *
 * \param i iterator pointer
 *
 * depend on iterator tuple, sets to the
 * next msgpack field or next response in the stream.
 *
 * \retval 0 end of iteration
 * \retval 1 next step of iteration
 */
int
tnt_next(struct tnt_iter *i);

/**
 * \brief reset iterator pos to beginning
 *
 * \param i iterator pointer
 */
void
tnt_rewind(struct tnt_iter *i);


#endif /* TNT_ITER_H_INCLUDED */
