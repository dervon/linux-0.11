#ifndef _STDARG_H                                                         // 1/ 
#define _STDARG_H                                                         // 2/ 
                                                                          // 3/ 
typedef char *va_list;                                                    // 4/ 
                                                                          // 5/ 
/* Amount of space required in an argument list for an arg of type TYPE.  // 6/ 
   TYPE may alternatively be an expression whose type is used.  */        // 7/ 
                                                                          // 8/ 
#define __va_rounded_size(TYPE)  \                                        // 9/ 定义了取整后的TYPE类型的字节长度值，即TYPE类型(占3B->4B;占4B->4B;占5B->8B)
  (((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))    //10/ 
                                                                          //11/ 
#ifndef __sparc__                                                         //12/ 
#define va_start(AP, LASTARG) 						\ //13/ 
 (AP = ((char *) &(LASTARG) + __va_rounded_size (LASTARG)))               //14/ 使指针AP指向传给函数的可变参数表的第一个参数，比如指向printk(“abc%d”,a)中的参数a
#else                                                                     //15/ 
#define va_start(AP, LASTARG) 						\ //16/ 
 (__builtin_saveregs (),						\ //17/ 在libgcc2.c中定义，用于保存寄存器
  AP = ((char *) &(LASTARG) + __va_rounded_size (LASTARG)))               //18/ 
#endif                                                                    //19/ 
                                                                          //20/ 
void va_end (va_list);		/* Defined in gnulib */                   //21/ 在gnulib中定义，可以修改AP使其在重新调用va_start之前不能被使用。va_end必须在va_arg读完所有的参数后再被调用(VA_END(),X86平台定义为ap = ((char*)0)，使ap不再指向堆栈，而是跟NULL一样，有些直接定义为((void*)0)，这样编译器不会为VA_END产生代码，例如gcc在Linux的X86平台就是这样定义的。)
#define va_end(AP)                                                        //22/ 
                                                                          //23/ 
#define va_arg(AP, TYPE)						\ //24/ 将当前AP指向的参数返回，再修改AP，使其指向可变参数表的下一个参数
 (AP += __va_rounded_size (TYPE),					\ //25/ 
  *((TYPE *) (AP - __va_rounded_size (TYPE))))                            //26/ 
                                                                          //27/ 
#endif /* _STDARG_H */                                                    //28/ 
