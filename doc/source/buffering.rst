-------------------------------------------------------------------------------
                            Buffer for requests
-------------------------------------------------------------------------------

In case you don't need networking, but you need to be able to parse replies or
write requests.

=====================================================================
                        Create buffer
=====================================================================

.. c:function:: struct tnt_stream *tnt_buf(struct tnt_stream *s)

.. c:function:: struct tnt_stream *tnt_buf_as(struct tnt_stream *s, char *buf, size_t buf_len)

=====================================================================
                        Create requests
=====================================================================

It's done with basic functions:

* :c:type:`struct tnt_request`/:func:`tnt_request_compile`
* :func:`tnt_select`/:func:`tnt_insert`/...

=====================================================================
                          Parse response
=====================================================================

Use iterator to iterate through responses or
``stream->read_reply(struct tnt_stream *stream, struct tnt_reply *reply)``.
