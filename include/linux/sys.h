extern int sys_setup();                                                 // 1/ [b;]1-72行列出所有的系统调用名，给下面的系统调用表sys_call_table使用
extern int sys_exit();                                                  // 2/ 这些名称都对应到一个汇编地址，估计是对应的int 0x80中对应的中断处理程序的汇编首地址
extern int sys_fork();                                                  // 3/ 
extern int sys_read();                                                  // 4/ 
extern int sys_write();                                                 // 5/ 
extern int sys_open();                                                  // 6/ 
extern int sys_close();                                                 // 7/ 
extern int sys_waitpid();                                               // 8/ 
extern int sys_creat();                                                 // 9/ 
extern int sys_link();                                                  //10/ 
extern int sys_unlink();                                                //11/ 
extern int sys_execve();                                                //12/ 
extern int sys_chdir();                                                 //13/ 
extern int sys_time();                                                  //14/ 
extern int sys_mknod();                                                 //15/ 
extern int sys_chmod();                                                 //16/ 
extern int sys_chown();                                                 //17/ 
extern int sys_break();                                                 //18/ 
extern int sys_stat();                                                  //19/ 
extern int sys_lseek();                                                 //20/ 
extern int sys_getpid();                                                //21/ 
extern int sys_mount();                                                 //22/ 
extern int sys_umount();                                                //23/ 
extern int sys_setuid();                                                //24/ 
extern int sys_getuid();                                                //25/ 
extern int sys_stime();                                                 //26/ 
extern int sys_ptrace();                                                //27/ 
extern int sys_alarm();                                                 //28/ 
extern int sys_fstat();                                                 //29/ 
extern int sys_pause();                                                 //30/ 
extern int sys_utime();                                                 //31/ 
extern int sys_stty();                                                  //32/ 
extern int sys_gtty();                                                  //33/ 
extern int sys_access();                                                //34/ 
extern int sys_nice();                                                  //35/ 
extern int sys_ftime();                                                 //36/ 
extern int sys_sync();                                                  //37/ 
extern int sys_kill();                                                  //38/ 
extern int sys_rename();                                                //39/ 
extern int sys_mkdir();                                                 //40/ 
extern int sys_rmdir();                                                 //41/ 
extern int sys_dup();                                                   //42/ 
extern int sys_pipe();                                                  //43/ 
extern int sys_times();                                                 //44/ 
extern int sys_prof();                                                  //45/ 
extern int sys_brk();                                                   //46/ 
extern int sys_setgid();                                                //47/ 
extern int sys_getgid();                                                //48/ 
extern int sys_signal();                                                //49/ 
extern int sys_geteuid();                                               //50/ 
extern int sys_getegid();                                               //51/ 
extern int sys_acct();                                                  //52/ 
extern int sys_phys();                                                  //53/ 
extern int sys_lock();                                                  //54/ 
extern int sys_ioctl();                                                 //55/ 
extern int sys_fcntl();                                                 //56/ 
extern int sys_mpx();                                                   //57/ 
extern int sys_setpgid();                                               //58/ 
extern int sys_ulimit();                                                //59/ 
extern int sys_uname();                                                 //60/ 
extern int sys_umask();                                                 //61/ 
extern int sys_chroot();                                                //62/ 
extern int sys_ustat();                                                 //63/ 
extern int sys_dup2();                                                  //64/ 
extern int sys_getppid();                                               //65/ 
extern int sys_getpgrp();                                               //66/ 
extern int sys_setsid();                                                //67/ 
extern int sys_sigaction();                                             //68/ 
extern int sys_sgetmask();                                              //69/ 
extern int sys_ssetmask();                                              //70/ 
extern int sys_setreuid();                                              //71/ 
extern int sys_setregid();                                              //72/ 
                                                                        //73/ [b;]74-86行是系统调用函数指针表，用于int 0x80，作为跳转表，这些名称都对应到一个汇编地址，
fn_ptr sys_call_table[] = { sys_setup, sys_exit, sys_fork, sys_read,    //74/ [b;]估计是要填入到对应的int 0x80中对应的中断处理程序中的汇编首地址
sys_write, sys_open, sys_close, sys_waitpid, sys_creat, sys_link,       //75/ fn_ptr定义在include/linux/sched.h中——typedef int (*fn_ptr)();
sys_unlink, sys_execve, sys_chdir, sys_time, sys_mknod, sys_chmod,      //76/ 
sys_chown, sys_break, sys_stat, sys_lseek, sys_getpid, sys_mount,       //77/ 
sys_umount, sys_setuid, sys_getuid, sys_stime, sys_ptrace, sys_alarm,   //78/ 
sys_fstat, sys_pause, sys_utime, sys_stty, sys_gtty, sys_access,        //79/ 
sys_nice, sys_ftime, sys_sync, sys_kill, sys_rename, sys_mkdir,         //80/ 
sys_rmdir, sys_dup, sys_pipe, sys_times, sys_prof, sys_brk, sys_setgid, //81/ 
sys_getgid, sys_signal, sys_geteuid, sys_getegid, sys_acct, sys_phys,   //82/ 
sys_lock, sys_ioctl, sys_fcntl, sys_mpx, sys_setpgid, sys_ulimit,       //83/ 
sys_uname, sys_umask, sys_chroot, sys_ustat, sys_dup2, sys_getppid,     //84/ 
sys_getpgrp, sys_setsid, sys_sigaction, sys_sgetmask, sys_ssetmask,     //85/ 
sys_setreuid,sys_setregid };                                            //86/ 
