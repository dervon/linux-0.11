#ifndef _CTYPE_H                                                  // 1/ 
#define _CTYPE_H                                                  // 2/ 
                                                                  // 3/ 
#define _U	0x01	/* upper */                               // 4/ 该比特位用于大写字符A-Z  [b;]4-11行——(_C:控制字符;  _S:空白字符，如空格、\t、\n;  _P:标点字符;  _SP:空格字符;  _D:字符0-9;  _X:十六机制数字;  _U:大写字符A-Z;  _L:小写字符a-z;)
#define _L	0x02	/* lower */                               // 5/ 该比特位用于小写字符a-z
#define _D	0x04	/* digit */                               // 6/ 该比特位用于字符0-9
#define _C	0x08	/* cntrl */                               // 7/ 该比特位用于控制字符
#define _P	0x10	/* punct */                               // 8/ 该比特位用于标点字符
#define _S	0x20	/* white space (space/lf/tab) */          // 9/ 该比特位用于空白字符，如空格、\t、\n
#define _X	0x40	/* hex digit */                           //10/ 该比特位用于十六进制数字
#define _SP	0x80	/* hard space (0x20) */                   //11/ 该比特位用于空格字符
                                                                  //12/ 
extern unsigned char _ctype[];                                    //13/ 
extern char _ctmp;                                                //14/ 
                                                                  //15/ 
#define isalnum(c) ((_ctype+1)[c]&(_U|_L|_D))                     //16/ 判断ASCII码值为c的字符是否是 大写字符A-Z 或 小写字符a-z 或 字符0-9
#define isalpha(c) ((_ctype+1)[c]&(_U|_L))                        //17/ 判断ASCII码值为c的字符是否是 大写字符A-Z 或 小写字符a-z
#define iscntrl(c) ((_ctype+1)[c]&(_C))                           //18/ 判断ASCII码值为c的字符是否是 控制字符
#define isdigit(c) ((_ctype+1)[c]&(_D))                           //19/ 判断ASCII码值为c的字符是否是 字符0-9
#define isgraph(c) ((_ctype+1)[c]&(_P|_U|_L|_D))                  //20/ 判断ASCII码值为c的字符是否是 标点字符 或 大写字符A-Z 或 小写字符a-z 或 字符0-9
#define islower(c) ((_ctype+1)[c]&(_L))                           //21/ 判断ASCII码值为c的字符是否是 小写字符a-z
#define isprint(c) ((_ctype+1)[c]&(_P|_U|_L|_D|_SP))              //22/ 判断ASCII码值为c的字符是否是 标点字符 或 大写字符A-Z 或 小写字符a-z 或 字符0-9 或 空格字符
#define ispunct(c) ((_ctype+1)[c]&(_P))                           //23/ 判断ASCII码值为c的字符是否是 标点字符
#define isspace(c) ((_ctype+1)[c]&(_S))                           //24/ 判断ASCII码值为c的字符是否是 空白字符，如空格、\t、\n
#define isupper(c) ((_ctype+1)[c]&(_U))                           //25/ 判断ASCII码值为c的字符是否是 大写字符A-Z
#define isxdigit(c) ((_ctype+1)[c]&(_D|_X))                       //26/ 判断ASCII码值为c的字符是否是 字符0-9 或 十六进制数字
                                                                  //27/ 
#define isascii(c) (((unsigned) c)<=0x7f)                         //28/ 判断字符c的ASCII码是否小于等于127，若是则返回1，否则返回0
#define toascii(c) (((unsigned) c)&0x7f)                          //29/ 取出c的低7位对应的ASCII值
                                                                  //30/ 
#define tolower(c) (_ctmp=c,isupper(_ctmp)?_ctmp-('A'-'a'):_ctmp) //31/ 判断ASCII码值为c的字符是否是大写字符，如果是则转换为小写字符
#define toupper(c) (_ctmp=c,islower(_ctmp)?_ctmp-('a'-'A'):_ctmp) //32/ 判断ASCII码值为c的字符是否是小写字符，如果是则转换为大写字符
                                                                  //33/ 
#endif                                                            //34/ 
