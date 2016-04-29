#ifndef TNT_UPDATE_H_INCLUDED
#define TNT_UPDATE_H_INCLUDED

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

#include <tarantool/tnt_stream.h>

/**
 * \file tnt_update.h
 * \brief Update operation
 */

/**
 * \brief Generate and write update operation with predefined
 *
 * \param s     stream pointer
 * \param space space no
 * \param index index no
 * \param key   key to update
 * \param ops   ops to update (tnt_object)
 *
 * \returns count of bytes written
 * \retval  -1 oom
 * \sa tnt_update_cointainer
 * \sa tnt_update_cointainer_close
 */
ssize_t
tnt_update(struct tnt_stream *s, uint32_t space, uint32_t index,
	   struct tnt_stream *key, struct tnt_stream *ops);

/**
 * \brief Add bit operation for update to tnt_object
 *
 * \param ops     operation container
 * \param fieldno field number
 * \param op      operation ('&', '|', '^')
 * \param value   value for update op
 *
 * \returns count of bytes written
 * \retval  -1 oom
 * \sa tnt_update_cointainer
 * \sa tnt_update_cointainer_close
 */
ssize_t
tnt_update_bit(struct tnt_stream *ops, uint32_t fieldno, char op,
	       uint64_t value);

/**
 * \brief Add int arithmetic operation for update to tnt_object
 *
 * \param ops     operation container
 * \param fieldno field number
 * \param op      operation ('+', '-')
 * \param value   value for update op
 *
 * \returns count of bytes written
 * \retval  -1 oom
 * \sa tnt_update_cointainer
 * \sa tnt_update_cointainer_close
 */
ssize_t
tnt_update_arith_int(struct tnt_stream *ops, uint32_t fieldno, char op,
		     int64_t value);

/**
 * \brief Add float arithmetic operation for update to tnt_object
 *
 * \param ops     operation container
 * \param fieldno field number
 * \param op      operation ('+', '-')
 * \param value   value for update op
 *
 * \returns count of bytes written
 * \retval  -1 oom
 * \sa tnt_update_cointainer
 * \sa tnt_update_cointainer_close
 */
ssize_t
tnt_update_arith_float(struct tnt_stream *ops, uint32_t fieldno, char op,
		       float value);

/**
 * \brief Add double arithmetic operation for update to tnt_object
 *
 * \param ops     operation container
 * \param fieldno field number
 * \param op      operation ('+', '-')
 * \param value   value for update op
 *
 * \returns count of bytes written
 * \retval  -1 oom
 * \sa tnt_update_cointainer
 * \sa tnt_update_cointainer_close
 */
ssize_t
tnt_update_arith_double(struct tnt_stream *ops, uint32_t fieldno, char op,
		        double value);

/**
 * \brief Add delete operation for update to tnt_object
 *
 * \param ops     operation container
 * \param fieldno field number
 * \param fieldco field count
 *
 * \returns count of bytes written
 * \retval  -1 oom
 * \sa tnt_update_cointainer
 * \sa tnt_update_cointainer_close
 */
ssize_t
tnt_update_delete(struct tnt_stream *ops, uint32_t fieldno,
		  uint32_t fieldco);

/**
 * \brief Add insert before operation for update to tnt_object
 *
 * \param ops     operation container
 * \param fieldno field number
 * \param val     value to insert (tnt_object)
 *
 * \returns count of bytes written
 * \retval  -1 oom
 * \sa tnt_update_cointainer
 * \sa tnt_update_cointainer_close
 */
ssize_t
tnt_update_insert(struct tnt_stream *ops, uint32_t fieldno,
		  struct tnt_stream *val);

/**
 * \brief Add assign operation for update to tnt_object
 *
 * \param ops     operation container
 * \param fieldno field number
 * \param val     value to assign (tnt_object)
 *
 * \returns count of bytes written
 * \retval  -1 oom
 * \sa tnt_update_cointainer
 * \sa tnt_update_cointainer_close
 */
ssize_t
tnt_update_assign(struct tnt_stream *ops, uint32_t fieldno,
		  struct tnt_stream *val);

/**
 * \brief Add splice operation for update to tnt_object
 *
 * \param ops         operation container
 * \param fieldno     field number
 * \param position    cut from
 * \param offset      number of bytes to cut
 * \param buffer      buffer to insert instead
 * \param buffer_len  buffer length
 *
 * \returns count of bytes written
 * \retval  -1 oom
 * \sa tnt_update_cointainer
 * \sa tnt_update_cointainer_close
 */
ssize_t
tnt_update_splice(struct tnt_stream *ops, uint32_t fieldno,
		  uint32_t position, uint32_t offset,
		  const char *buffer, size_t buffer_len);

/**
 * \brief shortcut for tnt_object() with type == TNT_SBO_SPARSE
 */
struct tnt_stream *tnt_update_container(struct tnt_stream *ops);

/**
 * \brief shortcut for tnt_object_container_close()
 */
struct tnt_stream *
tnt_update_container(struct tnt_stream *ops);

int tnt_update_container_close(struct tnt_stream *ops);

int tnt_update_container_reset(struct tnt_stream *ops);

/**
 * \brief Generate and write upsert operation with predefined
 *
 * \param s     stream pointer
 * \param space space no
 * \param tuple (tnt_object instance) msgpack array with tuple to insert to
 * \param ops   ops to update (tnt_object)
 *
 * \returns count of bytes written
 * \retval  -1 oom
 * \sa tnt_update_cointainer
 * \sa tnt_update_cointainer_close
 */
ssize_t
tnt_upsert(struct tnt_stream *s, uint32_t space,
	   struct tnt_stream *tuple, struct tnt_stream *ops);

#endif /* TNT_UPDATE_H_INCLUDED */
