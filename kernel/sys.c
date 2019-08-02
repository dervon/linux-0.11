/*                                                                               //  1/ 
 *  linux/kernel/sys.c                                                           //  2/ 
 *                                                                               //  3/ 
 *  (C) 1991  Linus Torvalds                                                     //  4/ 
 */                                                                              //  5/ 
                                                                                 //  6/ 
#include <errno.h>                                                               //  7/ 
                                                                                 //  8/ 
#include <linux/sched.h>                                                         //  9/ 
#include <linux/tty.h>                                                           // 10/ 
#include <linux/kernel.h>                                                        // 11/ 
#include <asm/segment.h>                                                         // 12/ 
#include <sys/times.h>                                                           // 13/ 
#include <sys/utsname.h>                                                         // 14/ 
                                                                                 // 15/ 
int sys_ftime()                                                                  // 16/ [b;]返回日期和时间(此功能还没有实现)
{                                                                                // 17/ 
	return -ENOSYS;                                                          // 18/ 
}                                                                                // 19/ 
                                                                                 // 20/ 
int sys_break()                                                                  // 21/ [b;](此功能还没有实现)
{                                                                                // 22/ 
	return -ENOSYS;                                                          // 23/ 
}                                                                                // 24/ 
                                                                                 // 25/ 
int sys_ptrace()                                                                 // 26/ [b;]用于当前进程对子进程进行调试(此功能还没有实现)
{                                                                                // 27/ 
	return -ENOSYS;                                                          // 28/ 
}                                                                                // 29/ 
                                                                                 // 30/ 
int sys_stty()                                                                   // 31/ [b;]改变并打印终端行设置(此功能还没有实现)
{                                                                                // 32/ 
	return -ENOSYS;                                                          // 33/ 
}                                                                                // 34/ 
                                                                                 // 35/ 
int sys_gtty()                                                                   // 36/ [b;]取终端行设置信息(此功能还没有实现)
{                                                                                // 37/ 
	return -ENOSYS;                                                          // 38/ 
}                                                                                // 39/ 
                                                                                 // 40/ 
int sys_rename()                                                                 // 41/ [b;]修改文件名(此功能还没有实现)
{                                                                                // 42/ 
	return -ENOSYS;                                                          // 43/ 
}                                                                                // 44/ 
                                                                                 // 45/ 
int sys_prof()                                                                   // 46/ [b;](此功能还没有实现)
{                                                                                // 47/ 
	return -ENOSYS;                                                          // 48/ 
}                                                                                // 49/ 
                                                                                 // 50/ 
int sys_setregid(int rgid, int egid)                                             // 51/ [b;]设置当前任务的实际组ID(设为参数一)和有效组ID(设为参数二)
{                                                                                // 52/ 
	if (rgid>0) {                                                            // 53/ 当 参数指定的实际组ID > 0 时
		if ((current->gid == rgid) ||                                    // 54/ 如果当前任务的实际组ID与参数指定的实际组ID相同，
		    suser())                                                     // 55/ 或者当前任务具有超级用户权限
			current->gid = rgid;                                     // 56/ 则设置当前任务的实际组ID等于参数指定的实际组ID值
		else                                                             // 57/ 
			return(-EPERM);                                          // 58/ 否则返回错误码(操作未许可)
	}                                                                        // 59/ 
	if (egid>0) {                                                            // 60/ 当 参数指定的有效组ID > 0 时
		if ((current->gid == egid) ||                                    // 61/ 如果当前任务的实际组ID与参数指定的有效组ID相同，
		    (current->egid == egid) ||                                   // 62/ 或者当前任务的有效组ID与参数指定的有效组ID相同，
		    (current->sgid == egid) ||                                   // 63/ 或者当前任务的保存的组ID与参数指定的有效组ID相同，
		    suser())                                                     // 64/ 或者当前任务具有超级用户权限
			current->egid = egid;                                    // 65/ 则设置当前任务的有效组ID等于参数指定的有效组ID值
		else                                                             // 66/ 
			return(-EPERM);                                          // 67/ 否则返回错误码(操作未许可)
	}                                                                        // 68/ 
	return 0;                                                                // 69/ 
}                                                                                // 70/ 
                                                                                 // 71/ 
int sys_setgid(int gid)                                                          // 72/ [b;]设置当前任务的实际组ID和有效组ID都为参数指定的ID
{                                                                                // 73/ 
	return(sys_setregid(gid, gid));                                          // 74/ 
}                                                                                // 75/ 
                                                                                 // 76/ 
int sys_acct()                                                                   // 77/ [b;]打开或关闭进程记账功能(此功能还没有实现)
{                                                                                // 78/ 
	return -ENOSYS;                                                          // 79/ 
}                                                                                // 80/ 
                                                                                 // 81/ 
int sys_phys()                                                                   // 82/ [b;]映射任意物理内存到进程的虚拟地址空间(此功能还没有实现)
{                                                                                // 83/ 
	return -ENOSYS;                                                          // 84/ 
}                                                                                // 85/ 
                                                                                 // 86/ 
int sys_lock()                                                                   // 87/ [b;](此功能还没有实现)
{                                                                                // 88/ 
	return -ENOSYS;                                                          // 89/ 
}                                                                                // 90/ 
                                                                                 // 91/ 
int sys_mpx()                                                                    // 92/ [b;](此功能还没有实现)
{                                                                                // 93/ 
	return -ENOSYS;                                                          // 94/ 
}                                                                                // 95/ 
                                                                                 // 96/ 
int sys_ulimit()                                                                 // 97/ [b;](此功能还没有实现)
{                                                                                // 98/ 
	return -ENOSYS;                                                          // 99/ 
}                                                                                //100/ 
                                                                                 //101/ 
int sys_time(long * tloc)                                                        //102/ [b;]返回当前时间，即从1970.1.1:0:0:0开始到此时此刻经过的秒数，如果参数tloc不为null，当前时间还会被存储在fs段中tloc指定的内存地址处
{                                                                                //103/ 
	int i;                                                                   //104/ 
                                                                                 //105/ 
	i = CURRENT_TIME;                                                        //106/ 
	if (tloc) {                                                              //107/ 
		verify_area(tloc,4);                                             //108/ 对当前任务的逻辑地址中从tloc到tloc+4这一段范围以页为单位执行写操作前的检测操作，若页不可写则执行共享检验和复制页面操作
		put_fs_long(i,(unsigned long *)tloc);                            //109/ 将当前时间存放在fs段(fs此时指向任务自身的数据段/栈段)中tloc指定的内存地址处
	}                                                                        //110/ 
	return i;                                                                //111/ 
}                                                                                //112/ 
                                                                                 //113/ 
/*                                                                               //114/ 
 * Unprivileged users may change the real user id to the effective uid           //115/ 
 * or vice versa.                                                                //116/ 
 */                                                                              //117/ 
int sys_setreuid(int ruid, int euid)                                             //118/ [b;]设置当前任务的实际用户ID(设为参数一)和有效用户ID(设为参数二)
{                                                                                //119/ 
	int old_ruid = current->uid;                                             //120/ 
	                                                                         //121/ 
	if (ruid>0) {                                                            //122/ 当 参数指定的实际用户ID > 0 时
		if ((current->euid==ruid) ||                                     //123/ 如果当前任务的有效用户ID与参数指定的实际用户ID相同，
                    (old_ruid == ruid) ||                                        //124/ 或者当前任务的实际用户ID与参数指定的实际用户ID相同，
		    suser())                                                     //125/ 或者当前任务具有超级用户权限
			current->uid = ruid;                                     //126/ 则设置当前任务的实际用户ID等于参数指定的实际用户ID值
		else                                                             //127/ 
			return(-EPERM);                                          //128/ 否则返回错误码(操作未许可)
	}                                                                        //129/ 
	if (euid>0) {                                                            //130/ 当 参数指定的有效用户ID > 0 时
		if ((old_ruid == euid) ||                                        //131/ 如果当前任务的实际用户ID与参数指定的有效用户ID相同，
                    (current->euid == euid) ||                                   //132/ 或者当前任务的有效用户ID与参数指定的有效用户ID相同，
		    suser())                                                     //133/ 或者当前任务具有超级用户权限
			current->euid = euid;                                    //134/ 则设置当前任务的有效用户ID等于参数指定的有效用户ID值
		else {                                                           //135/ 
			current->uid = old_ruid;                                 //136/ 否则不作改变，返回错误码(操作未许可)
			return(-EPERM);                                          //137/ 
		}                                                                //138/ 
	}                                                                        //139/ 
	return 0;                                                                //140/ 
}                                                                                //141/ 
                                                                                 //142/ 
int sys_setuid(int uid)                                                          //143/ [b;]设置当前任务的实际用户ID和有效用户ID都为参数指定的ID
{                                                                                //144/ 
	return(sys_setreuid(uid, uid));                                          //145/ 
}                                                                                //146/ 
                                                                                 //147/ 
int sys_stime(long * tptr)                                                       //148/ [b;]设置系统开机时间
{                                                                                //149/ 
	if (!suser())                                                            //150/ 保证必须是超级用户
		return -EPERM;                                                   //151/ 
	startup_time = get_fs_long((unsigned long *)tptr) - jiffies/HZ;          //152/ 将fs段(fs此时指向任务自身的数据段/栈段)中tptr指定的内存地址处的当前时间取出来，减去系统已经运行的时间秒值，得到开机时间用于覆盖startup_time
	return 0;                                                                //153/ 
}                                                                                //154/ 
                                                                                 //155/ 
int sys_times(struct tms * tbuf)                                                 //156/ [b;]获取当前任务运行时间统计值存入参数tbuf指定的结构中，并返回系统已经运行的时间滴答值
{                                                                                //157/ 
	if (tbuf) {                                                              //158/ 
		verify_area(tbuf,sizeof *tbuf);                                  //159/ 对当前任务的逻辑地址中从tbuf到tbuf+(sizeof *tbuf)这一段范围以页为单位执行写操作前的检测操作，若页不可写则执行共享检验和复制页面操作
		put_fs_long(current->utime,(unsigned long *)&tbuf->tms_utime);   //160/ 将当前进程在用户态运行的时间(滴答数)存放在fs段(fs此时指向任务自身的数据段/栈段)中&tbuf->tms_utime指定的内存地址处
		put_fs_long(current->stime,(unsigned long *)&tbuf->tms_stime);   //161/ 将当前进程在内核态运行的时间(滴答数)存放在fs段(fs此时指向任务自身的数据段/栈段)中&tbuf->tms_stime指定的内存地址处
		put_fs_long(current->cutime,(unsigned long *)&tbuf->tms_cutime); //162/ 将当前进程的已终止子进程用户态运行的时间总和(滴答数)存放在fs段(fs此时指向任务自身的数据段/栈段)中&tbuf->tms_cutime指定的内存地址处
		put_fs_long(current->cstime,(unsigned long *)&tbuf->tms_cstime); //163/ 将当前进程的已终止子进程内核态运行的时间总和(滴答数)存放在fs段(fs此时指向任务自身的数据段/栈段)中&tbuf->tms_cstime指定的内存地址处
	}                                                                        //164/ 
	return jiffies;                                                          //165/ 
}                                                                                //166/ 
                                                                                 //167/ 
int sys_brk(unsigned long end_data_seg)                                          //168/ [b;]设置当前任务数据结构中brk(即代码+数据+bss字节长度值)的值为end_data_seg，返回brk的值
{                                                                                //169/ 
	if (end_data_seg >= current->end_code &&                                 //170/ 如果参数值大于等于代码段结尾，并且小于(堆栈-16KB)，
	    end_data_seg < current->start_stack - 16384)                         //171/ 
		current->brk = end_data_seg;                                     //172/ 则设置当前任务数据结构中brk为end_data_seg
	return current->brk;                                                     //173/ 
}                                                                                //174/ 
                                                                                 //175/ 
/*                                                                               //176/ 
 * This needs some heave checking ...                                            //177/ 
 * I just haven't get the stomach for it. I also don't fully                     //178/ 
 * understand sessions/pgrp etc. Let somebody who does explain it.               //179/ 
 */                                                                              //180/ 
int sys_setpgid(int pid, int pgid)                                               //181/ [b;]设置进程号为参数pid的进程的进程组号为参数pgid
{                                                                                //182/ 
	int i;                                                                   //183/ 
                                                                                 //184/ 
	if (!pid)                                                                //185/ 如果参数pid==0
		pid = current->pid;                                              //186/ 则将当前进程的进程号赋给参数pid
	if (!pgid)                                                               //187/ 如果参数pgid==0
		pgid = current->pid;                                             //188/ 则将当前进程的进程号赋给参数pgid
	for (i=0 ; i<NR_TASKS ; i++)                                             //189/ 遍寻64(0-63)个任务槽
		if (task[i] && task[i]->pid==pid) {                              //190/ 如果由某个任务的进程号等于参数pid
			if (task[i]->leader)                                     //191/ 如果该进程是会话首进程，则返回错误码(操作未许可)
				return -EPERM;                                   //192/ 
			if (task[i]->session != current->session)                //193/ 如果该进程的会话号与当前进程的会话号不同，则返回错误码(操作未许可)
				return -EPERM;                                   //194/ 
			task[i]->pgrp = pgid;                                    //195/ 将该进程的进程组号置为参数pgid
			return 0;                                                //196/ 
		}                                                                //197/ 
	return -ESRCH;                                                           //198/ 如果没找到某个任务的进程号等于参数pid，则返回错误码(操作未许可)
}                                                                                //199/ 
                                                                                 //200/ 
int sys_getpgrp(void)                                                            //201/ [b;]获取当前任务的进程组号
{                                                                                //202/ 
	return current->pgrp;                                                    //203/ 
}                                                                                //204/ 
                                                                                 //205/ 
int sys_setsid(void)                                                             //206/ [b;]创建一个会话，设置其会话号=其组号=其进程号，并返回会话号
{                                                                                //207/ 
	if (current->leader && !suser())                                         //208/ 如果当前进程是会话首进程并且不是超级用户，则返回错误码(操作未许可)
		return -EPERM;                                                   //209/ 
	current->leader = 1;                                                     //210/ 设置当前进程为新会话首进程，并设置当前进程会话号和进程组号都等于其进程号
	current->session = current->pgrp = current->pid;                         //211/ 
	current->tty = -1;                                                       //212/ 设置当前进程没有控制终端
	return current->pgrp;                                                    //213/ 返回会话号
}                                                                                //214/ 
                                                                                 //215/ 
int sys_uname(struct utsname * name)                                             //216/ [b;]获取系统名称等信息放入用户数据栈中name指定的内存地址处
{                                                                                //217/ 
	static struct utsname thisname = {                                       //218/ 
		"linux .0","nodename","release ","version ","machine "           //219/ 当前运行系统的名称、与实现相关的网络中节点名称(主机名称)、本操作系统实现的当前发行级别、本次发行的操作系统版本级别、系统运行的硬件类型名称
	};                                                                       //220/ 
	int i;                                                                   //221/ 
                                                                                 //222/ 
	if (!name) return -ERROR;                                                //223/ 如果参数name为空，则返回出错码(一般错误)
	verify_area(name,sizeof *name);                                          //224/ 对当前任务的逻辑地址中从name到name+sizeof *name这一段范围以页为单位执行写操作前的检测操作，若页不可写则执行共享检验和复制页面操作
	for(i=0;i<sizeof *name;i++)                                              //225/ 将thisname中定义的内容都存放在fs段(fs此时指向任务自身的数据段/栈段)中name指定的内存地址处
		put_fs_byte(((char *) &thisname)[i],i+(char *) name);            //226/ 
	return 0;                                                                //227/ 
}                                                                                //228/ 
                                                                                 //229/ 
int sys_umask(int mask)                                                          //230/ [b;]设置当前进程的创建文件属性屏蔽码umask为(参数mask & 0777)，并返回原屏蔽码
{                                                                                //231/ 
	int old = current->umask;                                                //232/ 
                                                                                 //233/ 
	current->umask = mask & 0777;                                            //234/ 
	return (old);                                                            //235/ 
}                                                                                //236/ 
