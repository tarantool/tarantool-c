-------------------------------------------------------------------------------
                             Reply parsing
-------------------------------------------------------------------------------

.. c:type:: struct tnt_reply

    Base reply object.

    .. code-block:: c

        struct tnt_reply {
            int alloc;
            uint64_t bitmap;
            const char * buf;
            size_t buf_size;
            uint64_t code;
            uint64_t sync;
            uint64_t schema_id;
            const char * error;
            const char * error_end;
            const char * data;
            const char * data_end;
        };

    If ``code > 0``, then ``error`` and ``error_end`` must be set.
    Otherwise ``data`` and ``data_end`` is set. ``schema_id`` is number of
    :ref:`schema` revision.

.. c:member:: uint64_t tnt_reply.code

    Return code of query. If ``code == 0`` then it's ok, otherwise use
    :macro:`TNT_REPLY_ERROR` to convert it to errorcode.

.. c:member:: uint64_t tnt_reply.sync

    Sync of query.

.. c:member:: uint64_t tnt_reply.schema_id

    Schema id of query.

.. c:member:: const char *tnt_reply.data
              const char *tnt_reply.data_end

    Query data. Msgpack object.

    Parse it with any msgpack library (e.g. msgpuck).

.. c:member:: const char *tnt_reply.error
              const char *tnt_reply.error_end

    Pointer to error string in case of ``code != 0``

.. c:type:: ssize_t (* tnt_reply_t)(void * ptr, char * dst, ssize_t size)

    It's needed for function :func:`tnt_reply_from`.'

.. c:function:: struct tnt_reply *tnt_reply_init(struct tnt_reply *r)

    Initialize reply request.

.. c:function:: void tnt_reply_free(struct tnt_reply *r)

    Free reply request.

.. c:function:: int tnt_reply(struct tnt_reply *r, char *buf, size_t size,
                              size_t *off)

    Parse reply from buffer ``buf``, it must contain full reply. Otherwise
    return count of bytes needed to process in ``off`` variable.

.. c:function:: int tnt_reply_from(struct tnt_reply *r, tnt_reply_t rcv,
                                   void *ptr)

    Parse reply from data, get all needed data from ``rcv`` callback and with
    cb context ``ptr``.

.. c:macro:: TNT_REPLY_ERR(reply)

    Return error number, shifted right.

=====================================================================
                            Example
=====================================================================

