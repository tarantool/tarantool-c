#!/usr/bin/env tarantool

-- This file has two purposes:
-- 1) Generate odbc.ini and odbcinst.ini
-- 2) Configure tarantool instance which would be used during
--    the ODBC tests.

local os = require('os')
local log = require('log')
local fio = require('fio')

-- local listen_port = os.getenv('LISTEN')
local listen_port = 33000

log.info(listen_port)

box.cfg {
    listen = listen_port,
    --listen              = os.getenv("LISTEN"),
}

require('console').listen(os.getenv('ADMIN'))
fiber = require('fiber')

-- Let's firstly generate odbc.ini and odbcinst.ini
local odbc_test_dir_path = fio.dirname(fio.abspath(arg[0]))
log.info("odbc_test_dir_path: " .. odbc_test_dir_path)

local odbc_ini_path = fio.pathjoin(odbc_test_dir_path, "odbc.ini")
log.info("odbc_ini_path: " .. odbc_ini_path)

local odbc_inst_path = fio.pathjoin(odbc_test_dir_path, "odbcinst.ini")
local driver_path = fio.pathjoin(fio.dirname(fio.dirname(odbc_test_dir_path)),
    "odbc", "libtnt_odbc.so")

local f = fio.open(odbc_ini_path, {"O_RDWR", "O_CREAT", "O_TRUNC"})
f:write([[
    [tarantoolTest]
    Description=Tarantool test DSN
    Trace=Yes
    Server=localhost
    Port=33000
    Database=test
    Log_filename=./tarantool_odbc.log
    Log_level=5" > odbc.ini
]])
f:close()

f = fio.open(odbc_inst_path, {"O_RDWR", "O_CREAT", "O_TRUNC"})
f:write(([[
    [Tarantool]
    Description     = Tarantool
    Driver         = %s"
]]):format(driver_path))
f:close()

--[[
box.schema.func.create('fiber.time')
box.schema.user.grant('guest','execute', 'function', 'fiber.time')

sp = box.schema.space.create('first', {id=512})
sp:create_index('primary', {})

box.schema.user.grant('guest','read,write', 'space', 'first')

box.schema.user.create("myamlya", {password='1234'});
]]--

lp = {
    test = 'test',
    test_empty = '',
    test_big = '123456789012345678901234567890123456789012345678901234567890' -- '1234567890' * 6
}

for k, v in pairs(lp) do
    if #box.space._user.index.name:select { k } == 0 then
        box.schema.user.create(k, { password = v })
        if k == 'test' then
            box.schema.user.grant('test', 'read,write,execute,create', 'universe')
        end
    end
end

if not box.space.test then
    local test = box.schema.space.create('test')
    test:create_index('primary', { type = 'TREE', unique = true, parts = { 1, 'unsigned' } })
    test:create_index('secondary', { type = 'TREE', unique = false, parts = { 2, 'unsigned', 3, 'string' } })
    box.schema.user.grant('test', 'read,write', 'space', 'test')
end

if not box.space.msgpack then
    local msgpack = box.schema.space.create('msgpack')
    msgpack:create_index('primary', { parts = { 1, 'unsigned' } })
    box.schema.user.grant('test', 'read,write', 'space', 'msgpack')
    msgpack:insert { 1, 'float as key', { [2.7] = { 1, 2, 3 } } }
    msgpack:insert { 2, 'array as key', { [{ 2, 7 }] = { 1, 2, 3 } } }
    msgpack:insert { 3, 'array with float key as key', { [{ [2.7] = 3, [7] = 7 }] = { 1, 2, 3 } } }
    msgpack:insert { 6, 'array with string key as key', { ['megusta'] = { 1, 2, 3 } } }
    box.schema.func.create('test_4')
    box.schema.user.grant('guest', 'execute', 'function', 'test_4');
end

function test_1()
    require('log').error('1')
    return true, {
        c = {
            ['106'] = { 1, 1428578535 },
            ['2'] = { 1, 1428578535 }
        },
        pc = {
            ['106'] = { 1, 1428578535, 9243 },
            ['2'] = { 1, 1428578535, 9243 }
        },
        s = { 1, 1428578535 },
        u = 1428578535,
        v = {}
    }, true
end

function test_2()
    return { k2 = 'v', k1 = 'v2' }
end

function test_3(x, y)
    return x + y
end

function test_4()
    return box.session.user()
end
