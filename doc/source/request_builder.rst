-------------------------------------------------------------------------------
                               Building requests
-------------------------------------------------------------------------------

.. c:type:: enum tnt_iterator_t

    Custom iterator type

    * **TNT_ITER_EQ** - Equiality iterator
    * **TNT_ITER_REQ** - Revers equality iterator
    * **TNT_ITER_ALL** - Receive all elements
    * **TNT_ITER_LT** - Less than iterator
    * **TNT_ITER_LE** - Less or equal
    * **TNT_ITER_GE** - Greater or equal
    * **TNT_ITER_GT** - Greater than
    * **TNT_ITER_BITS_ALL_SET** - all specified bits are set (bitset specific)
    * **TNT_ITER_BITS_ANY_SET** - any specified bits are set (bitset specific)
    * **TNT_ITER_BITS_ALL_NOT_SET** - all specified bits are not (bitset specific)
    * **TNT_ITER_OVERLAP** - Search for tuples with overlapping rectangles (r-tree specific)
    * **TNT_ITER_NEIGHBOR** - Search of nearest neighbour (r-tree specific)

.. c:type:: enum tnt_request_type

    * **TNT_OP_SELECT**
    * **TNT_OP_INSERT**
    * **TNT_OP_REPLACE**
    * **TNT_OP_UPDATE**
    * **TNT_OP_DELETE**
    * **TNT_OP_CALL**
    * **TNT_OP_AUTH**
    * **TNT_OP_EVAL**
    * **TNT_OP_PING**

.. c:type:: struct tnt_request

    Base request object. It contains every part of request.]

    .. code-block:: c

        struct tnt_request {
            struct {
                uint32_t sync;
                enum tnt_request_type type;
            } hdr;
            uint32_t space_id;
            uint32_t index_id;
            uint32_t offset;
            uint32_t limit;
            enum tnt_iterator_t iterator;
            const char * key;
            const char * key_end;
            struct tnt_stream * key_object;
            const char * tuple;
            const char * tuple_end;
            struct tnt_stream * tuple_object;
            int index_base;
            int alloc;
            struct tnt_stream * stream;
        };

=====================================================================
                           Creation
=====================================================================

.. c:function:: struct tnt_request *tnt_request_init(struct tnt_request *req)

    Allocate and initialize request

.. c:function:: struct tnt_request *tnt_request_select(struct tnt_request *req)
                struct tnt_request *tnt_request_insert(struct tnt_request *req)
                struct tnt_request *tnt_request_replace(struct tnt_request *req)
                struct tnt_request *tnt_request_update(struct tnt_request *req)
                struct tnt_request *tnt_request_delete(struct tnt_request *req)
                struct tnt_request *tnt_request_call(struct tnt_request *req)
                struct tnt_request *tnt_request_auth(struct tnt_request *req)
                struct tnt_request *tnt_request_eval(struct tnt_request *req)
                struct tnt_request *tnt_request_upsert(struct tnt_request *req)
                struct tnt_request *tnt_request_ping(struct tnt_request *req)

    Shortcuts for initialization of requests

=====================================================================
                    Set/get request fields
=====================================================================

.. c:member:: uint32_t tnt_request.hdr.sync

    Number of request. It's set automatically when request is compiled.

.. c:member:: enum tnt_request_type tnt_request.hdr.type

    Type of request.

.. c:member:: uint32_t tnt_request.space_id
              uint32_t tnt_request.index_id
              uint32_t tnt_request.offset
              uint32_t tnt_request.limit
              int tnt_request.index_base

    Field with specified values.

.. c:member:: enum tnt_iterator_t tnt_request.iterator

    Set request iterator type.

.. c:function:: int tnt_request_set_key(struct tnt_request *req, struct tnt_stream *obj)
                int tnt_request_set_key_format(struct tnt_request *req, const char *fmt, ...)

    Set key from stream object or make new key using :func:`tnt_object_vformat`.

    Returns ``-1`` if ``tnt_object_vformat`` fails.

.. c:function:: int tnt_request_set_tuple(struct tnt_request *req, struct tnt_stream *obj)
                int tnt_request_set_tuple_format(struct tnt_request *req, const char *fmt, ...)

    Set tuple from stream object or make new tuple using :func:`tnt_object_vformat`.

    * If operation is ``update``, then tuple is stream object with operations.
    * If operation is ``eval``/``call``, then tuple is stream object with arguments.

    Returns ``-1`` if ``tnt_object_vformat`` fails.

.. c:function:: int tnt_request_set_expr (struct tnt_request *req, const char *expr, size_t len)
                int tnt_request_set_exprz(struct tnt_request *req, const char *expr)

    Set expression for ``eval`` from string. If function ``<...>_exprz`` is
    used, then length is calculated using ``strlen(str)``.

    Returns ``-1`` if called not in ``expr`` command.

.. c:function:: int tnt_request_set_func (struct tnt_request *req, const char *func, size_t len)
                int tnt_request_set_funcz(struct tnt_request *req, const char *func)

    Set function for ``call`` from string. If function ``<...>_funcz`` is used,
    then length is calculated using ``strlen(str)``.

    Returns ``-1`` if called not in ``func`` command.

.. c:function:: int tnt_request_set_ops(struct tnt_request *req, struct tnt_stream *s)

    Set operations for ``update`` or ``upsert`` from stream object.

=====================================================================
                       Manipulating requests
=====================================================================

.. c:function:: tnt_request_compile(struct tnt_stream *s, struct tnt_request *req)

    Compile request into stream. If ``tnt_request_encode`` is used, then request
    is compiled into stream, that's pinned to it.

    Returns ``-1`` if bad command or can't write to stream or stream is not pinned.

.. c:function:: tnt_request_free(struct tnt_request *req)

    Free request object.

=====================================================================
                              Example
=====================================================================

.. literalinclude:: example.c
    :language: c
    :lines: 157,171-174

.. literalinclude:: example.c
    :language: c
    :lines: 187-202

.. literalinclude:: example.c
    :language: c
    :lines: 225-226,230-250,255-259

.. literalinclude:: example.c
    :language: c
    :lines: 279,281-293,298-306