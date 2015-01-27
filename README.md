##Tarantool C connector [![Build Status](https://travis-ci.org/tarantool/tarantool-c.png?branch=master)](https://travis-ci.org/tarantool/tarantool-c)

A set of small libraries to interact with Tarantool v1.6.

Repository contents:

src/tp.h - a self-sufficient single-header serializer/deserializer implementation
for Tarantool v1.5 protocol

src/iproto.h - tarantool v1.6 constants and reply unpacker to use with
https://github.com/tarantool/msgpuck

src/file.h - tarantool v1.6 log/snapshot reader

src/session.h - bufferized network io
