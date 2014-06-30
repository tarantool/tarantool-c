#ifndef LIBTB_H_
#define LIBTB_H_

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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define TB_INCLUDE(NAME) <TB_PREFIX/NAME>

#ifdef TB_LOCAL
# define TB_PREFIX TB_LOCAL
#else
# define TB_PREFIX tarantool
#endif

#include TB_INCLUDE(tp.1.5.h)
#include TB_INCLUDE(file.h)
#include TB_INCLUDE(session.h)
#include TB_INCLUDE(console.h)
#include TB_INCLUDE(utf8.h)
#include TB_INCLUDE(queue.h)
#include TB_INCLUDE(lex.h)

#undef TP_INCLUDE
#undef TP_PREFIX

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
