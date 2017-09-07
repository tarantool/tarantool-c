#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include <msgpuck.h>

#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>

#include "common.h"
#include "tnt_assoc.h"

void
hex_dump (const char *desc, const char *addr, size_t len) {
	size_t i;
	unsigned char buff[17];
	unsigned char *pc = (unsigned char*)addr;

	if (desc != NULL)
		printf ("%s:\n", desc);
	for (i = 0; i < len; i++) {
		if ((i % 16) == 0) {
			if (i != 0) printf ("  %s\n", buff);
			printf ("  %04x ", (unsigned int)i);
		}
		printf (" %02x", pc[i]);
		if ((pc[i] < 0x20) || (pc[i] > 0x7e))
			buff[i % 16] = '.';
		else
			buff[i % 16] = pc[i];
		buff[(i % 16) + 1] = '\0';
	}
	while ((i % 16) != 0) {
		printf ("   ");
		i++;
	}
	printf ("  %s\n", buff);
}

void
hex_dump_c (const char *addr, size_t len) {
	const char *addr_end = addr + len;
	size_t n = 0;
	while (addr < addr_end) {
		if (n == 0) { printf("\""); n = 14; }
		printf("\\x%02x", *(unsigned char *)addr);
		addr += 1; n -= 1;
		if (n == 0) { printf("\"\n"); }
	}
	if (n != 0) { printf("\"\n"); }
}

static inline void
mp_dump_hex (const char *addr, size_t len) {
	const char *addr_end = addr + len;
	printf("\"");
	while (addr < addr_end) printf("\\x%02x", *(unsigned char *)addr++);
	printf("\"");
}

void
mp_dump (const char *addr, size_t len) {
	printf("mp_dump_begin\n  ");
	const char *addr_end = addr + len;
	struct mp_dump_stack stack[128];
	memset(stack, 0, sizeof(struct mp_dump_stack) * 128);
	size_t stack_size = 0;
	char begin = 1;
	while (addr < addr_end) {
		if (begin) {
			begin = 0;
		} else {
			if (stack_size > 0) {
				while (stack_size > 0 && stack[stack_size - 1].lft == 0) {
					switch (stack[stack_size - 1].type) {
					case (MP_ARRAY):
						printf("]");
						break;
					case (MP_MAP):
						printf("}");
						break;
					default:
						printf("???");
					}
					stack_size -= 1;
				}
				if (stack_size > 0 && stack[stack_size - 1].type == MP_MAP) {
					if (stack[stack_size - 1].lft % 2) {
						printf(":");
					} else if (!stack[stack_size - 1].start_flag) {
						printf(",");
					}
					if (!stack[stack_size - 1].start_flag)
						printf(" ");
					stack[stack_size - 1].start_flag = 0;
					stack[stack_size - 1].lft -= 1;
				}
			}
			if (!stack_size) {
				printf("\n  ");
			}
		}
		switch (mp_typeof(*addr)) {
		case (MP_NIL):
			printf("nil");
			mp_decode_nil(&addr);
			break;
		case (MP_UINT):
			printf("%zd", mp_decode_uint(&addr));
			break;
		case (MP_INT):
			printf("%zd", mp_decode_int(&addr));
			break;
		case (MP_STR): {
			uint32_t str_len = 0;
			const char *str = mp_decode_str(&addr, &str_len);
			printf("\"%.*s\"", str_len, str);
			break;
		}
		case (MP_BIN): {
			uint32_t bin_len = 0;
			const char *bin = mp_decode_bin(&addr, &bin_len);
			mp_dump_hex(bin, bin_len);
			break;
		}
		case (MP_ARRAY):
			stack[stack_size++] = (struct mp_dump_stack){
				.type = MP_ARRAY,
				.start_flag = 1,
				.lft = mp_decode_array(&addr)
			};
			printf("[");
			break;
		case (MP_MAP):
			stack[stack_size++] = (struct mp_dump_stack){
				.type = MP_MAP,
				.start_flag = 1,
				.lft = mp_decode_map(&addr) * 2
			};
			printf("{");
			break;
		case (MP_BOOL):
			if (mp_decode_bool(&addr)) {
				printf ("true");
			} else {
				printf ("false");
			}
			break;
		case (MP_FLOAT):
			printf("%f", mp_decode_float(&addr));
			break;
		case (MP_DOUBLE):
			printf("%f", mp_decode_double(&addr));
			break;
		case (MP_EXT):
			printf("ext");
			mp_next(&addr);
			break;
		default:
			printf("whattheheck");
			mp_next(&addr);
		}
	}
	while (stack_size > 0 && stack[stack_size - 1].lft == 0) {
		switch (stack[stack_size - 1].type) {
		case (MP_ARRAY):
			printf("]");
			break;
		case (MP_MAP):
			printf("}");
			break;
		default:
			printf("???");
		}
		stack_size -= 1;
	}
	printf("\n");
}

int
check_bbytes(const char *buf, size_t buf_size, const char *bb, size_t bb_size) {
	if (bb == NULL) goto error;
	if (buf_size != bb_size) goto error;
	if (memcmp(buf, bb, bb_size)) goto error;
	return 0;
error:
	if (bb) {
		hex_dump("expected", bb, bb_size);
		mp_dump(bb, bb_size);
	}
	hex_dump("got", (const char *)buf, buf_size);
	mp_dump(buf, buf_size);
	hex_dump_c((const char *)buf, buf_size);
	return -1;
}

int
check_rbytes(struct tnt_reply *s, const char *bb, size_t bb_size) {
	return check_bbytes(s->buf, s->buf_size, bb, bb_size);
}

int
check_sbytes(struct tnt_stream *s, const char *bb, size_t bb_size) {
	return check_bbytes(TNT_SBUF_DATA(s), TNT_SBUF_SIZE(s), bb, bb_size);
}

int
check_nbytes(struct tnt_stream *s, const char *bb, size_t bb_size) {
	struct tnt_stream_net *sn = TNT_SNET_CAST(s);
	return check_bbytes(sn->sbuf.buf, sn->sbuf.off, bb, bb_size);
}

int dump_schema_index(struct tnt_schema_sval *sval) {
	mh_int_t ipos = 0;
	mh_foreach(sval->index, ipos) {
		struct tnt_schema_ival *ival = NULL;
		ival = (*mh_assoc_node(sval->index, ipos))->data;
		printf("    %d: %s\n", ival->number, ival->name);
	}
	return 0;
}

int dump_schema(struct tnt_stream *s) {
	struct mh_assoc_t *schema = (TNT_SNET_CAST(s)->schema)->space_hash;
	mh_int_t spos = 0;
	mh_foreach(schema, spos) {
		struct tnt_schema_sval *sval = NULL;
		sval = (*mh_assoc_node(schema, spos))->data;
		printf("  %d: %s\n", sval->number, sval->name);
		(void )dump_schema_index(sval);
	}
	return 0;
}
