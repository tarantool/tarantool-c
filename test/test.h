#ifndef TEST_H_INCLUDED
#define TEST_H_INCLUDED

#include <stdio.h>

/**
@brief example

@code
	#include "test.h"

	int main(void) {
		plan(3);		// count of test You planned to check
		ok(1, "Test name 1");
		is(4, 2 * 2, "2 * 2 == 4");
		isnt(5, 2 * 2, "2 * 2 != 5);
		return check_plan();	// print resume
	}
@endcode


*/

/* private function, use ok(...) instead */
int __ok(int condition, const char *fmt, ...);

/* private function, use note(...) or diag(...) instead */
void __space(FILE *stream);

#define msg(stream, ...) do { __space(stream); fprintf(stream, "# ");            \
	fprintf(stream, __VA_ARGS__); fprintf(stream, "\n"); } while(0)

#define note(...) msg(stdout, __VA_ARGS__)
#define diag(...) msg(stderr, __VA_ARGS__)

/**
@brief set and print plan
@param count
Before anything else, you need a testing plan.  This basically declares
how many tests your program is going to run to protect against premature
failure.
*/
void plan(int count);

/**
@brief check if plan is reached and print report
*/
int check_plan(void);
/* Ok there is no easy way to make common solution for portable __VA_OPT__. So let's 
 * define here two sets of practically identical macros.
 */


#define ok(condition, fmt, ...)	do {		\
	int res = __ok(condition, fmt, ##__VA_ARGS__);		\
	if (!res) {					\
		__space(stderr);			\
		fprintf(stderr, "#   Failed test '");	\
		fprintf(stderr, fmt, ##__VA_ARGS__);		\
		fprintf(stderr, "'\n");			\
		__space(stderr);			\
		fprintf(stderr, "#   in %s at line %d\n", __FILE__, __LINE__); \
	}						\
	res = res;					\
}while(0)

#define is(a, b, fmt, ...)	do{			\
	int res = __ok((a) == (b), fmt, ##__VA_ARGS__);	\
	if (!res) {					\
		__space(stderr);			\
		fprintf(stderr, "#   Failed test '");	\
		fprintf(stderr, fmt, ##__VA_ARGS__);		\
		fprintf(stderr, "'\n");			\
		__space(stderr);			\
		fprintf(stderr, "#   in %s at line %d\n", __FILE__, __LINE__); \
	}						\
	res = res;					\
}while(0)

#define isnt(a, b, fmt, ...) do{			\
	int res = __ok((a) != (b), fmt, ##__VA_ARGS__);	\
	if (!res) {					\
		__space(stderr);			\
		fprintf(stderr, "#   Failed test '");	\
		fprintf(stderr, fmt, ##__VA_ARGS__);		\
		fprintf(stderr, "'\n");			\
		__space(stderr);			\
		fprintf(stderr, "#   in %s at line %d\n", __FILE__, __LINE__); \
	}						\
	res = res;					\
}while(0)

#define fail(fmt, ...)		\
	ok(0, fmt, ##__VA_ARGS__)


#endif /* TEST_H_INCLUDED */

