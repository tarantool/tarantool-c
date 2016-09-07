.. _working_with_tnt_stream:

-------------------------------------------------------------------------------
                        Building a request with "tnt_stream"
-------------------------------------------------------------------------------

Functions in this section add commands by appending them to a ``tnt_stream``
buffer. To send the buffered query to the server, you can use the
:func:`tnt_flush` function and then call :func:`tnt->read_reply`, for example.

=====================================================================
                      Adding requests (except UPDATE)
=====================================================================

.. c:function:: ssize_t tnt_auth_raw(struct tnt_stream *s, const char *user, int ulen, const char *pass, int plen, const char *base64_salt)
                ssize_t tnt_auth(struct tnt_stream *s, const char *user, int ulen, const char *pass, int plen)
                ssize_t tnt_deauth(struct tnt_stream *s)

    Two functions for adding an authentication request.
    ``ulen`` and ``plen`` are the lengths (in bytes) of user's name ``user`` and
    password ``pass``.

    ``tnt_deauth`` is a shortcut for ``tnt_auth(s, NULL, 0, NULL, 0)``.

.. c:function:: ssize_t tnt_call(struct tnt_stream *s, const char *proc, size_t plen, struct tnt_stream *args)
                ssize_t tnt_call_16(struct tnt_stream *s, const char *proc, size_t plen, struct tnt_stream *args)
                ssize_t tnt_eval(struct tnt_stream *s, const char *expr, size_t elen, struct tnt_stream *args)

    Add a call/eval request.

.. c:function:: ssize_t tnt_delete(struct tnt_stream *s, uint32_t space, uint32_t index, struct tnt_stream *key)

    Add a delete request.

.. c:function:: ssize_t tnt_insert(struct tnt_stream *s, uint32_t space, struct tnt_stream *tuple)
                ssize_t tnt_replace(struct tnt_stream *s, uint32_t space, struct tnt_stream *tuple)

    Add an insert/replace request.

.. c:function:: ssize_t tnt_ping(struct tnt_stream *s)

    Add a ping request.

.. c:function:: ssize_t tnt_select(struct tnt_stream *s, uint32_t space, uint32_t index, uint32_t limit, uint32_t offset, uint8_t iterator, struct tnt_stream *key)

    Add a select request.

    ``limit`` is the number of requests to gather. For gathering all results,
    set ``limit`` = ``UINT32_MAX``.

    ``offset`` is the number of requests to skip.

    ``iterator`` is the :ref:`iterator type <tnt_iterator_types>` to use.

=====================================================================
                       Adding an UPDATE request
=====================================================================

.. c:function:: ssize_t tnt_update(struct tnt_stream *s, uint32_t space, uint32_t index, struct tnt_stream *key, struct tnt_stream *ops)

    Basic function for adding an update request.

    ``ops`` must be a container gained with the :func:`tnt_update_container`
    function.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                   Container for operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. c:function:: struct tnt_stream *tnt_update_container(struct tnt_stream *ops)

    Create an update container.

.. c:function:: int tnt_update_container_close(struct tnt_stream *ops)

    Finish working with the container.

.. c:function:: int tnt_update_container_reset(struct tnt_stream *ops)

    Reset the container's state.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                          Operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. c:function:: ssize_t tnt_update_bit(struct tnt_stream *ops, uint32_t fieldno, char op, uint64_t value)

    Function for adding a byte operation.

    Possible ``op`` values are:

    * ``'&'`` - for binary AND
    * ``'|'`` - for binary OR
    * ``'^'`` - for binary XOR

.. c:function:: ssize_t tnt_update_arith_int(struct tnt_stream *ops, uint32_t fieldno, char op, int64_t value)
                ssize_t tnt_update_arith_float(struct tnt_stream *ops, uint32_t fieldno, char op, float value)
                ssize_t tnt_update_arith_double(struct tnt_stream *ops, uint32_t fieldno, char op, double value)

    Three functions for adding an arithmetic operation for a specific data type
    (integer, float or double).

    Possible ``op``'s are:

    * ``+`` - for addition
    * ``-`` - for subtraction

.. c:function:: ssize_t tnt_update_delete(struct tnt_stream *ops, uint32_t fieldno, uint32_t fieldcount)

    Add a delete operation for the update request.
    ``fieldcount`` is the number of fields to delete.

.. c:function:: ssize_t tnt_update_insert(struct tnt_stream *ops, uint32_t fieldno, struct tnt_stream *val)

    Add an insert operation for the update request.

.. c:function:: ssize_t tnt_update_assign(struct tnt_stream *ops, uint32_t fieldno, struct tnt_stream *val)

    Add an assign operation for the update request.

.. c:function:: ssize_t tnt_update_splice(struct tnt_stream *ops, uint32_t fieldno, uint32_t position, uint32_t offset, const char *buffer, size_t buffer_len)

    Add a splice operation for the update request.

    "Splice" means to remove ``offset`` bytes from position ``position`` in
    field ``fieldno`` and paste ``buffer`` in the room of this fragment.

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
