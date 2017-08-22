#ifndef TNT_EXECUTE_H_INCLUDED
#define TNT_EXECUTE_H_INCLUDED

/**
 * \file tnt_execute.h
 * \brief SQL execution request
 */

/**
 * \brief Construct SQL request and write it into stream
 *
 * \param s    stream object to write request to
 * \param expr SQL query string
 * \param elen query length
 * \param args tnt_object instance with messagepack array with params
 *             to bind to the request
 *
 * \retval number of bytes written to stream
 */
ssize_t
tnt_execute(struct tnt_stream *s, const char *expr, size_t elen,
	    struct tnt_stream *params);

#endif /* TNT_EXECUTE_H_INCLUDED */