#!/usr/bin/env tarantool
os = require('os')

-- local listen_port = os.getenv('LISTEN')
local listen_port = 33000

require('log').info(listen_port)

box.cfg{
   listen           = listen_port,
   log_level        = 7,
 --  log           = 'tarantool.log',
}

require('console').listen(os.getenv('ADMIN'))

fiber = require('fiber')

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
   if #box.space._user.index.name:select{k} == 0 then
      box.schema.user.create(k, { password = v })
      if k == 'test' then
         box.schema.user.grant('test', 'read', 'space', '_space')
         box.schema.user.grant('test', 'read', 'space', '_index')
         box.schema.user.grant('test', 'read', 'space', '_truncate')
         box.schema.user.grant('test', 'execute', 'universe')
         box.schema.user.grant('test', 'write', 'universe')
      end
   end
end

if not box.space.test then
   local test = box.schema.space.create('test')
   test:create_index('primary',   {type = 'TREE', unique = true, parts = {1, 'unsigned'}})
   test:create_index('secondary', {type = 'TREE', unique = false, parts = {2, 'unsigned', 3, 'string'}})
   box.schema.user.grant('test', 'read,write', 'space', 'test')
end

if not box.space.msgpack then
   local msgpack = box.schema.space.create('msgpack')
   msgpack:create_index('primary', {parts = {1, 'unsigned'}})
   box.schema.user.grant('test', 'read,write', 'space', 'msgpack')
   msgpack:insert{1, 'float as key', {[2.7] = {1, 2, 3}}}
   msgpack:insert{2, 'array as key', {[{2, 7}] = {1, 2, 3}}}
   msgpack:insert{3, 'array with float key as key', {[{[2.7] = 3, [7] = 7}] = {1, 2, 3}}}
   msgpack:insert{6, 'array with string key as key', {['megusta'] = {1, 2, 3}}}
   box.schema.func.create('test_4')
   box.schema.user.grant('guest', 'execute', 'function', 'test_4');
end

function test_1()
    require('log').error('1')
    return true, {
        c= {
            ['106']= {1, 1428578535},
            ['2']= {1, 1428578535}
        },
        pc= {
            ['106']= {1, 1428578535, 9243},
            ['2']= {1, 1428578535, 9243}
        },
        s= {1, 1428578535},
        u= 1428578535,
        v= {}
    }, true
end

function test_2()
    return { k2= 'v', k1= 'v2'}
end

function test_3(x, y)
    return x + y
end

function test_4()
    return box.session.user()
end
