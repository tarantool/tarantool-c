#include <stdio.h>
#include <string.h>

#define OK 0
#define FAIL -1

int
int_sql_regexp(const char *p, const char *p_e, const char *t, const char *t_e)
{
	fprintf(stderr, "pattern '%s' text '%s'\n", p, t);
	if (p == p_e && t == t_e)
		return 1;
	if (p == p_e && t != t_e)
		return 0;

	if (*p == '%') {
		if (int_sql_regexp(p+1, p_e, t, t_e)) /* zero string match */
			return 1;
		else if (t!=t_e)
			return int_sql_regexp(p, p_e, t+1, t_e) /* one symblol match */
				|| int_sql_regexp(p+1, p_e, t+1, t_e);
		else
			return 0;

	} else if (t != t_e && (*p == '_' || *p == *t))
		return int_sql_regexp(p+1, p_e, t+1, t_e);
	else
		return 0;
}

int
sql_regexp(const char *pattern,  const char *text)
{
	return int_sql_regexp(pattern, pattern + strlen(pattern),
			     text, text + strlen(text));
}

int
main(int ac, char *av[])
{
	printf("%d\n", sql_regexp(av[1], av[2]));
}
