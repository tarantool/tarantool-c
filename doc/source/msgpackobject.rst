-------------------------------------------------------------------------------
                             MessagePack layer
-------------------------------------------------------------------------------

Basic MsgPack object layer (msgpuck wrapper). It supports array/map traversal,
dynamic size msgpack arrays and e.t.c. For detailed analyze read
`MessagePack specification`_

=====================================================================
                        MsgPack introduction
=====================================================================

MessagePack is an efficient binary serialization format. It lets you exchange
data among multiple languages like JSON. But it's faster and smaller. Small
integers are encoded into a single byte, and typical short strings require only
one extra byte in addition to the strings themselves.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                         Types
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ``integer`` (``int`` < 0 and ``uint`` >= 0) represents an integer
* ``nil`` represents ``nil``
* ``boolean`` represents ``true`` or ``false``
* ``float`` - 4 bytes, ``double`` - 8 bytes) represents a floating
  point number
* ``string`` represents a UTF-8 string
* ``binary`` represents a byte array
* ``array`` represents a sequence of objects
* ``map`` represents key-value pairs of objects
* ``extension`` represents a tuple of type information and a byte array where
  type information is an integer whose meaning is defined by applications

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                      Limitations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* a value of an Integer object is limited from -(2^63) upto (2^64)-1
* a value of a Float object is IEEE 754 single or double precision
  floating-point number
* maximum length of a Binary object is (2^32)-1
* maximum byte size of a String object is (2^32)-1
* String objects may contain invalid byte sequence and the behavior of a
  deserializer depends on the actual implementation when it received invalid
  byte sequence
* Deserializers should provide functionality to get the original byte array so
  that applications can decide how to handle the object
* maximum number of elements of an Array object is (2^32)-1
* maximum number of key-value associations of a Map object is (2^32)-1

=====================================================================
                          Object creation
=====================================================================

.. c:function:: struct tnt_stream *tnt_object(struct tnt_stream *s)

    Create empty msgpack object. If ``s`` is passed as ``NULL``, then object is
    allocated. Otherwise allocated object will be initialized.

.. c:function:: struct tnt_stream *tnt_object_as(struct tnt_stream *s, char *buf,
                                                 size_t buf_len)

    Create read-only msgpack object from buffer. Source string isn't copied.

    :param char *     buf: input buffer
    :param size_t buf_len: length of buffer

=====================================================================
                        Scalar MsgPack types
=====================================================================

.. c:function:: ssize_t tnt_object_add_nil(struct tnt_stream *s)

    Append ``nil`` to a stream object.

.. c:function:: ssize_t tnt_object_add_int(struct tnt_stream *s, int64_t value)

    Add integer to a stream object. If it's value is more or equal zero, then
    it's packed in ``uint`` MsgPack type, otherwise it'll be ``int``

.. c:function:: ssize_t tnt_object_add_str (struct tnt_stream *s, const char *str, uint32_t len)
                ssize_t tnt_object_add_strz(struct tnt_stream *s, const char *str)

    Append utf-8 string to a stream object. If using ``<...>_strz`` function,
    then length is calculated using ``strlen(str)``.

.. c:function:: ssize_t tnt_object_add_bin(struct tnt_stream *s, const char *bin,
                                           uint32_t len)

    Append byte array to a stream object.

.. c:function:: ssize_t tnt_object_add_bool(struct tnt_stream *s, char value)

    Append boolean value to a stream object. If ``value == 0``, then appending
    ``false``, otherwise ``true``.

.. c:function:: ssize_t tnt_object_add_float(struct tnt_stream *s, float val)

    Append float value to a stream object. ``float`` means 4-byte floating point
    number.

.. c:function:: ssize_t tnt_object_add_double(struct tnt_stream *s, double val)

    Append double value to a stream object. ``double`` means 8-byte floating
    point number.

=====================================================================
                        Array/Map manipulation
=====================================================================

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    Array/Map in MsgPack
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To understand why's there many problems when working with MsgPack map/arrays
with dynamic size we need to understand how it's originally specified.

Arrays/Maps are a sequence of elements following the 'header'. Depending on
the number of elements in the sequence length of header varies. (length of
map is number of pairs of elements in it).

For example:

* length(elements) < 16 => length(header) == 1 byte
* length(elements) < (2^16) => length(header) == 3 byte
* length(elements) < (2^32) => length(header) == 5 byte

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                Working with Array/Map
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

So when you, dynamically, add 1 element and it's length becomes 16 - header
grow by 2 bytes (the same applies to 2^32). There's three strategies to work
with it:

.. containertype:: TNT_SBO_SIMPLE

    Set size before adding elements into it. It's default option.

.. containertype:: TNT_SBO_SPARSE

    Every container's header has length of 5 bytes. It's recommended if you have
    very big tuples.

.. containertype:: TNT_SBO_PACKED

    When you're finished to work with container - it will be packed.

.. c:function:: int tnt_object_type(struct tnt_stream *s, enum TNT_SBO_TYPE type)

    Function for setting object type. You can set it only when container is
    empty.

    Returns -1 if it's not empty.

.. c:function:: ssize_t tnt_object_add_array(struct tnt_stream *s, uint32_t size)

    Append array header to stream object. If :containertype:`TNT_SBO_SPARSE` or
    :containertype:`TNT_SBO_PACKED` is set as container type, then size is
    ignored.

.. c:function:: ssize_t tnt_object_add_map(struct tnt_stream *s, uint32_t size)

    Append map header to stream object. If :containertype:`TNT_SBO_SPARSE` or
    :containertype:`TNT_SBO_PACKED` is set as container type, then size is
    ignored.

.. c:function:: ssize_t tnt_object_container_close(struct tnt_stream *s)

    Close latest opened container. It's used when you set :func:`tnt_object_type`
    with :containertype:`TNT_SBO_SPARSE` or :containertype:`TNT_SBO_PACKED` value.

=====================================================================
                        Object manipulation
=====================================================================

.. c:function:: ssize_t tnt_object_format(struct tnt_stream *s, const char *fmt, ...)
                ssize_t tnt_object_vformat(struct tnt_stream *s, const char *fmt, va_list vl)


    Append msgpack values formatted to the stream object. ``<...>_vformat``
    function uses ``va_list`` as third argument.

    Format string consists from:

    * '[' and ']' pairs, defining arrays,
    * '{' and '}' pairs, defining maps
    * %d, %i - int
    * %u - unsigned int
    * %ld, %li - long
    * %lu - unsigned long
    * %lld, %lli - long long
    * %llu - unsigned long long
    * %hd, %hi - short
    * %hu - unsigned short
    * %hhd, %hhi - char (as number)
    * %hhu - unsigned char (as number)
    * %f - float
    * %lf - double
    * %b - bool
    * %s - zero-end string
    * %.*s - string with specified length
    * %% is ignored
    * %'smth else' assert and undefined behaviour
    * NIL - a nil value

    all other symbols are ignored.

.. c:function:: int tnt_object_verify(struct tnt_stream *s, int8_t type)

    Verify that object is valid msgpack structure. If ``type == -1``, then it
    doesn't verify first type, otherwise it checks that first type is ``type``.

.. c:function:: int tnt_object_reset(struct tnt_stream *s)

    Reset stream object to basic state.

=====================================================================
                            Example
=====================================================================

.. _MessagePack specification: https://github.com/msgpack/msgpack/blob/master/spec.md
