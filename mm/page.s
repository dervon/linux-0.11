/*                                                    ## 1# 
 *  linux/mm/page.s                                   ## 2# 
 *                                                    ## 3# 
 *  (C) 1991  Linus Torvalds                          ## 4# 
 */                                                   ## 5# 
                                                      ## 6# 
/*                                                    ## 7# 
 * page.s contains the low-level page-exception code. ## 8# 
 * the real work is done in mm.c                      ## 9# 
 */                                                   ##10# 
                                                      ##11# 
.globl _page_fault                                    ##12# 
                                                      ##13# 
_page_fault:                                          ##14# 
	xchgl %eax,(%esp)                             ##15# 
	pushl %ecx                                    ##16# 
	pushl %edx                                    ##17# 
	push %ds                                      ##18# 
	push %es                                      ##19# 
	push %fs                                      ##20# 
	movl $0x10,%edx                               ##21# 
	mov %dx,%ds                                   ##22# 
	mov %dx,%es                                   ##23# 
	mov %dx,%fs                                   ##24# 
	movl %cr2,%edx                                ##25# 
	pushl %edx                                    ##26# 
	pushl %eax                                    ##27# 
	testl $1,%eax                                 ##28# 
	jne 1f                                        ##29# 
	call _do_no_page                              ##30# 
	jmp 2f                                        ##31# 
1:	call _do_wp_page                              ##32# 
2:	addl $8,%esp                                  ##33# 
	pop %fs                                       ##34# 
	pop %es                                       ##35# 
	pop %ds                                       ##36# 
	popl %edx                                     ##37# 
	popl %ecx                                     ##38# 
	popl %eax                                     ##39# 
	iret                                          ##40# 
