-------------------------------------------------------------------------------
                             Simple request creation
-------------------------------------------------------------------------------

Every command in this section will append command to ``tnt_net`` buffer. To send
query to server use :func:`tnt_flush` and then ``tnt->read_reply``, for example.

.. c:function:: ssize_t tnt_auth(struct tnt_stream *s, const char *user, int ulen, const char *pass, int plen)
                ssize_t tnt_deauth(struct tnt_stream *s)

    Create authentication request.

    ``tnt_deauth`` is a shortcut for ``tnt_auth(s, NULL, 0, NULL, 0)``.

.. c:function:: ssize_t tnt_call(struct tnt_stream *s, const char *proc, size_t plen, struct tnt_stream *args)
                ssize_t tnt_eval(struct tnt_stream *s, const char *expr, size_t elen, struct tnt_stream *args)

    Create a call/eval request.

.. c:function:: ssize_t tnt_delete(struct tnt_stream *s, uint32_t space, uint32_t index, struct tnt_stream *key)

    Create a delete request.

.. c:function:: ssize_t tnt_insert(struct tnt_stream *s, uint32_t space, struct tnt_stream *tuple)
                ssize_t tnt_replace(struct tnt_stream *s, uint32_t space, struct tnt_stream *tuple)

    Create an insert/replace request.

.. c:function:: ssize_t tnt_ping(struct tnt_stream *s)

    Create a ping request.

.. c:function:: ssize_t tnt_select(struct tnt_stream *s, uint32_t space, uint32_t index, uint32_t limit, uint32_t offset, uint8_t iterator, struct tnt_stream *key)

    Create select request. For gathering all results - use ``UINT32_MAX`` as limit.

=====================================================================
                      Working with Update
=====================================================================

.. c:function:: ssize_t tnt_update(struct tnt_stream *s, uint32_t space, uint32_t index,
                                   struct tnt_stream *key, struct tnt_stream *ops)

    Basic function for adding Update operation to stream.

    ``ops`` must be container gained with :func:`tnt_update_container`

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                   Container for operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. c:function:: struct tnt_stream *tnt_update_container(struct tnt_stream *ops)

    Create update container.

.. c:function:: int tnt_update_container_close(struct tnt_stream *ops)

    Finish working with container

.. c:function:: int tnt_update_container_reset(struct tnt_stream *ops)

    Reset container state

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                          Operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. c:function:: ssize_t tnt_update_bit(struct tnt_stream *ops, uint32_t fieldno, char op, uint64_t value)

    Function for adding byte operation:

    Possible ``op``'s are:

    * ``'&'`` - for binary AND
    * ``'|'`` - for binary OR
    * ``'^'`` - for binary XOR

.. c:function:: ssize_t tnt_update_arith_int(struct tnt_stream *ops, uint32_t fieldno, char op, int64_t value)
                ssize_t tnt_update_arith_float(struct tnt_stream *ops, uint32_t fieldno, char op, float value)
                ssize_t tnt_update_arith_double(struct tnt_stream *ops, uint32_t fieldno, char op, double value)

    Three functions for adding arithmetic operation in different types:

    Possible ``op``'s are:

    * ``+`` - for addition
    * ``-`` - for substraction

.. c:function:: ssize_t tnt_update_delete(struct tnt_stream *ops, uint32_t fieldno, uint32_t fieldcount)

    Add delete operation for update to stream object.

.. c:function:: ssize_t tnt_update_insert(struct tnt_stream *ops, uint32_t fieldno, struct tnt_stream *val)

    Add insert operation for update to stream object.

.. c:function:: ssize_t tnt_update_assign(struct tnt_stream *ops, uint32_t fieldno, struct tnt_stream *val)

    Add assign operation for update to stream object.

.. c:function:: ssize_t tnt_update_splice(struct tnt_stream *ops, uint32_t fieldno, uint32_t position, uint32_t offset, const char *buffer, size_t buffer_len)

    Add splice operation for update to stream object.

    Remove ``offset`` bytes from position ``position`` in field ``fieldno`` and
    paste ``buffer`` in the room of this fragment.

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
