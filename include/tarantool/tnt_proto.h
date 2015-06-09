#ifndef TNT_PROTO_H_INCLUDED
#define TNT_PROTO_H_INCLUDED

/*
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* header */
enum tnt_header_key_t {
	TNT_CODE = 0x00,
	TNT_SYNC = 0x01
};

/* request body */
enum tnt_body_key_t {
	TNT_SPACE = 0x10,
	TNT_INDEX = 0x11,
	TNT_LIMIT = 0x12,
	TNT_OFFSET = 0x13,
	TNT_ITERATOR = 0x14,
	TNT_INDEX_BASE = 0x15,
	TNT_KEY = 0x20,
	TNT_TUPLE = 0x21,
	TNT_FUNCTION = 0x22,
	TNT_USERNAME = 0x23,
	TNT_EXPRESSION = 0x27,
	TNT_SERVER_ID = 0x02,
	TNT_LSN = 0x03,
	TNT_TIMESTAMP = 0x04,
	TNT_SERVER_UUID = 0x24,
	TNT_CLUSTER_UUID = 0x25,
	TNT_VCLOCK = 0x26
};

/* response body */
enum tnt_response_key_t {
	TNT_DATA = 0x30,
	TNT_ERROR = 0x31
};

/* request types */
enum tnt_request_type {
	TNT_OP_SELECT = 1,
	TNT_OP_INSERT = 2,
	TNT_OP_REPLACE = 3,
	TNT_OP_UPDATE = 4,
	TNT_OP_DELETE = 5,
	TNT_OP_CALL = 6,
	TNT_OP_AUTH = 7,
	TNT_OP_EVAL = 8,
	TNT_OP_PING = 64,
	TNT_OP_JOIN = 65,
	TNT_OP_SUBSCRIBE = 66
};

enum tnt_update_op_t {
	TNT_UOP_ADDITION = '+',
	TNT_UOP_SUBSTRACT = '-',
	TNT_UOP_AND = '&',
	TNT_UOP_XOR = '^',
	TNT_UOP_OR = '|',
	TNT_UOP_DELETE = '#',
	TNT_UOP_INSERT = '!',
	TNT_UOP_ASSIGN = '=',
	TNT_UOP_SPLICE = ':',
};

enum tnt_iterator_t {
	TNT_ITER_EQ = 0,
	TNT_ITER_REQ,
	TNT_ITER_ALL,
	TNT_ITER_LT,
	TNT_ITER_LE,
	TNT_ITER_GE,
	TNT_ITER_GT,
	TNT_ITER_BITS_ALL_SET,
	TNT_ITER_BITS_ANY_SET,
	TNT_ITER_BITS_ALL_NOT_SET,
	TNT_ITER_OVERLAP,
	TNT_ITER_NEIGHBOR,
};

#define TNT_SCRAMBLE_SIZE 20
#define TNT_GREETING_SIZE 128
#define TNT_VERSION_SIZE  64
#define TNT_SALT_SIZE     44

enum tnt_spaces_t {
	tnt_sp_space = 280,
	tnt_sp_index = 288,
	tnt_sp_func  = 296,
	tnt_sp_user  = 304,
	tnt_sp_priv  = 312,
	tnt_vsp_space = 281,
	tnt_vsp_index = 289,
	tnt_vsp_func  = 297,
	tnt_vsp_user  = 305,
	tnt_vsp_priv  = 313,
};

enum tnt_indexes_t {
	tnt_vin_primary = 0,
	tnt_vin_owner   = 1,
	tnt_vin_name    = 2,
};

#endif /* TNT_PROTO_H_INCLUDED */
