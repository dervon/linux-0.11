/*                                                                          //  1/ 
 *  linux/kernel/vsprintf.c                                                 //  2/ 
 *                                                                          //  3/ 
 *  (C) 1991  Linus Torvalds                                                //  4/ 
 */                                                                         //  5/ 
                                                                            //  6/ 
/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */                        //  7/ 
/*                                                                          //  8/ 
 * Wirzenius wrote this portably, Torvalds fucked it up :-)                 //  9/ 
 */                                                                         // 10/ 
                                                                            // 11/ 
#include <stdarg.h>                                                         // 12/ 
#include <string.h>                                                         // 13/ 
                                                                            // 14/ 
/* we use this so that we can do without the ctype library */               // 15/ 
#define is_digit(c)	((c) >= '0' && (c) <= '9')                          // 16/ 
                                                                            // 17/ 
static int skip_atoi(const char **s)                                        // 18/ 
{                                                                           // 19/ 
	int i=0;                                                            // 20/ 
                                                                            // 21/ 
	while (is_digit(**s))                                               // 22/ 
		i = i*10 + *((*s)++) - '0';                                 // 23/ 
	return i;                                                           // 24/ 
}                                                                           // 25/ 
                                                                            // 26/ 
#define ZEROPAD	1		/* pad with zero */                         // 27/ 
#define SIGN	2		/* unsigned/signed long */                  // 28/ 
#define PLUS	4		/* show plus */                             // 29/ 
#define SPACE	8		/* space if plus */                         // 30/ 
#define LEFT	16		/* left justified */                        // 31/ 
#define SPECIAL	32		/* 0x */                                    // 32/ 
#define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */      // 33/ 
                                                                            // 34/ 
#define do_div(n,base) ({ \                                                 // 35/ 
int __res; \                                                                // 36/ 
__asm__("divl %4":"=a" (n),"=d" (__res):"0" (n),"1" (0),"r" (base)); \      // 37/ 
__res; })                                                                   // 38/ 
                                                                            // 39/ 
static char * number(char * str, int num, int base, int size, int precision // 40/ 
	,int type)                                                          // 41/ 
{                                                                           // 42/ 
	char c,sign,tmp[36];                                                // 43/ 
	const char *digits="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";          // 44/ 
	int i;                                                              // 45/ 
                                                                            // 46/ 
	if (type&SMALL) digits="0123456789abcdefghijklmnopqrstuvwxyz";      // 47/ 
	if (type&LEFT) type &= ~ZEROPAD;                                    // 48/ 
	if (base<2 || base>36)                                              // 49/ 
		return 0;                                                   // 50/ 
	c = (type & ZEROPAD) ? '0' : ' ' ;                                  // 51/ 
	if (type&SIGN && num<0) {                                           // 52/ 
		sign='-';                                                   // 53/ 
		num = -num;                                                 // 54/ 
	} else                                                              // 55/ 
		sign=(type&PLUS) ? '+' : ((type&SPACE) ? ' ' : 0);          // 56/ 
	if (sign) size--;                                                   // 57/ 
	if (type&SPECIAL)                                                   // 58/ 
		if (base==16) size -= 2;                                    // 59/ 
		else if (base==8) size--;                                   // 60/ 
	i=0;                                                                // 61/ 
	if (num==0)                                                         // 62/ 
		tmp[i++]='0';                                               // 63/ 
	else while (num!=0)                                                 // 64/ 
		tmp[i++]=digits[do_div(num,base)];                          // 65/ 
	if (i>precision) precision=i;                                       // 66/ 
	size -= precision;                                                  // 67/ 
	if (!(type&(ZEROPAD+LEFT)))                                         // 68/ 
		while(size-->0)                                             // 69/ 
			*str++ = ' ';                                       // 70/ 
	if (sign)                                                           // 71/ 
		*str++ = sign;                                              // 72/ 
	if (type&SPECIAL)                                                   // 73/ 
		if (base==8)                                                // 74/ 
			*str++ = '0';                                       // 75/ 
		else if (base==16) {                                        // 76/ 
			*str++ = '0';                                       // 77/ 
			*str++ = digits[33];                                // 78/ 
		}                                                           // 79/ 
	if (!(type&LEFT))                                                   // 80/ 
		while(size-->0)                                             // 81/ 
			*str++ = c;                                         // 82/ 
	while(i<precision--)                                                // 83/ 
		*str++ = '0';                                               // 84/ 
	while(i-->0)                                                        // 85/ 
		*str++ = tmp[i];                                            // 86/ 
	while(size-->0)                                                     // 87/ 
		*str++ = ' ';                                               // 88/ 
	return str;                                                         // 89/ 
}                                                                           // 90/ 
                                                                            // 91/ 
int vsprintf(char *buf, const char *fmt, va_list args)                      // 92/ [b;]送格式化输出到字符串中，可直接参考C库函数手册
{                                                                           // 93/ 
	int len;                                                            // 94/ 
	int i;                                                              // 95/ 
	char * str;                                                         // 96/ 
	char *s;                                                            // 97/ 
	int *ip;                                                            // 98/ 
                                                                            // 99/ 
	int flags;		/* flags to number() */                     //100/ 
                                                                            //101/ 
	int field_width;	/* width of output field */                 //102/ 
	int precision;		/* min. # of digits for integers; max       //103/ 
				   number of chars for from string */       //104/ 
	int qualifier;		/* 'h', 'l', or 'L' for integer fields */   //105/ 
                                                                            //106/ 
	for (str=buf ; *fmt ; ++fmt) {                                      //107/ 
		if (*fmt != '%') {                                          //108/ 
			*str++ = *fmt;                                      //109/ 
			continue;                                           //110/ 
		}                                                           //111/ 
			                                                    //112/ 
		/* process flags */                                         //113/ 
		flags = 0;                                                  //114/ 
		repeat:                                                     //115/ 
			++fmt;		/* this also skips first '%' */     //116/ 
			switch (*fmt) {                                     //117/ 
				case '-': flags |= LEFT; goto repeat;       //118/ 
				case '+': flags |= PLUS; goto repeat;       //119/ 
				case ' ': flags |= SPACE; goto repeat;      //120/ 
				case '#': flags |= SPECIAL; goto repeat;    //121/ 
				case '0': flags |= ZEROPAD; goto repeat;    //122/ 
				}                                           //123/ 
		                                                            //124/ 
		/* get field width */                                       //125/ 
		field_width = -1;                                           //126/ 
		if (is_digit(*fmt))                                         //127/ 
			field_width = skip_atoi(&fmt);                      //128/ 
		else if (*fmt == '*') {                                     //129/ 
			/* it's the next argument */                        //130/ 
			field_width = va_arg(args, int);                    //131/ 
			if (field_width < 0) {                              //132/ 
				field_width = -field_width;                 //133/ 
				flags |= LEFT;                              //134/ 
			}                                                   //135/ 
		}                                                           //136/ 
                                                                            //137/ 
		/* get the precision */                                     //138/ 
		precision = -1;                                             //139/ 
		if (*fmt == '.') {                                          //140/ 
			++fmt;	                                            //141/ 
			if (is_digit(*fmt))                                 //142/ 
				precision = skip_atoi(&fmt);                //143/ 
			else if (*fmt == '*') {                             //144/ 
				/* it's the next argument */                //145/ 
				precision = va_arg(args, int);              //146/ 
			}                                                   //147/ 
			if (precision < 0)                                  //148/ 
				precision = 0;                              //149/ 
		}                                                           //150/ 
                                                                            //151/ 
		/* get the conversion qualifier */                          //152/ 
		qualifier = -1;                                             //153/ 
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {            //154/ 
			qualifier = *fmt;                                   //155/ 
			++fmt;                                              //156/ 
		}                                                           //157/ 
                                                                            //158/ 
		switch (*fmt) {                                             //159/ 
		case 'c':                                                   //160/ 
			if (!(flags & LEFT))                                //161/ 
				while (--field_width > 0)                   //162/ 
					*str++ = ' ';                       //163/ 
			*str++ = (unsigned char) va_arg(args, int);         //164/ 
			while (--field_width > 0)                           //165/ 
				*str++ = ' ';                               //166/ 
			break;                                              //167/ 
                                                                            //168/ 
		case 's':                                                   //169/ 
			s = va_arg(args, char *);                           //170/ 
			len = strlen(s);                                    //171/ 
			if (precision < 0)                                  //172/ 
				precision = len;                            //173/ 
			else if (len > precision)                           //174/ 
				len = precision;                            //175/ 
                                                                            //176/ 
			if (!(flags & LEFT))                                //177/ 
				while (len < field_width--)                 //178/ 
					*str++ = ' ';                       //179/ 
			for (i = 0; i < len; ++i)                           //180/ 
				*str++ = *s++;                              //181/ 
			while (len < field_width--)                         //182/ 
				*str++ = ' ';                               //183/ 
			break;                                              //184/ 
                                                                            //185/ 
		case 'o':                                                   //186/ 
			str = number(str, va_arg(args, unsigned long), 8,   //187/ 
				field_width, precision, flags);             //188/ 
			break;                                              //189/ 
                                                                            //190/ 
		case 'p':                                                   //191/ 
			if (field_width == -1) {                            //192/ 
				field_width = 8;                            //193/ 
				flags |= ZEROPAD;                           //194/ 
			}                                                   //195/ 
			str = number(str,                                   //196/ 
				(unsigned long) va_arg(args, void *), 16,   //197/ 
				field_width, precision, flags);             //198/ 
			break;                                              //199/ 
                                                                            //200/ 
		case 'x':                                                   //201/ 
			flags |= SMALL;                                     //202/ 
		case 'X':                                                   //203/ 
			str = number(str, va_arg(args, unsigned long), 16,  //204/ 
				field_width, precision, flags);             //205/ 
			break;                                              //206/ 
                                                                            //207/ 
		case 'd':                                                   //208/ 
		case 'i':                                                   //209/ 
			flags |= SIGN;                                      //210/ 
		case 'u':                                                   //211/ 
			str = number(str, va_arg(args, unsigned long), 10,  //212/ 
				field_width, precision, flags);             //213/ 
			break;                                              //214/ 
                                                                            //215/ 
		case 'n':                                                   //216/ 
			ip = va_arg(args, int *);                           //217/ 
			*ip = (str - buf);                                  //218/ 
			break;                                              //219/ 
                                                                            //220/ 
		default:                                                    //221/ 
			if (*fmt != '%')                                    //222/ 
				*str++ = '%';                               //223/ 
			if (*fmt)                                           //224/ 
				*str++ = *fmt;                              //225/ 
			else                                                //226/ 
				--fmt;                                      //227/ 
			break;                                              //228/ 
		}                                                           //229/ 
	}                                                                   //230/ 
	*str = '\0';                                                        //231/ 
	return str-buf;                                                     //232/ 
}                                                                           //233/ 
