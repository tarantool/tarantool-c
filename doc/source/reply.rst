-------------------------------------------------------------------------------
                            Parsing a reply
-------------------------------------------------------------------------------

=====================================================================
                          Basic type
=====================================================================

.. c:type:: struct tnt_reply

    Base reply object.

    .. code-block:: c

        struct tnt_reply {
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

.. c:member:: const char *tnt_reply.buf

    Pointer to a buffer with the reply data.
    It's needed for the function :func:`tnt_reply`.

.. c:member:: size_t tnt_reply.buf_size

    Size of the buffer with reply data, in bytes.
    It's needed for the function :func:`tnt_reply`.

.. c:member:: uint64_t tnt_reply.code

    The return code of a query.

    If ``code == 1``, then it's ok, but the read buffer was not big enough
    for the reply data, so ``data`` and ``data_end`` are set.

    If ``code == 0`` then it's ok.

    If ``code < 0``, then it's not ok, so ``error`` and ``error_end`` may be
    set. If they are not set, then it's a network error. Use the macro
    :c:macro:`TNT_REPLY_ERROR` to convert it to an error code.

.. c:member:: uint64_t tnt_reply.sync

    Sync of a query. Generated automatically when the query is sent, and so it
    comes back with the reply.

.. c:member:: uint64_t tnt_reply.schema_id

    Schema ID of a query. This is the number of the
    :ref:`schema <working_with_a_schema>` revision.

.. c:member:: const char *tnt_reply.error
              const char *tnt_reply.error_end

    Pointers to an error string in case of ``code != 0``.
    See all error codes in the file
    `/src/box/errcode.h <https://github.com/tarantool/tarantool/blob/1.6/src/box/errcode.h>`_)
    in the main Tarantool project.

.. c:member:: const char *tnt_reply.data
              const char *tnt_reply.data_end

    ``data`` is the processed reply data.
    This is a MessagePack object. Parse it with any msgpack library,
    e.g. ``msgpuck``.

    ``data_end`` is the offset for further reading in case the read buffer
    was not big enough for the reply data.
    Corresponds to the value returned in the ``off`` argument of the
    ``tnt_reply()`` function.

=====================================================================
                     Manipulating a reply
=====================================================================

.. c:function:: struct tnt_reply *tnt_reply_init(struct tnt_reply *r)

    Initialize a reply request.

.. c:function:: void tnt_reply_free(struct tnt_reply *r)

    Free a reply request.

.. c:function:: int tnt_reply(struct tnt_reply *r, char *buf, size_t size, size_t *off)

    Parse ``size`` bytes of an iproto reply from the buffer ``buf`` (it must
    contain a full reply).

    In ``off``, return the number of bytes remaining in the reply (if processed
    all ``size`` bytes), or the number of processed bytes (if processing
    failed).

.. c:function:: int tnt_reply_from(struct tnt_reply *r, tnt_reply_t rcv, void *ptr)

    Parse an iproto reply from the ``rcv`` callback and with the context
    ``ptr``.

.. c:macro:: TNT_REPLY_ERR(reply)

    Return an error code (number, shifted right) converted from
    ``tnt_reply.code``.

..  // Examples are commented out for a while as we currently revise them.
..  =====================================================================
..                             Example
..  =====================================================================

  .. literalinclude:: example.c
      :language: c
      :lines: 159,164-167,179-183

  .. literalinclude:: example.c
      :language: c
      :lines: 209-220
