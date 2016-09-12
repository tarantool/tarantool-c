.. _working_with_tnt_request:

-------------------------------------------------------------------------------
                    Building a request with "tnt_request"
-------------------------------------------------------------------------------

=====================================================================
                      Basic types
=====================================================================

.. _tnt_iterator_types:

.. c:type:: enum tnt_iterator_t

    Custom iterator type. Possible values:

    * **TNT_ITER_EQ** - Equality iterator
    * **TNT_ITER_REQ** - Reverse equality iterator
    * **TNT_ITER_ALL** - Receive all elements
    * **TNT_ITER_LT** - "Less than" iterator
    * **TNT_ITER_LE** - "Less or equal" iterator
    * **TNT_ITER_GE** - "Greater or equal" iterator
    * **TNT_ITER_GT** - "Greater than" iterator
    * **TNT_ITER_BITS_ALL_SET** - All specified bits are set (bitset specific)
    * **TNT_ITER_BITS_ANY_SET** - Any specified bits are set (bitset specific)
    * **TNT_ITER_BITS_ALL_NOT_SET** - All specified bits are not set (bitset
      specific)
    * **TNT_ITER_OVERLAP** - Search for tuples with overlapping rectangles
      (R-tree specific)
    * **TNT_ITER_NEIGHBOR** - Search for the nearest neighbour (R-tree specific)

.. c:type:: enum tnt_request_type

    Request type. Possible values:

    * **TNT_OP_SELECT**
    * **TNT_OP_INSERT**
    * **TNT_OP_REPLACE**
    * **TNT_OP_UPDATE**
    * **TNT_OP_DELETE**
    * **TNT_OP_CALL**
    * **TNT_OP_CALL_16**
    * **TNT_OP_AUTH**
    * **TNT_OP_EVAL**
    * **TNT_OP_PING**

.. c:type:: struct tnt_request

    Base request object. It contains all parts of a request.

    .. code-block:: c

        struct tnt_request {
            struct {
                uint64_t sync;
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
        };

    See field descriptions further in this section.

=====================================================================
                        Creating a request
=====================================================================

.. c:function:: struct tnt_request *tnt_request_init(struct tnt_request *req)

    Allocate and initialize a request.

.. c:function:: struct tnt_request *tnt_request_select(struct tnt_request *req)
                struct tnt_request *tnt_request_insert(struct tnt_request *req)
                struct tnt_request *tnt_request_replace(struct tnt_request *req)
                struct tnt_request *tnt_request_update(struct tnt_request *req)
                struct tnt_request *tnt_request_delete(struct tnt_request *req)
                struct tnt_request *tnt_request_call(struct tnt_request *req)
                struct tnt_request *tnt_request_call_16(struct tnt_request *req)
                struct tnt_request *tnt_request_auth(struct tnt_request *req)
                struct tnt_request *tnt_request_eval(struct tnt_request *req)
                struct tnt_request *tnt_request_upsert(struct tnt_request *req)
                struct tnt_request *tnt_request_ping(struct tnt_request *req)

    Shortcuts for allocating and initializing requests of specific types.

=====================================================================
                      Request header
=====================================================================

.. c:member:: uint64_t tnt_request.hdr.sync

    Sync ID number of a request. Generated automatically when the request is
    compiled.

.. c:member:: enum tnt_request_type tnt_request.hdr.type

    Type of a request.

=====================================================================
                   User-defined request fields
=====================================================================

.. c:member:: uint32_t tnt_request.space_id
              uint32_t tnt_request.index_id
              uint32_t tnt_request.offset
              uint32_t tnt_request.limit

    Space and index ID numbers, offset and limit for SELECT (specified in
    records).

=====================================================================
                Set/get request fields and functions
=====================================================================

.. c:function:: int tnt_request_set_iterator(struct tnt_request *req, enum tnt_iterator_t iter)

    Set an iterator type for SELECT.

    Field that is set in ``tnt_request``:

    .. code-block:: c

        enum tnt_iterator_t iterator;

.. c:function:: int tnt_request_set_key(struct tnt_request *req, struct tnt_stream *s)
                int tnt_request_set_key_format(struct tnt_request *req, const char *fmt, ...)

    Set a key (both key start and end) for SELECT/UPDATE/DELETE from a stream
    object.

    Or set a key using the print-like function :func:`tnt_object_vformat`.
    Take ``fmt`` format string followed by arguments for the format string.
    Return ``-1`` if the :func:`tnt_object_vformat` function fails.

    Fields that are set in ``tnt_request``:

    .. code-block:: c

        const char * key;
        const char * key_end;
        struct tnt_stream * key_object; // set by `tnt_request_set_key_format`

.. c:function:: int tnt_request_set_tuple(struct tnt_request *req, struct tnt_stream *obj)
                int tnt_request_set_tuple_format(struct tnt_request *req, const char *fmt, ...)

    Set a tuple (both tuple start and end) for UPDATE/EVAL/CALL from a stream.

    Or set a tuple using the print-like function :func:`tnt_object_vformat`.
    Take ``fmt`` format string followed by arguments for the format string.
    Return ``-1`` if the :func:`tnt_object_vformat` function fails.

    * For UPDATE, the tuple is a stream object with operations.
    * For EVAL/CALL, the tuple is a stream object with arguments.

    Fields that are set in ``tnt_request``:

    .. code-block:: c

        const char * tuple;
        const char * tuple_end;
        struct tnt_stream * tuple_object;  // set by `tnt_request_set_tuple_format`

.. c:function:: int tnt_request_set_expr (struct tnt_request *req, const char *expr, size_t len)
                int tnt_request_set_exprz(struct tnt_request *req, const char *expr)

    Set an expression (both expression start and end) for EVAL from a string.

    If the function ``<...>_exprz`` is used, then length is calculated using
    :func:`strlen(str)`. Otherwise, ``len`` is the expression's length (in
    bytes).

    Return ``-1`` if ``expr`` is not :func:`tnt_request_evaluate`.

    Fields that are set in ``tnt_request``:

    .. code-block:: c

        const char * key;
        const char * key_end;
        struct tnt_stream * key_object; // set by `tnt_request_set_exprz`

.. c:function:: int tnt_request_set_func (struct tnt_request *req, const char *func, size_t len)
                int tnt_request_set_funcz(struct tnt_request *req, const char *func)

    Set a function (both function start and end) for CALL from a string.

    If the function ``<...>_funcz`` is used, then length is calculated using
    :func:`strlen(str)`. Otherwise, ``len`` is the function's length (in bytes).

    Return ``-1`` if ``func`` is not :func:`tnt_request_call`.

    Fields that are set in ``tnt_request``:

    .. code-block:: c

        const char * key;
        const char * key_end;
        struct tnt_stream * key_object; // set by `tnt_request_set_funcz`

.. c:function:: int tnt_request_set_ops(struct tnt_request *req, struct tnt_stream *s)

    Set operations (both operations start and end) for UPDATE/UPSERT from a
    stream.

    Fields that are set in ``tnt_request``:

    .. code-block:: c

        const char * key;
        const char * key_end;

.. c:function:: int tnt_request_set_index_base(struct tnt_request *req, uint32_t index_base)

    Set an index base (field offset) for UPDATE/UPSERT.

    Field that is set in ``tnt_request``:

    .. code-block:: c

        int index_base;

=====================================================================
                       Manipulating a request
=====================================================================

.. c:function:: uint64_t tnt_request_compile(struct tnt_stream *s, struct tnt_request *req)

    Compile a request into a stream.

    Return ``-1`` if bad command or can't write to stream.

.. c:function:: void tnt_request_free(struct tnt_request *req)

    Free a request object.

..  // Examples are commented out for a while as we currently revise them.
..  =====================================================================
..                             Example
..  =====================================================================

  Examples here are common for building requests with both ``tnt_stream`` and
  ``tnt_request`` objects.

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
