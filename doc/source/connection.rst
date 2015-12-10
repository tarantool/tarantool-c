-------------------------------------------------------------------------------
                            Networking layer in tarantool-c
-------------------------------------------------------------------------------

Basic network layer with batching support is implemented in multiple parts:

* tnt_net.c - client side layer
* tnt_io.c  - network abstraction layer
* tnt_iob.c - network buffer layer
* tnt_opt.c - network options layer

=====================================================================
                        Create connection
=====================================================================

.. c:function:: struct tnt_stream *tnt_net(struct tnt_stream *s)

    Create a tarantool connection object. If ``s`` is NULL, then it'll be
    allocated.

    Returns NULL if can't allocate Memory

=====================================================================
                          Error handling
=====================================================================

Possible error types:

.. errtype:: TNT_EOK

    Not an error.

.. errtype:: TNT_EFAIL

    Failed to parse URI, bad protocol for sockets or bad configuration option
    for :func:`tnt_set`.

.. errtype:: TNT_EMEMORY

    Memory allocation failed.

.. errtype:: TNT_ESYSTEM

    System error, ``_errno`` would be set. Acquire it with :func:`tnt_errno`.

.. errtype:: TNT_EBIG

    Buffer is too big (in case send/read buffer is less then fragment you're
    trying to write in it/read from).

.. errtype:: TNT_ERESOLVE

    Failed to resolve hostname (gethostbyname(2) failed).

.. errtype:: TNT_ETMOUT

    Timeout was reached while ``connect``/``read``/``write``.

Functions to work with errors:

.. c:function:: enum tnt_error tnt_error(struct tnt_stream *s)

    Return errcode of last operation.

.. c:function:: char *tnt_strerror(struct tnt_stream *s)

    Format error as string message (in case it's :errtype:`TNT_ESYSTEM`, then
    driver uses system function ``strerror()`` to format message)

.. c:function:: int tnt_errno(struct tnt_stream *s)

    Get errno of last error (in case it's :errtype:`TNT_ESYSTEM`)

=====================================================================
                Manipulating connection
=====================================================================

.. c:function:: int tnt_set(struct tnt_stream *s, int opt, ...)

    Set options for connection, possible options are:

    * TNT_OPT_URI (``const char *``) - uri for connecting to tarantool.
    * TNT_OPT_TMOUT_CONNECT (``struct timeval *``) - option for setting timeout
      on connect.
    * TNT_OPT_TMOUT_SEND (``struct timeval *``) - option for setting timeout
      on send.
    * TNT_OPT_SEND_CB (``ssize_t (*send_cb_t)(struct tnt_iob *b, void *buf, \
      size_t len)``) - callback to be called instead of write into socket.
    * TNT_OPT_SEND_CBV (``ssize_t (*sendv_cb_t)(struct tnt_iob *b, const \
      struct iovec *iov, int iov_count)``) - callback to be called instead of
      write into socket.
    * TNT_OPT_SEND_BUF (``int``) - size of buffer for sending.
    * TNT_OPT_SEND_CB_ARG (``void *``) - context for send callbacks.
    * TNT_OPT_RECV_CB (``ssize_t (*recv_cb_t)(struct tnt_iob *b, void *buf, \
      size_t len)``) - callback to be called instead of read from socket.
    * TNT_OPT_RECV_BUF (``int``) - size of buffer for recv.
    * TNT_OPT_RECV_CB_ARG (``void *``) - context for recv callback.

    It will return -1 and store error in
    Can return TNT_EFAIL if can't parse URI or option is not defined.
    Can return TNT_EMEMORY if can't allocate memory for URI.

.. c:function:: int tnt_connect(struct tnt_stream *s)

    Connect to tarantool with preconfigured and allocated settings

    Returns -1 in one of next cases:

    * Can't connect
    * Can't read greeting
    * Can't authenticate (if login/password was provided with URI)
    * OOM while authenticating and getting schema
    * Can't parse schema

.. c:function:: void tnt_close(struct tnt_stream *s)

    Close connection to tarantool

.. c:function:: ssize_t tnt_flush(struct tnt_stream *s)

    Flush all buffered data to socket.

    Returns -1 in case of network error.

.. c:function:: int tnt_fd(struct tnt_stream *s)

    Return file descriptor of connection.

.. c:function:: int tnt_reload_schema(struct tnt_stream *s)

    Reload schema from server - old schema is purged and then new schema is
    downloaded/parsed from server.

    See also :ref:`schema-description`

.. c:function:: int32_t tnt_get_spaceno(struct tnt_stream *s, const char *space, size_t space_len)
                int32_t tnt_get_indexno(struct tnt_stream *s, int space, const char *index, size_t index_len)

    Get space/index number from their names. If you're using ``tnt_get_indexno``,
    then space number is required.

=====================================================================
                               Freeing
=====================================================================

.. c:function:: void tnt_stream_free(struct tnt_stream *s)

    This function is used to free every stream object in this library

=====================================================================
                             Example
=====================================================================

.. code-block:: c

    #include <stdlib.h>
    #include <stdio.h>

    #include <tarantool/tarantool.h>
    #include <tarantool/tnt_net.h>
    #include <tarantool/tnt_opt.h>

    int main() {
        const char * uri = "localhost:3301";
        struct tnt_stream * tnt = tnt_net(NULL); // Allocating stream
        tnt_set(tnt, TNT_OPT_URI, uri);          // Set URI
        tnt_set(tnt, TNT_OPT_SEND_BUF, 0);       // Disable buffering for send
        tnt_set(tnt, TNT_OPT_RECV_BUF, 0);       // Disable buffering for recv
        tnt_connect(tnt);                        // Initialize stream and
                                                 // connect to Tarantool
        tnt_ping(tnt);                           // Send ping request
        struct tnt_reply * reply = tnt_reply_init(NULL); // Initialize reply
        tnt->read_reply(tnt, reply);             // Read reply from server
        tnt_reply_free(reply);                   // Free reply
        tnt_close(tnt); tnt_stream_free(tnt);    // Close connection and free
                                                 // stream object
    }

