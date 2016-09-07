-------------------------------------------------------------------------------
                    Using a buffer for requests
-------------------------------------------------------------------------------

You may need a stream buffer (``tnt_buf``) if you don't need the networking
functionality of ``tnt_net``, but you need to write requests or parse replies.

=====================================================================
                        Creating a buffer
=====================================================================

.. c:function:: struct tnt_stream *tnt_buf(struct tnt_stream *s)

    Create a stream buffer.

.. c:function:: struct tnt_stream *tnt_buf_as(struct tnt_stream *s, char *buf, size_t buf_len)

    Create an immutable stream buffer from the buffer ``buf``. It can be used
    for parsing responses.

=====================================================================
                        Writing requests
=====================================================================

Use the basic functions for building requests:

* :func:`tnt_select` / :func:`tnt_insert` (see ":ref:`working_with_tnt_stream`")
* :c:type:`struct tnt_request` / :func:`tnt_request_compile` (see
  ":ref:`working_with_tnt_request`")

=====================================================================
                        Parsing replies
=====================================================================

Use an iterator to iterate through replies in your stream buffer,
or use the stream's :func:`read_reply` method
(``stream->read_reply(struct tnt_stream *stream, struct tnt_reply *reply)``).
