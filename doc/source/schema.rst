.. _working_with_a_schema:

-------------------------------------------------------------------------------
                            Working with a schema
-------------------------------------------------------------------------------

Schema is needed for mapping ``"space_name" -> "space_id"`` and
``("space_id", "index_name") -> "index_id"``.

=====================================================================
                        Creating a schema
=====================================================================

.. c:function:: struct tnt_schema *tnt_schema_new(struct tnt_schema *sch)

    Allocate and initialize a schema object.

=====================================================================
                Creating requests for acquiring a schema
=====================================================================

.. c:function:: ssize_t tnt_get_space(struct tnt_stream *s)
                ssize_t tnt_get_index(struct tnt_stream *s)

    Construct a query for selecting values from a schema.
    These are shortcuts for:

    * :func:`tnt_select(s, 281, 0, UINT32_MAX, 0, TNT_ITER_ALL, "\x90")`
    * :func:`tnt_select(s, 289, 0, UINT32_MAX, 0, TNT_ITER_ALL, "\x90")`

    where ``281`` and ``289`` are the IDs of the spaces listing all spaces
    (``281``) and all indexes (``289``) in the current Tarantool instance.

=====================================================================
                        Adding responses
=====================================================================

.. c:function:: struct tnt_schema_add_spaces(struct tnt_schema *sch, struct tnt_reply *r)
                struct tnt_schema_add_indexes(struct tnt_schema *sch, struct tnt_reply *r)

    Add spaces or indices to a schema.

