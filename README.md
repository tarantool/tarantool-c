<a href="http://tarantool.org">
  <img src="https://avatars2.githubusercontent.com/u/2344919?v=2&s=250" align="right">
</a>

# The C API for low-level access to SQL in Tarantool 2.0

There are three ways to use C and SQL:

1. By passing SQL statements to `box.sql.execute(string)`
   (see the example in
   [Tarantool's SQL manual](https://tarantool.io/en/doc/2.0/tutorials/sql_tutorial/#box-sql-execute)).
2. Via ODBC
   (see the [list of ODBC functions](#list-of-odbc-functions)
   at the end of this document).
3. Via the `tarantool-c` extensions which we describe here.

With the `tarantool-c` extensions, users who are familiar
with `tarantool-c` for NoSQL will find that the integration
of `tarantool-c` for SQL is easy to understand because its
functions use the same principles. Also users who are new
to Tarantool, but have seen other APIs such as the one
for the MySQL client library, will find that the
`tarantool-c` API has functions which serve the same purpose.

This is a beta product which is not appropriate
for production use. We will welcome comments.
If you encounter bugs or need more features, please say so on
https://github.com/tarantool/tarantool-c/issues.

## Table of contents

* [Getting tarantool-c](#getting-tarantool-c)
* [An example program](#an-example-program)
* [Functions of the API](#functions-of-the-api)
  * [tnt_affected_rows](#tnt_affected_rows)
  * [tnt_bind_query](#tnt_bind_query)
  * [tnt_bind_query_param](#tnt_bind_query_param)
  * [tnt_bind_result](#tnt_bind_result)
  * [tnt_col_\[\*type\*\]](#tnt_col_type)
  * [tnt_col_bin](#tnt_col_bin)
  * [tnt_col_double](#tnt_col_double)
  * [tnt_col_float](#tnt_col_float)
  * [tnt_col_int](#tnt_col_int)
  * [tnt_col_is_null](#tnt_col_is_null)
  * [tnt_col_len](#tnt_col_len)
  * [tnt_col_name](#tnt_col_name)
  * [tnt_col_str](#tnt_col_str)
  * [tnt_col_type](#tnt_col_type-1)
  * [tnt_fetch](#tnt_fetch)
  * [tnt_field_names](#tnt_field_names)
  * [tnt_fulfill](#tnt_fulfill)
  * [tnt_number_of_cols](#tnt_number_of_cols)
  * [tnt_prepare](#tnt_prepare)
  * [tnt_query](#tnt_query)
  * [tnt_setup_bind_param](#tnt_setup_bind_param)
  * [tnt_stmt_close_cursor](#tnt_stmt_close_cursor)
  * [tnt_stmt_code](#tnt_stmt_code)
  * [tnt_stmt_error](#tnt_stmt_error)
  * [tnt_stmt_execute](#tnt_stmt_execute)
  * [tnt_stmt_free](#tnt_stmt_free)
  * [tnt_stmt_execute](#tnt_stmt_execute)
  * [tnt_stmt_execute](#tnt_stmt_execute)
  * [tnt_stmt_execute](#tnt_stmt_execute)
  * [tnt_stmt_execute](#tnt_stmt_execute)
  * [collect\_http()](#collect_http)
* [Structures of the API](#structures-of-the-API)
  * [Structure `tnt_bind_t`](#structure-tnt_bind_t)
  * [Structure `tnt_stmt`](#structure-tnt_stmt)
  * [Structure `col_type`](#structure-col_type)
* [List of ODBC functions](#list-of-odbc-functions)

## Getting `tarantool-c`

At the moment the download must be from our
["odbc" branch on github](https://github.com/tarantool/tarantool-c/tree/odbc).
The plan is to move it to the main branch after more testing.

You will need `git` and a C toolset on Linux.

```bash
git clone -b odbc https://github.com/tarantool/tarantool-c.git tarantool-c-odbc
cd tarantool-c-odbc
cmake .
make
```

If `make` finishes without errors, you now have a library file named
`./tnt/libtarantool.so.2.0.0`. Make sure that this file, and
`tarantool-c-odbc`'s header (\*.h) files, are on your path.

## An example program

Start an instance of the Tarantool 2.0 server, listening on port 3301.
With it, using the server as a client or with your favorite client,
say:

```lua
box.cfg{listen=3301}
box.schema.user.grant('guest','read,write,execute','universe')
```

Now create this program, which we'll call `exemplum.c`.

```c
#include <stdio.h>;
#include <stdlib.h>;

#include <tarantool/tarantool.h>;
#include <tarantool/tnt_fetch.h>;

void main()
{
  struct tnt_stream *tnt = tnt_net(NULL);
  tnt_set(tnt, TNT_OPT_URI, "localhost:3301");
  if (tnt_connect(tnt) < 0) {
     printf("Connection refused\n");
     exit(-1);
  }
  /* NEW CALLS START */
  char statement1[] = "CREATE TABLE EXEMPLAR (s1 INT PRIMARY KEY, s2 INT);";
  tnt_stmt_t* x = tnt_prepare(tnt, statement1, strlen(statement1));
  tnt_stmt_execute(x);
  printf("result after execute = %d\n", tnt_stmt_code(x));
  /* NEW CALLS END */
  tnt_close(tnt);
  tnt_stream_free(tnt);
}
```

Perhaps this program will look familiar, because most of it is
a copy of what's in
[the Tarantool manual's C example](https://tarantool.io/doc/1.7/book/connectors/index.html#example-1):

* The way to connect to the server is the same.
* The concepts for creation and teardown of objects are the same.

The differences are:
* There is a new include: `#include <tarantool/tnt_fetch.h>`.
* There are new calls between the comments NEW CALLS START and NEW CALLS END.

Now compile and run. The example here assumes you used github
from your $HOME directory and you made `exemplum.c` on your $HOME
directory.

```bash
gcc -I$HOME/tarantool-c-odbc/include -o exemplum exemplum.c  -L$HOME/tarantool-c-odbc/tnt -ltarantool
export LD_LIBRARY_PATH=$HOME/tarantool-c-odbc/tnt
./exemplum
```

If all has gone well, you will see that the `printf` displays
"result after execute = 0". That means that `tnt_stmt_execute()`
succeeded.

If you look using the regular Tarantool client, you will see that
there is a new table named EXEMPLAR.

The rest of this document has a description of each function
in the API, and then a description of each structure.

## Functions of the API

In this section we list every function in the API.

Order is alphabetical but with cross-references.

Each description shows the name, arguments, and possible result.

Most of the "Example snippets" in this section could be added
in `exemplum.c` immediately before the "NEW CALLS END" comment, and would
run correctly if the `EXEMPLAR` table is new.

### tnt_affected_rows

**Description:**

```c
int64_t tnt_affected_rows(tnt_stmt_t *);
```

Return the number of rows accessed by certain SQL statements.

**Parameters:** a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt).

**Returns:** the number of rows affected by the last DML statement
that was executed.

The return value is 0 if the statement failed or was not a data-change
statement. (The data-change statements are delete, insert, replace, update.)

Rows changed by triggered statements are not counted.

Rows changed by update statements are counted even if the
row remains unchanged.

"Last executed statement" does not mean the last executed
statement on `tnt_stmt_t` only, it means the last executed
statement of the session.

**Example snippet:**

```c
char statement2[] = "INSERT INTO EXEMPLAR VALUES (2,2),(3,3);";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
tnt_stmt_execute(x2);
int64_t affected_rows= tnt_affected_rows(x2);
printf("%ld\n", affected_rows);
```

The `printf` should say 2.

### tnt_bind_query

**Description:**

```c
tnt_bind_query(tnt_stmt_t * stmt, tnt_bind_t * bnd, int number_of_parameters);
```

Statements may contain parameter markers, seen as "?"s.
For example `SELECT * FROM t WHERE a = ?;` has a parameter marker.
If there has been a binding, then the value and
type of the parameter are known.

The `tnt_bind_query` function is for input parameters
(values are supplied to the DBMS from the caller);
for output parameters see [tnt_bind_result](#tnt_bind_result).

**Parameters:**

* a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt);
* a pointer to a list of addresses and types (see
  [structure tnt_bind_t](#structure-tnt_bind_t) for details);
* the size of the list.

For other ways to allocate and initialize `tnt_bind_t` structures,
see [tnt_setup_bind_param](#tnt_setup_bind_param)
and [tnt_bind_query_param](#tnt_bind_query_param).

**Example snippet:**

```c
char statement2[] = "INSERT INTO EXEMPLAR VALUES (44,?);";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
int num= 100;
tnt_size_t num_len= 0;
int num_error= 55;
int num_is_null= 0; /* 0 means not null. 1 would mean null. */
tnt_bind_t p_num[1];
p_num[0].type= TNTC_INT;
p_num[0].buffer= &num; /* point to the value that will be inserted */
p_num[0].in_len= sizeof(int); /* ignored because .type is TNTC_INT */
p_num[0].out_len= &num_len; /* ignored "" */
p_num[0].is_null= &num_is_null; /* won't be null since num_is_null = 0 */
p_num[0].error= &num_error;
tnt_bind_query(x2, p_num, 1);
tnt_stmt_execute(x2);
printf("result after execute = %d\n", tnt_stmt_code(x2));
```

Note: Setting `.in_len` and `.out_len` is unnecessary in this case.

### tnt_bind_query_param

**Description:**

```c
int tnt_bind_query_param(tnt_stmt_t *stmt, int icol, int type, const void* val_ptr, int len);
```

`tnt_bind_query_param` is a variation of [tnt_bind_query](#tnt_bind_query),
which manages the memory automatically (with `tnt_bind_query`
the programmer is responsible for allocating the appropriate array of
[structure tnt_bind_t](#structure-tnt_bind_t). Some programmers
will find that `tnt_bind_query_param` is safer.
However, some fields cannot be set to non-default values.

**Parameters:**

* a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt);
* a column number, and also an offset in a hidden `tnt_bind_t` array;
* a data type, one of the numeric type values in [structure col_type](#structure-col_type);
* a pointer to a value;
* a number indicating a length.

**Returns:** a number.

Calling `tnt_bind_query_param (tnt_stmt_ptr, 1, TNTC_STR, &input, 5);`
is equivalent to saying "allocate an invisible `tnt_bind_t`
(or increase its size if it exists but is too small to hold column #1's specifications),
let `p.type = TNTC_STR`, let `p.buffer = &input`".

The example snippet here does the same operation as
the example snippet for [tnt_bind_query](#tnt_bind_query)
but with fewer C statements.

**Example snippet:**

```c
char statement2[] = "INSERT INTO EXEMPLAR VALUES (45,?);";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
int num= 100;
tnt_bind_query_param(x2, 0, TNTC_INT, &num, 0);
tnt_stmt_execute(x2);
printf("result after execute = %d\n", tnt_stmt_code(x2));</pre>
```

### tnt_bind_result

**Description:**

```c
int tnt_bind_result(tnt_stmt_t *, tnt_bind_t *, int number_of_parameters);
```

`SELECT` and `VALUE` statements generate result sets, and for each row in the
result set there is a "fetch" function (see [tnt_fetch](#tnt_fetch).
But there must be descriptions including addresses for each column,
so that `tnt_fetch` will know where to put results and how to convert.

**Parameters:**

* a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt);
* a pointer to a [tnt_bind_t structure](#structure-tnt_bind_t) (a list of addresses and types);
* a number showing how many parameters will be bound (the size of the list).

This does not work with `SELECT ... INTO ...;` statements.

The information that is delivered by `tnt_bind_result` may also
be obtained via the functions whose names begin with `tnt_col_`
(`tnt_col_bin`, `tnt_col_len`, and so on).

**Example snippet:**

```c
char statement2[] = "SELECT s2 FROM EXEMPLAR LIMIT 1;";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
int num= 1;
tnt_size_t num_len= 0;
int num_error= 55;
int num_is_null= 0;
tnt_bind_t p_num[1];
p_num[0].type= TNTC_INT;
p_num[0].buffer= &num;
p_num[0].in_len= sizeof(int); /* ignored because .type is TNTC_INT */
p_num[0].out_len= &num_len; /* ignored "" */
p_num[0].is_null= &num_is_null;
p_num[0].error= &num_error;
tnt_bind_result(x2, p_num, 1);
tnt_stmt_execute(x2);
printf("result after execute = %d\n", tnt_stmt_code(x2));
tnt_fetch(x2);
printf("num=%d, num_errore=%d\n", num, num_error);</pre>
```
This should put something in `num` if `EXEMPLAR` has at least one row.

### tnt_col_[\*type\*]

These are notes which apply for all functions whose names begin
with `tnt_col_`.

The `tnt_col_[*type*]` functions retrieve values, or pointers to values,
from the last row that was fetched.
Normally an SQL statement is executed with
[tnt_prepare](#tnt_prepare) and [tnt_stmt_execute](#tnt_stmt_execute).
If the execution is successful, and the SQL statement began
with `SELECT` or `VALUES`, then there is a result set.
Rows in a result are fetched with [tnt_fetch](#tnt_fetch).
After the fetch, `tnt_col_[*type*]` calls are possible.

Each of these functions has a `col` parameter.
It must contain a column number.
The first column number is 0. This is different from "SQL column
position", since within SQL the first column number is 1.

Most of the `tnt_col_[*type*]` functions have a return
type, for example `tnt_col_float` returns a float.
Automatic type coversion (for example casting an integer to a float)
is possible; however, users should not depend on it.
Using `tnt_col_[*type*]` functions with an incorrect type
(different from what the [tnt_col_type](#tnt_col_type-1) function itself
would return) may cause undefined behavior.
Users who want implicit conversions should use binding variables.
For example, to extract all integer and double columns as strings,
one could use [bind result](#tnt_bind_result) variables with type TNTC_STR.

The `tnt_col_[*type*]` functions (except `tnt_col_is_null`) should not
be used with NULL. For example the value returned by
[tnt_col_len](#tnt_col_len) is undefined if the result column is NULL.
In a future version it may be possible to use `tnt_col_name>`
and `tnt_col_type` even when the value is NULL.

If you are not certain what the column's type is, see
[tnt_col_type](#tnt_col_type-1).

If you are not certain whether the value is NULL, see
[tnt_col_is_null](#tnt_col_is_null).

### tnt_col_bin

**Description:**

```c
const char *tnt_col_bin(tnt_stmt_t *, int col);
```

For a column in a result set, after the row is fetched, `tnt_col_bin`
will return the column value via a pointer to a string.

**Parameters:**

* a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt);
* a column number.

**Returns:** a pointer to a string.

The return-value pointer will become invalid when the next
row is fetched or when the statement is closed, so programmers should
make copies of what they intend to use for a long period.

See also [tnt_col_str](#tnt_col_str), which is similar (typically
`tnt_col_bin` is used for `BLOBs`, and `tnt_col_str` is used for `CHARs`).

If the return-value pointer points to a string that might contain
\0 bytes, use `tnt_col_len` to get the actual length of the string.

See also [tnt_col_\[\*type\*\]](#tnt_col_type) for notes which apply to
all functions whose names start with `tnt_col_`.

**Example snippet:**

```c
char statement2[] = "VALUES (X'440055');";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
tnt_stmt_execute(x2);
tnt_fetch(x2);
const char* i= tnt_col_bin(x2, 0);
int len= tnt_col_len(x2, 0);
int k;
for (k= 0; k < len; ++k) printf("%x\n", *(i+k));</pre>
```
The `printf` will display 44, 0, 55.

### tnt_col_double

**Description**:

```c
double tnt_col_double(tnt_stmt_t *, int col);
```

For a column in a result set, after the row is fetched,
`tnt_col_double` will return the column value as a 64-bit (double precision)
float.

**Parameters:**

* a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt);
* a column number.

**Returns:** a number.

See also [tnt_col_float](#tnt_col_float).

See also [tnt_col_\[\*type\*\]](#tnt_col_type) for notes which apply to
all functions whose names start with `tnt_col_`.

**Example snippet:**

```c
char statement2[] = "VALUES(1.23456789012345);";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
tnt_stmt_execute(x2);
tnt_fetch(x2);
double i= tnt_col_double(x2, 0);
printf("%1.15lf\n", i);
```
The `printf` will display 1.234567890123450.

### tnt_col_float

**Description:**

```c
double tnt_col_double(tnt_stmt_t *, int col);
```

For a column in a result set, after the row is fetched,
`tnt_col_float` will return the column value as a single-precision float.

**Parameters:**

* a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt);
* a column number.

**Returns:** a number.

See also [tnt_col_double](#tnt_col_double).

See also [tnt_col_\[\*type\*\]](#tnt_col_type) for notes which apply to
all functions whose names start with `tnt_col_`.

**Example snippet:**

```c
char statement2[] = "VALUES(1.23456789012345);";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
tnt_stmt_execute(x2);
tnt_fetch(x2);
float i= tnt_col_float(x2, 0);
printf("%1.15f\n", i);</pre>
```

The `printf` will display 1.234567... an inaccurate result.

### tnt_col_int

**Description:**

```c
int64_t tnt_col_int(tnt_stmt_t *, int col);
```

For a column in a result set, after the row is fetched,
`tnt_col_float` will return the column value as a 64-bit signed integer.

**Parameters:**

* a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt);
* a column number.

**Returns:** a number.

See also [tnt_col_\[\*type\*\]](#tnt_col_type) for notes which apply to
all functions whose names start with `tnt_col_`.

**Example snippet:**

```c
  char statement2[] = "SELECT s2 FROM EXEMPLAR LIMIT 1;";
  tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
  tnt_stmt_execute(x2);
  tnt_fetch(x2);
  int64_t i= tnt_col_int(x2, 0);
  printf("%ld\n", i);</pre>
```

`EXEMPLAR` must contain at least 1 row. This will display one value.

### tnt_col_is_null

**Description:**

```c
int tnt_col_is_null(tnt_stmt_t *, int col);
```

For a column in a result set, after the row is fetched, this
function will return TRUE (1) if the column value is NULL,
otherwise this function will return FALSE (0).

**Parameters:**

* a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt);
* a column number.

**Returns:** a number.

See also [tnt_col_\[\*type\*\]](#tnt_col_type) for notes which apply to
all functions whose names start with `tnt_col_`.

**Example snippet:**

```c
char statement2[] = "VALUES(NULL);";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
tnt_stmt_execute(x2);
tnt_fetch(x2);
int i= tnt_col_is_null(x2, 0);
printf("%d\n", i);</pre>
```

The `printf` will display 1.

### tnt_col_len

**Description:**

```c
int tnt_col_len(tnt_stmt_t *, int col);
```

For a column in a result set, after the row is fetched,
`tnt_col_len` will return the length of a column value as a 64-bit
signed integer.

**Parameters:**

* a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt);
* a column number.

**Returns:** a number.

`tnt_col_len` is useful if the length can vary (as for `CHAR` and `BLOB`
data types), it is not recommended if the length cannot vary
(as for numeric data types).

See also [tnt_col_\[\*type\*\]](#tnt_col_type) for notes which apply to
all functions whose names start with `tnt_col_`.

**Example snippet:**

```c
char statement2[] = "VALUES('Щ');";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
tnt_stmt_execute(x2);
tnt_fetch(x2);
int len= tnt_col_len(x2, 0);
printf("%d\n", len);</pre>
```

The `printf` will display 2.

### tnt_col_name

**Description:**

```c
const char *tnt_col_name(tnt_stmt_t *, int col);
```

For a column in a result set, before or after the row is fetched,
`tnt_col_name` function will return a pointer to the name of the column.

**Parameters:**

* a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt);
* a column number.

**Returns:** a pointer to a string. The string will end with '\0'.

There will be a return value even if the result set has zero rows.

Every column in a result set has a name (for a description of how Tarantool
decides what each name is, see the SQL manual section for `SELECT`).

For example, `SELECT t.a, t.b as "Wo" FROM t;` has
two columns, named A and Wo. So `tnt_col_name(..., 0)` returns 'A'.
To get the list of all column names, see also
[tnt_field_names](#tnt_field_names).

See also [tnt_col_\[\*type\*\]](#tnt_col_type) for notes which apply to
all functions whose names start with `tnt_col_`.

**Example snippet:**

```c
char statement2[] = "VALUES('Щ' as \"cyrillic_character\");";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
tnt_stmt_execute(x2);
const char *s2= tnt_col_name(x2, 0);
printf("%s\n", s2);</pre>
```

The `printf` will display `cyrillic_character`.

### tnt_col_str

**Description:**

```c
const char *tnt_col_str(tnt_stmt_t *, int col);
```

For a column in a result set, after the row is fetched, `tnt_col_str`
will return the column value via a pointer to a string.

**Parameters:**

* a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt);
* a column number.

**Returns:** a pointer to a string. The string will end with '\0'.

The return-value pointer will become invalid when the next
row is fetched or when the statement is closed, so programmers should
make copies of what they intend to use for a long period.

See also [tnt_col_bin](#tnt_col_bin), which is similar (typically
`tnt_col_bin` is used for `BLOBs` and `tnt_col_str` is used for `CHARs`).

See also [tnt_col_\[\*type\*\]](#tnt_col_type) for notes which apply to
all functions whose names start with `tnt_col_`.

**Example snippet:**

```c
char statement2[] = "SELECT 'a', 'b', 'c', 'd';";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
tnt_stmt_execute(x2);
tnt_fetch(x2);
const char *s= tnt_col_str(x2, 3);
printf("%s\n", s);</pre>
```

The `printf` will display `d`.

### tnt_col_type

**Description:**

```c
int tnt_col_type(tnt_stmt_t *, int col);
```

For a column in a result set, after the row is fetched, this
function will return the column data type.

**Parameters:**

* a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt);
* a column number.

**Returns:** a pointer to a string. The string will end with '\0'.

Every column in a result set (what `SELECT` or `VALUES` generates)
has a data type, for example "`CHAR(5)`".
`tnt_col_type` has partial information about
that type, for example `TNTC_STR`. For the list of possible types and their
numeric values, see [structure col_type](#structure-col_type).

See also [tnt_col_\[\*type\*\]](#tnt_col_type) for notes which apply to
all functions whose names start with `tnt_col_`.

**Example snippet:**

```c
char statement2[] = "SELECT 1, 2.2, x'440045', 'd';";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
tnt_stmt_execute(x2);
tnt_fetch(x2);
const char *s= tnt_col_str(x2, 3);
int k;
for (k= 0; k <= 3; ++k) printf("%d\n", tnt_col_type(x2, k));</pre>
```

The `printf` will display 2, 9, 4, 3.

### tnt_fetch

**Description:**

```c
int tnt_fetch(tnt_stmt_t *);
```

Fetch the next row from a result set.

**Parameters:** a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt).

**Returns:** a number with a code: OK, FAIL, or NODATA.

A "result set" is what a `SELECT` or `VALUES` statement generates.

It is necessary to call `tnt_fetch` after generating a result set, and before
trying to look at the result set contents with `tnt_col_...`
functions or with the fields that are set up via
[tnt_bind_result](#tnt_bind_result).

`tnt_fetch` always gets the "next" row and discards whatever
values or pointers were set up by any previous `tnt_fetch` call.

This way:
* if `tnt_fetch` is successful then the return-value is OK;
* if there is an error then the return-value is FAIL;
* if there is no error but there are no more rows to fetch then the result
  is NODATA.

To describe it another way...
A result set has an automatic "cursor" associated
with it. Cursors are for fetching one row at a time.
Immediately after `SELECT` or `VALUES` is executed, the cursor is
at the first row.
Each time `tnt_fetch` is called, the cursor advances to the next row.
The result of a "fetch" is that values are taken from the result
set to send to the calling program, and the calling program can specify
before the fetch where the values are to go -- this is why
[tnt_bind_result](#tnt_bind_result) exists.

Alternatively, the calling program can contain items that do not
need to be discovered until after the fetch -- this is why the
[tnt_col_\[\*type\*\]](#tnt_col_type) functions exist.

One can call `tnt_fetch` until there are no more rows.

The return from `tnt_fetch` will be OK = success, FAIL = error, or
NODATA = there are no more rows. (Programmers can also call
[tnt_stmt_code](#tnt_stmt_code) to see
whether `tnt_fetch` caused FAIL or NODATA, but we do not
recommend that.)

**Example snippet:**

```c
char statement2[] = "SELECT 1, 2.2, x'440045', 'd';";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
tnt_stmt_execute(x2);
int k1, k2;
k1= tnt_fetch(x2);
k2= tnt_fetch(x2);
printf("%d %d\n", k1, k2);</pre>
```

The `printf` will display 0, 2.

### tnt_field_names

**Description:**

```c
const char **tnt_field_names(tnt_stmt_t *);
```

**Parameters:** a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt).

**Returns:** a point to pointers to strings. Strings end with '\0'.

See [tnt_col_name](#tnt_col_name) for a description
of column names. We recommend using `tnt_col_name`
in a loop, rather than `tnt_field_names`.

**Example snippet:**

```c
  char statement2[] = "SELECT s2 FROM EXEMPLAR;";
  tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
  tnt_stmt_execute(x2);
  printf("result after execute = %d\n", tnt_stmt_code(x2));
  const char *tx= *tnt_field_names(x2);
  printf("%s.\n", tx);</pre>
```

The `printf` will display "S2".

### tnt_fulfill

**Description:**

```c
tnt_stmt_t *tnt_fulfill(struct tnt_stream *);
```

`tnt_fulfill` is a function which glues together the Tarantool
SQL API and the Tarantool NoSQL API.

**Parameters:** a pointer to a `tarantool-c` structure.

**Returns:** a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt).

Notice that the passed structure
is a stream and the result structure is `tnt_stmt_t`, which
is usually used as input for the functions that work with the example
program.

Important: this works only after `tnt_execute`, which is a
function in the `tarantool-c` API, not in the `tarantool-c-odbc` API.

**Example snippet:** none, because this does not fit the pattern of functions
that would work with the `exemplum.c` example program.

### tnt_number_of_cols

**Description:**

```c
int tnt_number_of_cols(tnt_stmt_t *);
```

**Parameters:** a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt).

**Returns:** a number.

A result set has a certain number of rows and a certain number of columns.
For example, `SELECT a,b,c ...;` will generate a result set with
three columns. Therefore, after `SELECT a,b,c ...;` is executed,
`tnt_number_of_columns` will return 3.

**Example snippet:**

```c
char statement2[] = "SELECT 5, 'd';";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
tnt_stmt_execute(x2);
tnt_fetch(x2);
int number_of_columns= tnt_number_of_cols(x2);
int k;
int64_t num_value;
const char *str_value;
printf("%d\n", number_of_columns);
for (k= 0; k < number_of_columns; ++k)
{
  int column_type= tnt_col_type(x2, k);
  if (column_type == TNTC_BIGINT)
  {
    num_value= tnt_col_int(x2, k);
    printf("num_value=%ld\n", num_value);
  }
  if (column_type == TNTC_STR)
  {
    str_value= tnt_col_str(x2, k);
    printf("str_value=%s\n", str_value);
  }
}
```

The `printf` will display `num_value=5, str_value=d`.

### tnt_prepare

**Description:**

```c
tnt_stmt_t *tnt_prepare(struct tnt_stream *s, const char *text, int32_t len);
```

Given an SQL statement and its length, do some parsing and set up
some internal tables for description of the statement and its
results.

**Parameters:**

* a pointer to a `tnt_stream` structure made with ordinary `tarantool-c`;
* a pointer to a string containing an SQL statement;
* a number = the number of bytes in the string that Parameter 2 points to.

**Returns:** a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt).

`tnt_prepare` returns a [tnt_stmt_t](#tnt_stmt_t) instance that can be used as
an input parameter for other functions.

Commonly the result of a successful `tnt_prepare` is eventually
used for [tnt_stmt_execute](#tnt_stmt_execute).

The memory that `tnt_prepare` allocates can be freed with
[tnt_stmt_free](#tnt_stmt_free) when it is no longer needed.

**Example snippet:**

```c
char statement2[] = "SELECT 5, 'd';";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));</pre>
```

### tnt_query

**Description:**

```c
tnt_stmt_t *tnt_query(struct tnt_stream *s, const char *text, int32_t len);
```

Given an SQL statement and its length, do some parsing and setup
work (that is, an effective `tnt_prepare()` call) followed by actual
execution (that is, an effective [tnt_stmt_execute](#tnt_stmt_execute) call).

**Parameters:**

* a pointer to a `tnt_stream` structure made with ordinary `tarantool-c`;
* a pointer to a string containing an SQL statement;
* a number = the number of bytes in the string that Parameter 2 points to.

**Returns:** a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt)

The return-value's `tnt_stmt_t` address can be used as an input parameter
for other functions in the API.

`tnt_query` is equivalent to what is called "execute immediate" in
other situations.
It is convenient for simple SQL statements that are executed infrequently.
But `tnt_prepare` and `tnt_stmt_execute` give programmers more
flexibility, because after `tnt_prepare` they can
[bind](#tnt_bind_query) some input parameters.

**Example snippet:**

```c
char query[]= "CREATE TABLE t(id INT PRIMARY KEY);";
tnt_query(tnt,query,strlen(query));</pre>
```

### tnt_setup_bind_param

**Description:**

```c
void tnt_setup_bind_param(tnt_bind_t *p, int type,const void* val_ptr, int len);
```

`setup_bind_param` is a convenience function which fills in necessary fields
of `tnt_bind_t`.

**Parameters:**

* a pointer to a [structure tnt_bind_t](#structure-tnt_bind_t);
* a data type, one of the numeric type values in [structure col_type](#structure-col_type);
* a pointer to a value;
* a number indicating a length.

**Returns:** None.

Calling `tnt_setup_bind_param (p, TNTC_STR, &input, 5);`
is equivalent to saying "clear p to 0s, let p.type = TNTC_STR,
let p.buffer = &input".

The example snippet here does the same operation as
the example snippet for [tnt_bind_query](#tnt_bind_query)
but with fewer C statements.

**Example snippet:**

```c
char statement2[] = "INSERT INTO EXEMPLAR VALUES (46,?);";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
int num= 100;
tnt_bind_t p_num[1];
tnt_setup_bind_param(p_num, TNTC_INT, &num, 0);
tnt_bind_query(x2, p_num, 1);
tnt_stmt_execute(x2);
printf("result after execute = %d\n", tnt_stmt_code(x2));
```

### tnt_stmt_close_cursor

**Description:**

```c
void tnt_stmt_close_cursor(tnt_stmt_t *);
```

Free a result set.

**Parameters:** a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt).

**Returns:** none.

A result set has an automatic "cursor" associated
with it. Result sets require some resources.
Closing the cursor with `tnt_stmt_close_cursor` frees those resources,
but makes further access to the result set impossible.

It is not guaranteed that cursors are closed automatically when
the next statement (of any kind, not just `SELECT` or `VALUES`) is executed,
even if it fails -- therefore we recommend calling `tnt-stmt_close_cursor`
after executing any `SELECT` or `VALUES` statement and processing the result set.

**Example snippet:**

```c
char statement2[] = "SELECT 1, 2.2, x'440045', 'd';";
tnt_stmt_t* x2 = tnt_prepare(tnt, statement2, strlen(statement2));
tnt_stmt_execute(x2);
tnt_stmt_close_cursor(x2);</pre>
```

### tnt_stmt_code

**Description:**

```c
int tnt_stmt_code(tnt_stmt_t *);
```

Get an error code.

**Parameters:** a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt).

**Returns:** a number.

Almost all the functions described in this document can cause errors.
Usually the errors are not returned from the function -- the function
will typically return blank or undefined values.
So the way to find out the error code is to call `tnt_stmt_code`.
This is true for most of the odbc-c-api functions, except the
functions that return int, such as
[tnt_col_is_null](#tnt_col_is_null) or [tnt_fetch](#tnt_fetch).

If `tnt_stmt_code` returns 0 (OK), then the last function was successful.

To get the error string rather than the error code, use
[tnt_stmt_error](#tnt_stmt_error).

**Example snippet:**

```c
char statement3[] = "DROP TABLE no_such_table;";
tnt_stmt_t* x3 = tnt_prepare(tnt, statement3, strlen(statement3));
tnt_stmt_execute(x3);
int i= tnt_stmt_code(x3);
printf("%d\n", i);</pre>
```

There is no table named `no_such_table` so `printf` will not print 0.

### tnt_stmt_error

**Description:**

```c
const char *tnt_stmt_error(tnt_stmt_t *, size_t *length);
```

As with [tnt_stmt_code](#tnt_stmt_code) (where the conditions for "errors" are
described), `tnt_stmt_error` will return information about the last error
that was caused by any other function, for example "table not found".

**Parameters:**

* a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt);
* a pointer to a result number which will have the length of the return value.

**Returns:** a pointer to a string, the text of an error message, if any.

If `tnt_stmt_error` returns "ok", then the last function was successful.

To get the error code rather than the error string, use
[tnt_stmt_code](#tnt_stmt_code).

**Example snippet:**

```c
char statement3[] = "DROP TABLE no_such_table;";
tnt_stmt_t* x3 = tnt_prepare(tnt, statement3, strlen(statement3));
tnt_stmt_execute(x3);
size_t len;
const char *e= tnt_stmt_error(x3, &len);
printf("%s\n", e);</pre>
```

The `printf` will display
"Failed to execute SQL statement: no such table: NO_SUCH_TABLE".

### tnt_stmt_execute

**Description:**

```c
int tnt_stmt_execute(tnt_stmt_t *);
```

`tnt_stmt_execute` takes an SQL statement that was prepared earlier by
[tnt_prepare](#tnt_prepare)
and passes it to the Tarantool server for execution.

**Parameters:** a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt).

**Returns:** a number.

If the execution is successful and a result set (from `SELECT` or `VALUES`)
is produced, then a cursor is automatically opened so
that [tnt_fetch](tnt_fetch) can be invoked repetitively.

`tnt_stmt_execute` can only be done after `tnt_prepare`.
It may be used multiple times for the same `tnt_prepare` result.
If the original string for `tnt_prepare` contained "?" parameter markers,
then before `tnt_stmt_execute` is called the parameters must be
associated with addresses within the program memory -- this is why
[tnt_bind_query](#tnt_bind_query) exists.

**Example snippet:**:

```c
char statement3[] = "DROP TABLE no_such_table;";
tnt_stmt_t* x3 = tnt_prepare(tnt, statement3, strlen(statement3));
tnt_stmt_execute(x3);</pre>
```

### tnt_stmt_free

**Description:** `void tnt_stmt_free(tnt_stmt_t *);`
Releases all data used by a statement.

**Parameters:** a pointer to a [tnt_stmt_t structure](#structure-tnt_stmt).

**Returns:** none.

Three API functions (`tnt_prepare`, `tnt_query` and `tnt_fulfill`)
return a pointer to a `tnt_stmt_t` structure. This pointer
value becomes invalid.

Four API functions (`tnt_col_bin`, `tnt_col_str`, `tnt_col_name` and
`tnt_field_names`)
return a pointer to a string. This pointer value becomes invalid.

To put it another way...
The API includes functions that silently allocate memory for objects.
Calling `tnt_stmt_free` is a necessary part of teardown to avoid
memory leaks.
(To keep the `exemplum.c` example small we did not include this,
but teardown is good practice.)

**Example snippet:**

```c
char statement3[] = "SELECT 5;";
tnt_stmt_t* x3 = tnt_prepare(tnt, statement3, strlen(statement3));
tnt_stmt_execute(x3);
tnt_stmt_free(x3);</pre>
```

## Structures of the API

### Structure `tnt_bind_t`

This is a copy of a structure in
[tnt_fetch.h](https://github.com/tarantool/tarantool-c/blob/odbc/include/tarantool/tnt_fetch.h).

```c
/**
* Bind array element structure.
*/
typedef struct tnt_bind {
  int32_t type;
  void *buffer;
  tnt_size_t in_len;
  tnt_size_t *out_len;
  int *is_null;
  int *error; /* conversion result. O is OK */
  } tnt_bind_t;</pre>
```

The `tnt_bind_t` structure is important for
[tnt_bind_query](#tnt_bind_query) and [tnt_bind_result](#tnt_bind_result).

The `.type` value should be one of the TNTC values listed for
"Structure `col_type`", below. For example, if the SQL target
is defined as BIGINT, then `.type` should be TNTC_SBIGINT and
`.buffer` should point to an integer defined with "int64_t".

The `.buffer` value should point to the value that is being
passed, or the value that will be returned.

The `.in_len` value should be the number of bytes in what
`.buffer` points to, when passing from the C program to
the server. The `in_len` value only matters if `.type` = TNTC_BIN
or `.type` = TNTC_STR; for all other types, `in_len` is ignored
because the `.in_len` value is implied by the `.type` value.

The `.out_len` value should be the number of bytes in what
`.buffer` points to, when receiving from the server to the
C program. The `.out_len` value only matters if `.type` = TNTC_BIN
or `.type` = TNTC_STR; for all other types,`.out_len` is ignored
because the `.out_len` value is implied by the `.type` value.

The `.is_null` value should point to an integer which will
contain 1 for NULL, 0 for non-NULL.
Actually there are two ways to say that a value is or is
not NULL. The first way is to point from `.is_null` to a flag,
as we recommend here. The second way is to set `.type` to
TNTC_NIL, which works because Tarantool/NoSQL treats nil as a data type.

The `.error` value should be a pointer to an integer site which will receive
an error code for the result of the conversion. A non-zero
value indicates that conversion failed. This is not an
indication that the statement failed.

### Structure `tnt_stmt`

This is a copy of a structure in
[tnt_fetch.h](https://github.com/tarantool/tarantool-c/blob/odbc/include/tarantool/tnt_fetch.h).

```c
/**
 * Structure for query handling.
*/
typedef struct tnt_stmt {
 struct tnt_stream *stream;
 struct tnt_reply *reply;
 const char *data;
 struct tnt_coldata *row;
 const char **field_names;
 int64_t a_rows; /* Affected rows */
 int32_t nrows;
 int32_t ncols;
 int32_t cur_row;
 char *query;
 int32_t query_len;
 /* int32_t ibind_len; */
 tnt_bind_t *ibind;
 /* int32_t obind_len */
 tnt_bind_t *obind;
 uint64_t reqid;
 int reply_state;
 int qtype;
 int error;
} tnt_stmt_t;
```

[tnt_prepare](#tnt_prepare) and [tnt_query](#tnt_query)
allocate `tnt_stmt_t` structures. Many of the other API routines
pass, as their first parameter, pointers to these `tnt_stmt_t` structures.

### Structure `col_type`

This is derived from enum CTYPES in
[tnt_fetch.h](https://github.com/tarantool/tarantool-c/blob/odbc/include/tarantool/tnt_fetch.h).

The CTYPES are on the left, their numeric values
are in the middle, the SQL data type name (if any) is on the right.

```c
TNTC_NIL = MP_NIL, 0
TNTC_BIGINT = MP_INT, 2
TNTC_UBIGINT = MP_UINT, 1
TNTC_BOOL = MP_BOOL, 7 BOOLEAN (recommended C type  = uint16_t)
TNTC_FLOAT = MP_FLOAT, 8 FLOAT or REAL (recommended C type = float)
TNTC_DOUBLE = MP_DOUBLE, 9 DOUBLE PRECISION (recommended C type = double)
TNTC_CHAR = MP_STR, 3 CHAR or TEXT or DATE or TIMESTAMP
TNTC_STR = MP_STR, 3 CHAR (recommended C type = char[])
TNTC_BIN = MP_BIN, 4 BLOB (recommended C type = char[])
TNTC_MP_MAX_TP = 64, 64
TNTC_SINT, 65 INTEGER (recommended C type = int32_t)
TNTC_UINT, 66
TNTC_INT, 66
TNTC_SSHORT, 67 SMALLINT (recommended C type = int16_t)
TNTC_USHORT, 68
TNTC_SHORT, 69
TNTC_SBIGINT, 70 BIGINT (recommended C type = int64_t)
TNTC_LONG, 71
TNTC_SLONG, 72
TNTC_ULONG 73
```

The CTYPE enumeration as defined in `tnt_fetch.h` has values for host C types.
One should use CTYPE enumeration to assign types for bind
variables, see [structure tnt_bind_t](#structure-tnt_bind_t).
It is legal to use MessagePack type values (MP_INT, MP_UINT etc.)
but we recommend: always use CTYPE equivalent values (TNTC_BIGINT etc.).

## List of ODBC functions

These are the ODBC functions that Tarantool currently supports.
A separate ODBC document may appear in future.

```c
SQLRETURN affected_rows(SQLHSTMT, SQLLEN *);
  Like ODBC SQLNumRows https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlrowcount-function?view=sql-server2017
SQLRETURN alloc_connect(SQLHENV, SQLHDBC *);
  Like ODBC SQLAllocConnect https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlallocconnect-function?view=sql-server2017
SQLRETURN alloc_env(SQLHENV *);
  Like ODBC SQLAllocEnv https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlallocenv-function?view=sql-server2017
SQLRETURN alloc_stmt(SQLHDBC, SQLHSTMT *);
  Like ODBC SQLAllocStmt https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlallocstmt-function?view=sql-server2017
SQLRETURN col_attribute(SQLHSTMT, SQLUSMALLINT, SQLUSMALLINT, SQLPOINTER, SQLSMALLINT ,SQLSMALLINT *, SQLLEN *);
  Like ODBC SQLColAttribute https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlfetch-function?view=sql-server2017
SQLRETURN column_info(SQLHSTMT, SQLUSMALLINT, SQLCHAR *, SQLSMALLINT, SQLSMALLINT *,SQLSMALLINT *, SQLULEN *, SQLSMALLINT *, SQLSMALLINT *);
  Like ODBC SQLGetDescRec https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlgetdescrec-function?view=sql-server2017
SQLRETURN env_get_attr(SQLHENV, SQLINTEGER, SQLPOINTER, SQLINTEGER, SQLINTEGER *);
  Like ODBC SQLGetEnvAttr https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlgetconnectattr-function?view=sql-server2017
SQLRETURN env_set_attr(SQLHENV, SQLINTEGER, SQLPOINTER, SQLINTEGER);
  LIke ODBC SQLSetEnvAttr https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlsetenvattr-function?view=sql-server2017
SQLRETURN free_connect(SQLHDBC);
  Like ODBC SQLFreeConnect https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlfreeconnect-function?view=sql-server2017
SQLRETURN free_env(SQLHENV);
  Like ODBC SQLFreeEnv https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlfreeenv-function?view=sql-server2017
SQLRETURN free_stmt(SQLHSTMT, SQLUSMALLINT);
  Like ODBC SQLFreeStmt https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlfreestmt-function?view=sql-server2017
SQLRETURN get_connect_attr(SQLHDBC hdbc, SQLINTEGER  att, SQLPOINTER val, SQLINTEGER len, SQLINTEGER *olen);
  Like ODBC SQLGetConnectAttr https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlgetconnectattr-function?view=sql-server2017
SQLRETURN get_data(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT, SQLPOINTER, SQLLEN, SQLLEN *);
  Like ODBC SQLGetData https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlgetdata-function?view=sql-server2017
SQLRETURN get_diag_field(SQLSMALLINT, SQLHANDLE, SQLSMALLINT, SQLSMALLINT, SQLPOINTER, SQLSMALLINT, SQLSMALLINT *);
  Like ODBC SQLGetDiagField https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlgetdiagfield-function?view=sql-server2017
SQLRETURN get_diag_rec(SQLSMALLINT ,SQLHANDLE ,SQLSMALLINT, SQLCHAR *, SQLINTEGER *,SQLCHAR *, SQLSMALLINT, SQLSMALLINT *);
  Like ODBC SQLGetDiagRec https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlgetdiagrec-function?view=sql-server2017
SQLRETURN num_cols(SQLHSTMT, SQLSMALLINT *);
  Like ODBC SQLNumResultCols https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlnumresultcols-function?view=sql-server2017
SQLRETURN num_params(SQLHSTMT ,SQLSMALLINT *);
  Like ODBC SQLNumParams https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlnumparams-function?view=sql-server2017
SQLRETURN odbc_dbconnect(SQLHDBC, SQLCHAR *, SQLSMALLINT, SQLCHAR *, SQLSMALLINT,SQLCHAR *, SQLSMALLINT);
  Like ODBC SQLConnect https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlconnect-function?view=sql-server2017
SQLRETURN odbc_disconnect(SQLHDBC);
  Like ODBC SQLDisconnect https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqldisconnect-function?view=sql-server2017
SQLRETURN set_connect_attr(SQLHDBC hdbc, SQLINTEGER att, SQLPOINTER val, SQLINTEGER len);
  Like ODBC SQLSetConnectAttr https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlsetconnectattr-function?view=sql-server2017
SQLRETURN stmt_execute(SQLHSTMT);
  Like ODBC SQLExecute https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlexecute-function?view=sql-server2017
SQLRETURN stmt_fetch(SQLHSTMT);
  Like ODBC SQLFetch https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlfetch-function?view=sql-server2017
SQLRETURN stmt_in_bind(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT, SQLSMALLINT, SQLSMALLINT ,SQLUINTEGER, SQLSMALLINT, SQLPOINTER, SQLINTEGER, SQLLEN *);
  Like ODBC SQLBindParameter https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlgetdata-function?view=sql-server2017
SQLRETURN stmt_out_bind(SQLHSTMT ,SQLUSMALLINT, SQLSMALLINT, SQLPOINTER, SQLLEN, SQLLEN *);
  Like SQLBindCol https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlrowcount-function?view=sql-server2017
SQLRETURN stmt_prepare(SQLHSTMT ,SQLCHAR  *, SQLINTEGER);
  Like ODBC SQLStmtPrepare https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/sqlprepare-function?view=sql-server2017
```
