-------------------------------------------------------------------------------
                        Using the basic stream object
-------------------------------------------------------------------------------

=====================================================================
                            Basic type
=====================================================================

.. c:type:: struct tnt_stream

    .. code-block:: c

        struct tnt_stream {
            int alloc;
            ssize_t (* write)(struct tnt_stream * s, const char * buf, size_t size);
            ssize_t (* writev)(struct tnt_stream * s, struct iovec * iov, int count);
            ssize_t (* read)(struct tnt_stream * s, char * buf, size_t size);
            int (* read_reply)(struct tnt_stream * s, struct tnt_reply * r);
            void (* free)(struct tnt_stream * s);
            void * data;
            uint32_t wrcnt;
            uint64_t reqid;
        };

.. c:member:: int tnt_stream.alloc

    Allocation mark, equals zero if memory isn't allocated for the stream
    object. Otherwise not zero.

.. c:member:: void * tnt_stream.data

    Subclass data.

.. c:member:: uint_32t tnt_stream.wrcnt

    Counter of write operations.

.. c:member:: uint_64t tnt_stream.reqid

    Request ID number of a current operation.
    Incremented at every request compilation. Default is zero.

=====================================================================
                    Working with a stream
=====================================================================

.. c:function:: void tnt_stream_free(struct tnt_stream *s)

    Common free function for all stream objects.

.. c:function:: uint32_t tnt_stream_reqid(struct tnt_stream *s, uint32_t reqid)

    Increment and set the request ID that'll be used to construct a query,
    and return the previous request ID.

.. c:function:: ssize_t tnt_stream.write(struct tnt_stream * s, const char * buf, size_t size)
                ssize_t tnt_stream.writev(struct tnt_stream * s, struct iovec * iov, int count)

    Write a string or a vector to a stream.
    Here ``size`` is the string's size (in bytes), and ``count`` is the number
    of records in the ``iov`` vector.

.. c:function:: ssize_t tnt_stream.read(struct tnt_stream * s, char * buf, size_t size)
                int tnt_stream.read_reply(struct tnt_stream * s, struct tnt_reply * r)

    Read a reply from server. The :func:`read` function simply writes the reply
    to a string buffer, while the :func:`read_reply` function parses it and
    writes to a ``tnt_reply`` data structure.

..  // Examples are commented out for a while as we currently revise them.
..  =====================================================================
..                             Example
..  =====================================================================

  .. literalinclude:: example.c
      :language: c
      :lines: 350
