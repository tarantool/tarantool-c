#include "unit.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <msgpuck.h>

#include <tarantool/tarantool.h>

#include <tarantool/tnt_net.h>
#include <tarantool/tnt_opt.h>
#include <tarantool/tnt_fetch.h>
#include "common.h"

#define header() note("*** %s: prep ***", __func__)
#define footer() note("*** %s: done ***", __func__)

extern int call_utest(void);

int main() {
	return call_utest();
}

