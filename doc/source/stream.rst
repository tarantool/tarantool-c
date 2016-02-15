-------------------------------------------------------------------------------
                             Basic stream
-------------------------------------------------------------------------------

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

.. c:function:: void tnt_stream_free(struct tnt_stream *s)

    Common free function to all stream objects.

.. c:function:: uint32_t tnt_stream_reqid(struct tnt_stream *s, uint32_t reqid)

    Change rquest id that'll be used to construct query and return the previous
    one.

=====================================================================
                              Example
=====================================================================

.. literalinclude:: example.c
    :language: c
    :lines: 350
