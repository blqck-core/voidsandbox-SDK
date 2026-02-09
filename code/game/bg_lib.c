// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#include "../qcommon/q_shared.h"
#include "bg_lib.h"

static char *med3(char *, char *, char *, cmp_t *);
static void swapfunc(char *, char *, int, int);

// clang-format off
#define swapcode(TYPE, a, b, n)      \
	{                                \
		long i = (n) / sizeof(TYPE); \
		TYPE *pa = (TYPE *)(a);      \
		TYPE *pb = (TYPE *)(b);      \
		do {                         \
			TYPE tmp = *pa;          \
			*pa++ = *pb;             \
			*pb++ = tmp;             \
		} while(--i > 0);            \
	}
// clang-format on

// clang-format off
#define SWAPINIT(a, es) swaptype = ((char *)(a) - (char *)0) % sizeof(long) || (es) % sizeof(long) ? 2 : ((es) == sizeof(long) ? 0 : 1)
// clang-format on

static void swapfunc(char *a, char *b, int n, int swaptype) {
	if(swaptype <= 1) swapcode(long, a, b, n) else swapcode(char, a, b, n)
}

// clang-format off
#define swap(a, b)                    \
	if(swaptype == 0) {               \
		long t = *(long *)(a);        \
		*(long *)(a) = *(long *)(b);  \
		*(long *)(b) = t;             \
	} else {                          \
		swapfunc(a, b, es, swaptype); \
	}

#define vecswap(a, b, n) \
	if((n) > 0) swapfunc(a, b, n, swaptype)
// clang-format on

static char *med3(char *a, char *b, char *c, cmp_t *cmp) { return cmp(a, b) < 0 ? (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a)) : (cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c)); }

void qsort(void *a, size_t n, size_t es, cmp_t *cmp) {
	char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
	int d, r, swaptype, swap_cnt;

loop:
	SWAPINIT(a, es);
	swap_cnt = 0;

	if(n < 7) {
		for(pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
			for(pl = pm; pl > (char *)a && cmp(pl - es, pl) > 0; pl -= es) swap(pl, pl - es);
		return;
	}

	pm = (char *)a + (n / 2) * es;
	if(n > 7) {
		pl = a;
		pn = (char *)a + (n - 1) * es;
		if(n > 40) {
			d = (n / 8) * es;
			pl = med3(pl, pl + d, pl + 2 * d, cmp);
			pm = med3(pm - d, pm, pm + d, cmp);
			pn = med3(pn - 2 * d, pn - d, pn, cmp);
		}
		pm = med3(pl, pm, pn, cmp);
	}

	swap(a, pm);

	pa = pb = (char *)a + es;
	pc = pd = (char *)a + (n - 1) * es;

	while(1) {
		while(pb <= pc && (r = cmp(pb, a)) <= 0) {
			if(r == 0) {
				swap_cnt = 1;
				swap(pa, pb);
				pa += es;
			}
			pb += es;
		}
		while(pb <= pc && (r = cmp(pc, a)) >= 0) {
			if(r == 0) {
				swap_cnt = 1;
				swap(pc, pd);
				pd -= es;
			}
			pc -= es;
		}
		if(pb > pc) break;
		swap(pb, pc);
		swap_cnt = 1;
		pb += es;
		pc -= es;
	}

	if(swap_cnt == 0) {
		for(pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
			for(pl = pm; pl > (char *)a && cmp(pl - es, pl) > 0; pl -= es) swap(pl, pl - es);
		return;
	}

	pn = (char *)a + n * es;

	r = (int)(pa - (char *)a);
	vecswap(a, pb - r, r);

	r = (int)(pd - pc);
	vecswap(pb, pn - r, r);

	if((r = (int)(pb - pa)) > (int)es) qsort(a, r / es, es, cmp);
	if((r = (int)(pd - pc)) > (int)es) {
		a = pn - r;
		n = r / es;
		goto loop;
	}
}

size_t strlen(const char *string) {
	const char *s;

	s = string;
	while(*s) {
		s++;
	}
	return s - string;
}

int strlenru(const char *string) {
	int result = 0;
	int i = 0;

	while(string[i] != '\0') {
		unsigned char c = string[i];

		if(c <= 127) {
			result += 1;
			i += 1;
		}

		else if(c >= 192 && c <= 223) {
			result += 1;
			i += 2;
		} else {
			i += 1;
		}
	}

	return result;
}

int ifstrlenru(const char *string) {
	int n, i, rucount;

	n = strlen(string);
	rucount = 0;

	for(i = 0; i < n; i++) {
		if(string[i] >= -64 && string[i] <= -1) rucount += 1;
	}
	return rucount;
}

char *strcat(char *strDestination, const char *strSource) {
	char *s;

	s = strDestination;
	while(*s) {
		s++;
	}
	while(*strSource) {
		*s++ = *strSource++;
	}
	*s = 0;
	return strDestination;
}

char *strcpy(char *strDestination, const char *strSource) {
	char *s;

	s = strDestination;
	while(*strSource) {
		*s++ = *strSource++;
	}
	*s = 0;
	return strDestination;
}

int strcmp(const char *string1, const char *string2) {
	while(*string1 == *string2 && *string1 && *string2) {
		string1++;
		string2++;
	}
	return *string1 - *string2;
}

char *strchr(const char *string, int c) {
	while(*string) {
		if(*string == c) {
			return (char *)string;
		}
		string++;
	}
	if(c)
		return NULL;
	else
		return (char *)string;
}

char *strrchr(const char *string, int c) {
	const char *found = NULL;

	while(*string) {
		if(*string == c) found = string;

		string++;
	}

	if(c)
		return (char *)found;
	else
		return (char *)string;
}

char *strstr(const char *string, const char *strCharSet) {
	while(*string) {
		int i;

		for(i = 0; strCharSet[i]; i++) {
			if(string[i] != strCharSet[i]) {
				break;
			}
		}
		if(!strCharSet[i]) {
			return (char *)string;
		}
		string++;
	}
	return (char *)0;
}

int tolower(int c) {
	if(c >= 'A' && c <= 'Z') {
		c += 'a' - 'A';
	}
	return c;
}

int toupper(int c) {
	if(c >= 'a' && c <= 'z') {
		c += 'A' - 'a';
	}
	return c;
}

void *memmove(void *dest, const void *src, size_t count) {
	int i;

	if(dest > src) {
		for(i = count - 1; i >= 0; i--) {
			((char *)dest)[i] = ((char *)src)[i];
		}
	} else {
		for(i = 0; i < count; i++) {
			((char *)dest)[i] = ((char *)src)[i];
		}
	}
	return dest;
}

double tan(double x) { return sin(x) / cos(x); }

static int randSeed = 0;

void srand(unsigned seed) { randSeed = seed; }

int rand(void) {
	randSeed = (69069 * randSeed + 1);
	return randSeed & 0x7fff;
}

double atof(const char *string) {
	float sign;
	float value;
	int c;

	// skip whitespace
	while(*string <= ' ') {
		if(!*string) {
			return 0;
		}
		string++;
	}

	// check sign
	switch(*string) {
	case '+':
		string++;
		sign = 1;
		break;
	case '-':
		string++;
		sign = -1;
		break;
	default: sign = 1; break;
	}

	// read digits
	value = 0;
	c = string[0];
	if(c != '.') {
		do {
			c = *string++;
			if(c < '0' || c > '9') {
				break;
			}
			c -= '0';
			value = value * 10 + c;
		} while(1);
	} else {
		string++;
	}

	// check for decimal point
	if(c == '.') {
		double fraction;

		fraction = 0.1;
		do {
			c = *string++;
			if(c < '0' || c > '9') {
				break;
			}
			c -= '0';
			value += c * fraction;
			fraction *= 0.1;
		} while(1);
	}

	// not handling 10e10 notation...

	return value * sign;
}

static double _atof(const char **stringPtr) {
	const char *string;
	float sign;
	float value;
	int c = '0';

	string = *stringPtr;

	// skip whitespace
	while(*string <= ' ') {
		if(!*string) {
			*stringPtr = string;
			return 0;
		}
		string++;
	}

	// check sign
	switch(*string) {
	case '+':
		string++;
		sign = 1;
		break;
	case '-':
		string++;
		sign = -1;
		break;
	default: sign = 1; break;
	}

	// read digits
	value = 0;
	if(string[0] != '.') {
		do {
			c = *string++;
			if(c < '0' || c > '9') {
				break;
			}
			c -= '0';
			value = value * 10 + c;
		} while(1);
	}

	// check for decimal point
	if(c == '.') {
		double fraction;

		fraction = 0.1;
		do {
			c = *string++;
			if(c < '0' || c > '9') {
				break;
			}
			c -= '0';
			value += c * fraction;
			fraction *= 0.1;
		} while(1);
	}

	// not handling 10e10 notation...
	*stringPtr = string;

	return value * sign;
}

int atoi(const char *string) {
	int sign;
	int value;
	int c;

	// skip whitespace
	while(*string <= ' ') {
		if(!*string) {
			return 0;
		}
		string++;
	}

	// check sign
	switch(*string) {
	case '+':
		string++;
		sign = 1;
		break;
	case '-':
		string++;
		sign = -1;
		break;
	default: sign = 1; break;
	}

	// read digits
	value = 0;
	do {
		c = *string++;
		if(c < '0' || c > '9') {
			break;
		}
		c -= '0';
		value = value * 10 + c;
	} while(1);

	// not handling 10e10 notation...

	return value * sign;
}

static int _atoi(const char **stringPtr) {
	int sign;
	int value;
	int c;
	const char *string;

	string = *stringPtr;

	// skip whitespace
	while(*string <= ' ') {
		if(!*string) {
			return 0;
		}
		string++;
	}

	// check sign
	switch(*string) {
	case '+':
		string++;
		sign = 1;
		break;
	case '-':
		string++;
		sign = -1;
		break;
	default: sign = 1; break;
	}

	// read digits
	value = 0;
	do {
		c = *string++;
		if(c < '0' || c > '9') {
			break;
		}
		c -= '0';
		value = value * 10 + c;
	} while(1);

	// not handling 10e10 notation...

	*stringPtr = string;

	return value * sign;
}

int abs(int n) { return n < 0 ? -n : n; }

double fabs(double x) { return x < 0 ? -x : x; }

/*
 * Copyright Patrick Powell 1995
 * This code is based on code written by Patrick Powell (papowell@astart.com)
 * It may be used for any purpose as long as this notice remains intact
 * on all source code distributions
 * New implementation by Patrick Powell and others for vsnprintf.
 * Supports length checking in strings.
 */

#define LDOUBLE double
#define LLONG long

static int dopr(char *buffer, size_t maxlen, const char *format, va_list args);
static int fmtstr(char *buffer, size_t *currlen, size_t maxlen, char *value, int flags, int min, int max);
static int fmtint(char *buffer, size_t *currlen, size_t maxlen, LLONG value, int base, int min, int max, int flags);
static int fmtfp(char *buffer, size_t *currlen, size_t maxlen, LDOUBLE fvalue, int min, int max, int flags);
static int dopr_outch(char *buffer, size_t *currlen, size_t maxlen, char c);

/*
 * dopr(): poor man's version of doprintf
 */

/* format read states */
#define DP_S_DEFAULT 0
#define DP_S_FLAGS 1
#define DP_S_MIN 2
#define DP_S_DOT 3
#define DP_S_MAX 4
#define DP_S_MOD 5
#define DP_S_MOD_L 6
#define DP_S_CONV 7
#define DP_S_DONE 8

/* format flags - Bits */
#define DP_F_MINUS (1 << 0)
#define DP_F_PLUS (1 << 1)
#define DP_F_SPACE (1 << 2)
#define DP_F_NUM (1 << 3)
#define DP_F_ZERO (1 << 4)
#define DP_F_UP (1 << 5)
#define DP_F_UNSIGNED (1 << 6)

/* Conversion Flags */
#define DP_C_SHORT 1
#define DP_C_LONG 2
#define DP_C_LLONG 3
#define DP_C_LDOUBLE 4

#define char_to_int(p) (p - '0')
#define MAX(p, q) ((p >= q) ? p : q)
#define MIN(p, q) ((p <= q) ? p : q)

static int dopr(char *buffer, size_t maxlen, const char *format, va_list args) {
	char ch;
	LLONG value;
	LDOUBLE fvalue;
	char *strvalue;
	int min;
	int max;
	int state;
	int flags;
	int cflags;
	int total;
	size_t currlen;

	state = DP_S_DEFAULT;
	currlen = flags = cflags = min = 0;
	max = -1;
	ch = *format++;
	total = 0;

	while(state != DP_S_DONE) {
		if(ch == '\0') state = DP_S_DONE;

		switch(state) {
		case DP_S_DEFAULT:
			if(ch == '%')
				state = DP_S_FLAGS;
			else
				total += dopr_outch(buffer, &currlen, maxlen, ch);
			ch = *format++;
			break;
		case DP_S_FLAGS:
			switch(ch) {
			case '-':
				flags |= DP_F_MINUS;
				ch = *format++;
				break;
			case '+':
				flags |= DP_F_PLUS;
				ch = *format++;
				break;
			case ' ':
				flags |= DP_F_SPACE;
				ch = *format++;
				break;
			case '#':
				flags |= DP_F_NUM;
				ch = *format++;
				break;
			case '0':
				flags |= DP_F_ZERO;
				ch = *format++;
				break;
			default: state = DP_S_MIN; break;
			}
			break;
		case DP_S_MIN:
			if('0' <= ch && ch <= '9') {
				min = 10 * min + char_to_int(ch);
				ch = *format++;
			} else if(ch == '*') {
				min = va_arg(args, int);
				ch = *format++;
				state = DP_S_DOT;
			} else
				state = DP_S_DOT;
			break;
		case DP_S_DOT:
			if(ch == '.') {
				state = DP_S_MAX;
				ch = *format++;
			} else
				state = DP_S_MOD;
			break;
		case DP_S_MAX:
			if('0' <= ch && ch <= '9') {
				if(max < 0) max = 0;
				max = 10 * max + char_to_int(ch);
				ch = *format++;
			} else if(ch == '*') {
				max = va_arg(args, int);
				ch = *format++;
				state = DP_S_MOD;
			} else
				state = DP_S_MOD;
			break;
		case DP_S_MOD:
			switch(ch) {
			case 'h':
				cflags = DP_C_SHORT;
				ch = *format++;
				break;
			case 'l':
				cflags = DP_C_LONG;
				ch = *format++;
				break;
			case 'L':
				cflags = DP_C_LDOUBLE;
				ch = *format++;
				break;
			default: break;
			}
			if(cflags != DP_C_LONG)
				state = DP_S_CONV;
			else
				state = DP_S_MOD_L;
			break;
		case DP_S_MOD_L:
			switch(ch) {
			case 'l':
				cflags = DP_C_LLONG;
				ch = *format++;
				break;
			default: break;
			}
			state = DP_S_CONV;
			break;
		case DP_S_CONV:
			switch(ch) {
			case 'd':
			case 'i':
				if(cflags == DP_C_SHORT)
					value = (short int)va_arg(args, int);
				else if(cflags == DP_C_LONG)
					value = va_arg(args, long int);
				else if(cflags == DP_C_LLONG)
					value = va_arg(args, LLONG);
				else
					value = va_arg(args, int);
				total += fmtint(buffer, &currlen, maxlen, value, 10, min, max, flags);
				break;
			case 'o':
				flags |= DP_F_UNSIGNED;
				if(cflags == DP_C_SHORT)
					value = va_arg(args, unsigned int) & ((1 << sizeof(unsigned short int) * 8) - 1);
				else if(cflags == DP_C_LONG)
					value = va_arg(args, unsigned long int);
				else if(cflags == DP_C_LLONG)
					value = va_arg(args, unsigned LLONG);
				else
					value = va_arg(args, unsigned int);
				total += fmtint(buffer, &currlen, maxlen, value, 8, min, max, flags);
				break;
			case 'u':
				flags |= DP_F_UNSIGNED;
				if(cflags == DP_C_SHORT)
					value = va_arg(args, unsigned int) & ((1 << sizeof(unsigned short int) * 8) - 1);
				else if(cflags == DP_C_LONG)
					value = va_arg(args, unsigned long int);
				else if(cflags == DP_C_LLONG)
					value = va_arg(args, unsigned LLONG);
				else
					value = va_arg(args, unsigned int);
				total += fmtint(buffer, &currlen, maxlen, value, 10, min, max, flags);
				break;
			case 'X': flags |= DP_F_UP;
			case 'x':
				flags |= DP_F_UNSIGNED;
				if(cflags == DP_C_SHORT)
					value = va_arg(args, unsigned int) & ((1 << sizeof(unsigned short int) * 8) - 1);
				else if(cflags == DP_C_LONG)
					value = va_arg(args, unsigned long int);
				else if(cflags == DP_C_LLONG)
					value = va_arg(args, unsigned LLONG);
				else
					value = va_arg(args, unsigned int);
				total += fmtint(buffer, &currlen, maxlen, value, 16, min, max, flags);
				break;
			case 'f':
				if(cflags == DP_C_LDOUBLE)
					fvalue = va_arg(args, LDOUBLE);
				else
					fvalue = va_arg(args, double);
				total += fmtfp(buffer, &currlen, maxlen, fvalue, min, max, flags);
				break;
			case 'E': flags |= DP_F_UP;
			case 'e':
				if(cflags == DP_C_LDOUBLE)
					fvalue = va_arg(args, LDOUBLE);
				else
					fvalue = va_arg(args, double);
				total += fmtfp(buffer, &currlen, maxlen, fvalue, min, max, flags);
				break;
			case 'G': flags |= DP_F_UP;
			case 'g':
				if(cflags == DP_C_LDOUBLE)
					fvalue = va_arg(args, LDOUBLE);
				else
					fvalue = va_arg(args, double);
				total += fmtfp(buffer, &currlen, maxlen, fvalue, min, max, flags);
				break;
			case 'c': total += dopr_outch(buffer, &currlen, maxlen, va_arg(args, int)); break;
			case 's':
				strvalue = va_arg(args, char *);
				total += fmtstr(buffer, &currlen, maxlen, strvalue, flags, min, max);
				break;
			case 'p':
				strvalue = va_arg(args, void *);
				total += fmtint(buffer, &currlen, maxlen, (long)strvalue, 16, min, max, flags);
				break;
			case 'n':
				if(cflags == DP_C_SHORT) {
					short int *num;
					num = va_arg(args, short int *);
					*num = currlen;
				} else if(cflags == DP_C_LONG) {
					long int *num;
					num = va_arg(args, long int *);
					*num = currlen;
				} else if(cflags == DP_C_LLONG) {
					LLONG *num;
					num = va_arg(args, LLONG *);
					*num = currlen;
				} else {
					int *num;
					num = va_arg(args, int *);
					*num = currlen;
				}
				break;
			case '%': total += dopr_outch(buffer, &currlen, maxlen, ch); break;
			case 'w': ch = *format++; break;
			default: break;
			}
			ch = *format++;
			state = DP_S_DEFAULT;
			flags = cflags = min = 0;
			max = -1;
			break;
		case DP_S_DONE: break;
		default: break;
		}
	}
	if(buffer != NULL) {
		if(currlen < maxlen - 1)
			buffer[currlen] = '\0';
		else
			buffer[maxlen - 1] = '\0';
	}
	return total;
}

static int fmtstr(char *buffer, size_t *currlen, size_t maxlen, char *value, int flags, int min, int max) {
	int padlen, strln;
	int cnt = 0;
	int total = 0;

	if(value == 0) {
		value = "<NULL>";
	}

	for(strln = 0; value[strln]; ++strln);
	if(max >= 0 && max < strln) strln = max;
	padlen = min - strln;
	if(padlen < 0) padlen = 0;
	if(flags & DP_F_MINUS) padlen = -padlen;

	while(padlen > 0) {
		total += dopr_outch(buffer, currlen, maxlen, ' ');
		--padlen;
	}
	while(*value && ((max < 0) || (cnt < max))) {
		total += dopr_outch(buffer, currlen, maxlen, *value++);
		++cnt;
	}
	while(padlen < 0) {
		total += dopr_outch(buffer, currlen, maxlen, ' ');
		++padlen;
	}
	return total;
}

static int fmtint(char *buffer, size_t *currlen, size_t maxlen, LLONG value, int base, int min, int max, int flags) {
	int signvalue = 0;
	unsigned LLONG uvalue;
	char convert[24];
	int place = 0;
	int spadlen = 0;
	int zpadlen = 0;
	const char *digits;
	int total = 0;

	if(max < 0) max = 0;

	uvalue = value;

	if(!(flags & DP_F_UNSIGNED)) {
		if(value < 0) {
			signvalue = '-';
			uvalue = -value;
		} else if(flags & DP_F_PLUS)
			signvalue = '+';
		else if(flags & DP_F_SPACE)
			signvalue = ' ';
	}

	if(flags & DP_F_UP)
		digits = "0123456789ABCDEF";
	else
		digits = "0123456789abcdef";

	do {
		convert[place++] = digits[uvalue % (unsigned)base];
		uvalue = (uvalue / (unsigned)base);
	} while(uvalue && (place < sizeof(convert)));
	if(place == sizeof(convert)) place--;
	convert[place] = 0;

	zpadlen = max - place;
	spadlen = min - MAX(max, place) - (signvalue ? 1 : 0);
	if(zpadlen < 0) zpadlen = 0;
	if(spadlen < 0) spadlen = 0;
	if(flags & DP_F_ZERO) {
		zpadlen = MAX(zpadlen, spadlen);
		spadlen = 0;
	}
	if(flags & DP_F_MINUS) spadlen = -spadlen; /* Left Justifty */

	while(spadlen > 0) {
		total += dopr_outch(buffer, currlen, maxlen, ' ');
		--spadlen;
	}

	if(signvalue) total += dopr_outch(buffer, currlen, maxlen, signvalue);

	if(zpadlen > 0) {
		while(zpadlen > 0) {
			total += dopr_outch(buffer, currlen, maxlen, '0');
			--zpadlen;
		}
	}

	while(place > 0) total += dopr_outch(buffer, currlen, maxlen, convert[--place]);

	while(spadlen < 0) {
		total += dopr_outch(buffer, currlen, maxlen, ' ');
		++spadlen;
	}

	return total;
}

static LDOUBLE abs_val(LDOUBLE value) {
	LDOUBLE result = value;

	if(value < 0) result = -value;

	return result;
}

static LDOUBLE pow10(int exp) {
	LDOUBLE result = 1;

	while(exp) {
		result *= 10;
		exp--;
	}

	return result;
}

static long round(LDOUBLE value) {
	long intpart;

	intpart = value;
	value = value - intpart;
	if(value >= 0.5) intpart++;

	return intpart;
}

static int fmtfp(char *buffer, size_t *currlen, size_t maxlen, LDOUBLE fvalue, int min, int max, int flags) {
	int signvalue = 0;
	LDOUBLE ufvalue;
	char iconvert[20];
	char fconvert[20];
	int iplace = 0;
	int fplace = 0;
	int padlen = 0;
	int zpadlen = 0;
	int caps = 0;
	int total = 0;
	long intpart;
	long fracpart;

	if(max < 0) max = 6;

	ufvalue = abs_val(fvalue);

	if(fvalue < 0)
		signvalue = '-';
	else if(flags & DP_F_PLUS)
		signvalue = '+';
	else if(flags & DP_F_SPACE)
		signvalue = ' ';

	intpart = ufvalue;

	if(max > 9) max = 9;

	fracpart = round((pow10(max)) * (ufvalue - intpart));

	if(fracpart >= pow10(max)) {
		intpart++;
		fracpart -= pow10(max);
	}

	do {
		iconvert[iplace++] = (caps ? "0123456789ABCDEF" : "0123456789abcdef")[intpart % 10];
		intpart = (intpart / 10);
	} while(intpart && (iplace < 20));
	if(iplace == 20) iplace--;
	iconvert[iplace] = 0;

	do {
		fconvert[fplace++] = (caps ? "0123456789ABCDEF" : "0123456789abcdef")[fracpart % 10];
		fracpart = (fracpart / 10);
	} while(fracpart && (fplace < 20));
	if(fplace == 20) fplace--;
	fconvert[fplace] = 0;

	padlen = min - iplace - max - 1 - ((signvalue) ? 1 : 0);
	zpadlen = max - fplace;
	if(zpadlen < 0) zpadlen = 0;
	if(padlen < 0) padlen = 0;
	if(flags & DP_F_MINUS) padlen = -padlen;

	if((flags & DP_F_ZERO) && (padlen > 0)) {
		if(signvalue) {
			total += dopr_outch(buffer, currlen, maxlen, signvalue);
			--padlen;
			signvalue = 0;
		}
		while(padlen > 0) {
			total += dopr_outch(buffer, currlen, maxlen, '0');
			--padlen;
		}
	}
	while(padlen > 0) {
		total += dopr_outch(buffer, currlen, maxlen, ' ');
		--padlen;
	}
	if(signvalue) total += dopr_outch(buffer, currlen, maxlen, signvalue);

	while(iplace > 0) total += dopr_outch(buffer, currlen, maxlen, iconvert[--iplace]);

	/*
	 * Decimal point.  This should probably use locale to find the correct
	 * char to print out.
	 */
	if(max > 0) {
		total += dopr_outch(buffer, currlen, maxlen, '.');

		while(zpadlen-- > 0) total += dopr_outch(buffer, currlen, maxlen, '0');

		while(fplace > 0) total += dopr_outch(buffer, currlen, maxlen, fconvert[--fplace]);
	}

	while(padlen < 0) {
		total += dopr_outch(buffer, currlen, maxlen, ' ');
		++padlen;
	}

	return total;
}

static int dopr_outch(char *buffer, size_t *currlen, size_t maxlen, char c) {
	if(*currlen + 1 < maxlen) buffer[(*currlen)++] = c;
	return 1;
}

int Q_vsnprintf(char *str, size_t length, const char *fmt, va_list args) {
	if(str != NULL) str[0] = 0;
	return dopr(str, length, fmt, args);
}

int Q_snprintf(char *str, size_t length, const char *fmt, ...) {
	va_list ap;
	int retval;

	va_start(ap, fmt);
	retval = Q_vsnprintf(str, length, fmt, ap);
	va_end(ap);

	return retval;
}

int sscanf(const char *buffer, const char *fmt, ...) {
	int cmd;
	va_list ap;
	int count;
	size_t len;

	va_start(ap, fmt);
	count = 0;

	while(*fmt) {
		if(fmt[0] != '%') {
			fmt++;
			continue;
		}

		fmt++;
		cmd = *fmt;

		if(isdigit(cmd)) {
			len = (size_t)_atoi(&fmt);
			cmd = *(fmt - 1);
		} else {
			len = MAX_STRING_CHARS - 1;
			fmt++;
		}

		switch(cmd) {
		case 'i':
		case 'd':
		case 'u': *(va_arg(ap, int *)) = _atoi(&buffer); break;
		case 'f': *(va_arg(ap, float *)) = _atof(&buffer); break;
		case 's': {
			char *s = va_arg(ap, char *);
			while(isspace(*buffer)) buffer++;
			while(*buffer && !isspace(*buffer) && len-- > 0) *s++ = *buffer++;
			*s++ = '\0';
			break;
		}
		}
	}

	va_end(ap);
	return count;
}
