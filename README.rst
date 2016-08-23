-------------------------------------------------------------------------------
                            Tarantool C client libraries
-------------------------------------------------------------------------------

.. image:: https://travis-ci.org/tarantool/tarantool-c.svg?branch=master
    :target: https://travis-ci.org/tarantool/tarantool-c

===========================================================
                        About
===========================================================

**Tarantool-c** is a client library written in C for Tarantool.
The current version is 1.0

Tarantool-c depends on `msgpuck <https://github.com/tarantool/msgpuck>`_.

For documentation, please, visit `github pages <http://tarantool.github.com/tarantool-c>`_.

It consinsts of:

* tnt - tarantool IProto/networking library

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       Compilation/Installation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This project using CMake for generating Makefiles:

.. code-block:: console

    $ cmake .
    $ make
    #### For testing against installed tarantool
    $ make test
    #### For building documentation using Doxygen
    $ make doc
    #### For installing into system (headers+libraries)
    $ make install

Or you can install it using yum/apt into your favorite linux distribution
from `tarantool`'s repository

===========================================================
                        Examples
===========================================================

Start tarantool at port 3301 using this command:

.. code-block:: lua

    box.cfg{ listen = '3301' }

After you've installed tarantool-c build and execute this code:

.. code-block:: c

    #include <stdlib.h>
    #include <stdio.h>

    #include <tarantool/tarantool.h>
    #include <tarantool/tnt_net.h>
    #include <tarantool/tnt_opt.h>

    int main() {
        const char * uri = "localhost:3301";
        struct tnt_stream * tnt = tnt_net(NULL); // Allocating stream
        tnt_set(tnt, TNT_OPT_URI, uri); // Setting URI
        tnt_set(tnt, TNT_OPT_SEND_BUF, 0); // Disable buffering for send
        tnt_set(tnt, TNT_OPT_RECV_BUF, 0); // Disable buffering for recv
        tnt_connect(tnt); // Initialize stream and connect to Tarantool
        tnt_ping(tnt); // Send ping request
        struct tnt_reply * reply = tnt_reply_init(NULL); // Initialize reply
        tnt->read_reply(tnt, reply); // Read reply from server
        tnt_reply_free(reply); // Free reply
        tnt_close(tnt); tnt_stream_free(tnt); // Close connection and free stream object
    }

For more examples, please, visit ``test/tarantool-tcp.c`` or ``test/tarantool-unix.c`` files.

For RPM/DEB packages - use instructions from http://tarantool.org/download.html to add repositories.
