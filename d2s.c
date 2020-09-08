#define _D2S_DEBUG

#include <string.h>

#ifdef D2S_DEBUG
	#include <stdio.h>
	#define debug printf
#else
	#define debug
#endif

#define MAX(a, b) ((a) < (b) ? (b) : (a))

typedef struct {
	long l;
	char d[769];
} bigdec;

static bigdec M, L, U, S;
static char outbuf[1 + 17 + 1 + 17];

static void bigdec_init(bigdec *o, long i)
{
	o->l = 0;
	
	while(i) {
		o->d[o->l] = i % 10;
		i /= 10;
		o->l ++;
	}
}

#ifdef D2S_DEBUG
static void bigdec_debug(bigdec *o, char *p)
{
	long i;
	
	debug("%s", p);
	
	for(i = o->l - 1; i >= 0; i--) {
		debug("%i", o->d[i]);
	}
	
	debug("\n");
}
#endif

static void bigdec_mul(bigdec *o, long n)
{
	long i;
	long c = 0;
	
	for(i=0; i < o->l; i++) {
		long d = o->d[i];
		
		d *= n;
		d += c;
		c = d / 10;
		d %= 10;
		o->d[i] = d;
	}
	
	if(c) {
		o->d[o->l] = c;
		o->l ++;
	}
}

static char bigdec_digit(bigdec *o, long i)
{
	if(i < o->l) {
		return o->d[i];
	}
	
	return 0;
}

static long bigdec_cmp(bigdec *l, bigdec *r)
{
	long maxl = MAX(l->l, r->l);
	long i;
	char ld, rd;
	
	for(i = maxl - 1; i >= 0; i--) {
		ld = bigdec_digit(l, i);
		rd = bigdec_digit(r, i);
		
		if(ld < rd) return -1;
		if(ld > rd) return +1;
	}
	
	return 0;
}

char *d2s(double f, int* len_out)
{
	long e2, e10 = 0, m2, u, l;
	long j, k, p, ld, ud, tz, md;
	long ndl, ndr;
	
	/*
		Step 1
		
		Extract parts of the float number
	*/
	
	long i = *(long*)&f;
	long m = i & 0xfFFffFFffFFff;
	long e = i >> 52 & 0x7ff;
	long s = i >> 63 & 1;
	
#ifdef D2S_DEBUG
	debug("m = %li, e = %li, s = %li\n", m, e, s);
#endif
	
	/*
		Step 2
		
		Catch some special cases:
		signed infinity, nan, signed zero
	*/
	
	if(e == 0x7ff) {
		if(m == 0) {
			if(s == 0) {
				return "Infinity";
			}
			
			return  "-Infinity";
		}
		
		return "NaN";
	}
	else if(e == 0 && m == 0) {
		if(s == 0) {
			return "0";
		}
		
		return "-0";
	}
	
	/*
		Step 3
		
		Unify denormal and normal values and
		bring number into the form
			(-1)^s * m2 * 2^e2 
		where m2 and e2 are integers,
		Also calculate halfway points u and l
	*/
	
	if(e == 0) {
		e2 = 1 - 1023 - 52 - 1;
		m2 = m << 1;
		u  = m2 + 1;
		l  = m2 - 1;
	}
	else {
		e2 = e - 1023 - 52 - 2;
		m2 = m + (1L << 52) << 2;
		u  = m2 + 2;
		
		if(e == 0 && m > 1) {
			l = m2 - 1;
		}
		else {
			l = m2 - 2;
		}
	}
	
	/*
		minimum l :
			= 1
		maximum u : (0xfFFffFFffFFff + (1 << 52) << 2) + 2
			= 0x7fFFffFFffFFfe
		minimum e2: 1 - 1023 - 52 - 2
			= -1076
		maximum e2: 0x7fe - 1023 - 52 - 2
			= 969
	*/
	
#ifdef D2S_DEBUG
	debug("l = %c %li * 2 ** %li\n", s ? '-' : '+', l,  e2);
	debug("m = %c %li * 2 ** %li\n", s ? '-' : '+', m2, e2);
	debug("u = %c %li * 2 ** %li\n", s ? '-' : '+', u,  e2);
#endif
	
	/*
		Step 4
		
		Transform number from base-2 to base-10
		with the help of big decimal functions
		so we won't lose precision
	*/
	
	bigdec_init(&M, m2);
	bigdec_init(&L, l);
	bigdec_init(&U, u);
	
	if(e2 < 0) {
		while(e2 < 0) {
			bigdec_mul(&M, 5);
			bigdec_mul(&L, 5);
			bigdec_mul(&U, 5);
			e2 ++;
			e10 --;
		}
	}
	else if(e2 > 0) {
		while(e2 > 0) {
			bigdec_mul(&M, 2);
			bigdec_mul(&L, 2);
			bigdec_mul(&U, 2);
			e2 --;
		}
	}
	
#ifdef D2S_DEBUG
	bigdec_debug(&L, "L = ");
	bigdec_debug(&M, "M = ");
	bigdec_debug(&U, "U = ");
#endif
	
	/*
		Step 5
		
		Find shortest decimal representation
		of the transformed bigdec number
	*/
	
	bigdec_init(&S, 0);
	S.l = MAX(L.l, U.l);
	
	for(j = S.l - 1; j >= 0; j--) {
		ld = bigdec_digit(&L, j);
		md = bigdec_digit(&M, j);
		ud = bigdec_digit(&U, j);
		
		if(ld == ud) {
			S.d[j] = ld;
		}
		else {
			if(ld < md && md < ud) {
				S.d[j] = md;
			}
			else {
				S.d[j] = ud;
			}
			
			tz = j;
			break;
		}
	}
	
	while(j > 0) {
		j --;
		S.d[j] = 0;
	}
	
#ifdef D2S_DEBUG
	bigdec_debug(&S, "S = ");
#endif
	
	/*
		num of digits left to the point  = S.l + e10
		num of digits right to the point = -e10 - tz
	*/
	
	ndl = S.l + e10;
	ndr = -e10 - tz;
	
#ifdef D2S_DEBUG
	debug("e10 = %li\n", e10);
	debug("S.l = %li\n", S.l);
	debug("ndl = %li\n", ndl);
	debug("ndr = %li\n", ndr);
#endif
	
	/*
		it seems that there are never more than 17 significand digits
	*/
	
	/*
		Step 6
		
		Build decimal string
	*/
	
	k = 0;
	
	if(s) {
		outbuf[k++] = '-';
	}
	
	if(ndl > 17 || ndr > 17) {
		/* scientific format */
		
		outbuf[k++] = S.d[S.l - 1] + '0';
		
		if(tz < S.l - 1) {
			outbuf[k++] = '.';
			
			for(j = S.l - 2; j >= tz; j--) {
				outbuf[k++] = S.d[j] + '0';
			}
		}
		
		e10 += S.l - 1;
		
		if(e10) {
			outbuf[k++] = 'e';
			
			if(e10 < 0) {
				outbuf[k++] = '-';
				e10 *= -1;
			}
			
			if(e10 > 99) outbuf[k++] = e10 / 100  % 10 + '0';
			if(e10 > 9)  outbuf[k++] = e10 / 10   % 10 + '0';
			
			outbuf[k++] = e10 % 10 + '0';
		}
	}
	else {
		/* normal format */
		
		if(ndl < 1) {
			outbuf[k++] = '0';
		}
		else {
			for(j=0; j < ndl; j++) {
				outbuf[k++] = S.d[S.l - j - 1] + '0';
			}
		}
		
		if(ndr > 0) {
			outbuf[k++] = '.';
			
			for(j=0; j < ndr; j++) {
				p = -e10 - j - 1;
				
				if(p < S.l) {
					outbuf[k++] = S.d[p] + '0';
				}
				else {
					outbuf[k++] = '0';
				}
			}
		}
	}
	
	outbuf[k++] = 0;
	
	if(len_out) {
		*len_out = k - 1;
	}
	
	return outbuf;
}
