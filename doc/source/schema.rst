-------------------------------------------------------------------------------
                            Working with schema
-------------------------------------------------------------------------------

Schema is needed for mapping ``"space_name" -> "space_id"`` and
``("space_id", "index_name") -> "index_id"``.

=====================================================================
                        Create schema
=====================================================================

.. c:function:: struct tnt_schema *tnt_schema_new(struct tnt_schema *sch)

    Allocate and initialize schema object.

=====================================================================
                Creating requests for schema acquiring
=====================================================================

.. c:function:: ssize_t tnt_get_spaces(struct tnt_stream *s)
                ssize_t tnt_get_indexes(struct tnt_stream *s)

    Construct a query for selecting values from schema. It's shortcut for

    If ``name == NULL`` then ac

=====================================================================
                        Adding responses
=====================================================================

.. c:function:: struct tnt_schema_add_spaces(struct tnt_schema *s, struct tnt_reply *r)
                struct tnt_schema_add_indexes(struct tnt_schema *s, struct tnt_reply *r)

