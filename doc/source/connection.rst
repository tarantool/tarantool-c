-------------------------------------------------------------------------------
                         Networking layer
-------------------------------------------------------------------------------

The basic networking layer with batching support is implemented in multiple
parts:

* :file:`tnt_net.c` - client-side layer
* :file:`tnt_io.c`  - network abstraction layer
* :file:`tnt_iob.c` - network buffer layer
* :file:`tnt_opt.c` - network options layer

=====================================================================
                        Creating a connection
=====================================================================

.. c:function:: struct tnt_stream *tnt_net(struct tnt_stream *s)

    Create a tarantool connection object. If ``s`` is NULL, then allocate memory
    for it.

    Return NULL if can't allocate memory.

=====================================================================
                          Handling errors
=====================================================================

.. see tnt/tnt_net.c

Possible error codes:

.. errtype:: TNT_EOK

    Not an error.

.. errtype:: TNT_EFAIL

    Failed to parse URI, bad protocol for sockets, or bad configuration option
    for the :func:`tnt_set` function.

.. errtype:: TNT_EMEMORY

    Memory allocation failed.

.. errtype:: TNT_ESYSTEM

    System error, ``_errno`` will be set. Acquire it with the :func:`tnt_errno`
    function.

.. errtype:: TNT_EBIG

    Read/write fragment is too big (in case the send/read buffer is smaller than
    the fragment you're trying to write into/read from).

.. errtype:: TNT_ESIZE

    Buffer size is incorrect.

.. errtype:: TNT_ERESOLVE

    Failed to resolve the hostname (the function :func:`gethostbyname(2)`
    failed).

.. errtype:: TNT_ETMOUT

    Timeout was reached during connect/read/write operations.

.. errtype:: TNT_EBADVAL

    Currently unused.

.. errtype:: TNT_ELOGIN

    Authentication error.

.. errtype:: TNT_LAST

    Pointer to the final element of an enumerated data structure (enum).

Functions to work with errors:

.. c:function:: enum tnt_error tnt_error(struct tnt_stream *s)

    Return the error code of the last stream operation.

.. c:function:: char *tnt_strerror(struct tnt_stream *s)

    Format the error as a string message. In case the error code is
    :errtype:`TNT_ESYSTEM`, then the driver uses the system function
    :func:`strerror()` to format the message.

.. c:function:: int tnt_errno(struct tnt_stream *s)

    Return the ``errno_`` of the last stream operation (in case the error code
    is :errtype:`TNT_ESYSTEM`).

=====================================================================
                Manipulating a connection
=====================================================================

.. see tnt/tnt_net.c

.. c:function:: int tnt_set(struct tnt_stream *s, int opt, ...)

    You can set the following options for a connection:

    * TNT_OPT_URI (``const char *``) - URI for connecting to
      :program:`tarantool`.
    * TNT_OPT_TMOUT_CONNECT (``struct timeval *``) - timeout on connecting.
    * TNT_OPT_TMOUT_SEND (``struct timeval *``) - timeout on sending.
    * TNT_OPT_SEND_CB (``ssize_t (*send_cb_t)(struct tnt_iob *b, void *buf,
      size_t len)``) - a function to be called instead of writing into a socket;
      uses the buffer ``buf`` which is ``len`` bytes long.
    * TNT_OPT_SEND_CBV (``ssize_t (*sendv_cb_t)(struct tnt_iob *b,
      const struct iovec *iov, int iov_count)``) - a function to be called
      instead of writing into a socket;
      uses multiple (``iov_count``) buffers passed in ``iov``.
    * TNT_OPT_SEND_BUF (``int``) - the maximum size (in bytes) of the buffer for
      outgoing messages.
    * TNT_OPT_SEND_CB_ARG (``void *``) - context for "send" callbacks.
    * TNT_OPT_RECV_CB (``ssize_t (*recv_cb_t)(struct tnt_iob *b, void *buf,
      size_t len)``) - a function to be called instead of reading from a socket;
      uses the buffer ``buf`` which is ``len`` bytes long.
    * TNT_OPT_RECV_BUF (``int``) - the maximum size (in bytes) of the buffer for
      incoming messages.
    * TNT_OPT_RECV_CB_ARG (``void *``) - context for "receive" callbacks.

    Return -1 and store the error in the stream.
    The error code can be either :errtype:`TNT_EFAIL` if can't parse the URI or
    ``opt`` is not defined, or :errtype:`TNT_EMEMORY` if failed to allocate
    memory for the URI.

.. c:function:: int tnt_connect(struct tnt_stream *s)

    Connect to :program:`tarantool` with preconfigured and allocated settings.

    Return -1 in the following cases:

    * Can't connect
    * Can't read greeting
    * Can't authenticate (if login/password was provided with the URI)
    * OOM while authenticating and getting schema
    * Can't parse schema

.. c:function:: void tnt_close(struct tnt_stream *s)

    Close connection to :program:`tarantool`.

.. c:function:: ssize_t tnt_flush(struct tnt_stream *s)

    Flush all buffered data to the socket.

    Return -1 in case of network error.

.. c:function:: int tnt_fd(struct tnt_stream *s)

    Return the file descriptor of the connection.

.. c:function:: int tnt_reload_schema(struct tnt_stream *s)

    Reload the schema from server. Delete the old schema and download/parse
    a new schema from server.

    See also ":ref:`working_with_a_schema`".

.. c:function:: int32_t tnt_get_spaceno(struct tnt_stream *s, const char *space, size_t space_len)
                int32_t tnt_get_indexno(struct tnt_stream *s, int space, const char *index, size_t index_len)

    Get space/index number from their names.
    For :func:`tnt_get_indexno`, specify the length of the space name (in bytes)
    in ``space_len``.
    For :func:`tnt_get_indexno`, specify the space ID number in ``space`` and
    the length of the index name (in bytes) in ``index_len``.

=====================================================================
                        Freeing a connection
=====================================================================

Use the :func:`tnt_stream_free` function to free a connection object.

..  // Examples are commented out for a while as we currently revise them.
..  =====================================================================
..                             Example
..  =====================================================================

  .. literalinclude:: example.c
      :language: c
      :lines: 61-76,347-351

  .. literalinclude:: example.c
      :language: c
      :lines: 16-25,34-42
