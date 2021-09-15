#include "test.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <errno.h>

#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include <tarantool/tnt_opt.h>

#include "common.h"

#define header() note("*** %s: prep ***", __func__)
#define footer() note("*** %s: done ***", __func__)

static const int STREAM_COUNT = 2048;

static int
test_poll() {
	plan(1);
	header();

	int ok = 1;

	char uri[128] = {0};
	snprintf(uri, 128, "test:test@%s", getenv("LISTEN"));

	struct tnt_stream *streams[STREAM_COUNT];
	memset(streams, 0, sizeof(struct tnt_stream *) * STREAM_COUNT);

	int last = -1;

	for (int i = 0; i < STREAM_COUNT; ++i) {
		struct tnt_stream *tnt = tnt_net(NULL);

		assert(tnt != NULL);
		if (tnt_set(tnt, TNT_OPT_URI, uri) != 0) {
			fprintf(stderr, "failed to set uri\n");
			ok = 0;
			tnt_stream_free(tnt);
			goto done;
		}
		if (tnt_set(tnt, TNT_OPT_SEND_BUF, 0) != 0) {
			fprintf(stderr, "failed to set sendbuf size\n");
			ok = 0;
			tnt_stream_free(tnt);
			goto done;
		}
		if (tnt_set(tnt, TNT_OPT_RECV_BUF, 0) != 0) {
			fprintf(stderr, "failed to set rcvbuf size\n");
			ok = 0;
			tnt_stream_free(tnt);
			goto done;
		}

		struct timeval tv;
		tv.tv_sec = 10;
		tv.tv_usec = 0;

		if (tnt_set(tnt, TNT_OPT_TMOUT_CONNECT, &tv) != 0) {
			fprintf(stderr, "failed to set timeout connect\n");
			ok = 0;
			tnt_stream_free(tnt);
			goto done;
		}

		int ret = tnt_connect(tnt);
		if (ret != 0) {
			diag("tnt_connect: %s", strerror(errno));
			ok = 0;
			tnt_stream_free(tnt);
			goto done;
		}

		/*
		 * After successful connect save the stream to
		 * close then.
		 */
		streams[i] = tnt;
		last = i;

		tnt_ping(tnt);
		tnt_flush(tnt);
		struct tnt_reply *reply = tnt_reply_init(NULL);
		if (tnt->read_reply(tnt, reply) != 0) {
			fprintf(stderr, "failed to read reply\n");
			ok = 0;
			goto done;
		}
		tnt_reply_free(reply);
	}

done:
	is(1, ok, "expected non-assert test");

	while (last > -1) {
		if (streams[last] != NULL) {
			tnt_close(streams[last]);
			tnt_stream_free(streams[last]);
		}
		--last;
	}

	return check_plan();
}

int main()
{
	plan(1);

	test_poll();

	return 0;
}

