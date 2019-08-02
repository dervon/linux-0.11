/*                                                                 // 1/ 
 *  linux/fs/truncate.c                                            // 2/ 
 *                                                                 // 3/ 
 *  (C) 1991  Linus Torvalds                                       // 4/ 
 */                                                                // 5/ 
                                                                   // 6/ 
#include <linux/sched.h>                                           // 7/ 
                                                                   // 8/ 
#include <sys/stat.h>                                              // 9/ 
                                                                   //10/ 
static void free_ind(int dev,int block)                            //11/ [b;]用于释放设备号dev指定的设备上的一次间接逻辑块号block指定的一次间接块以及该一次间接块中的所有直接逻辑块号指定的直接块
{                                                                  //12/ 
	struct buffer_head * bh;                                   //13/ 
	unsigned short * p;                                        //14/ 
	int i;                                                     //15/ 
                                                                   //16/ 
	if (!block)                                                //17/ 判断一次间接逻辑块号block为0
		return;                                            //18/ 则返回
	if (bh=bread(dev,block)) {                                 //19/ 从设备号dev指定的设备上读取一次间接逻辑块号block指定的数据块到高速缓冲块中，返回指向该缓冲块对应缓冲头的指针赋给bh
		p = (unsigned short *) bh->b_data;                 //20/ 将bh指定的高速缓冲块的物理基地址赋给p
		for (i=0;i<512;i++,p++)                            //21/ 
			if (*p)                                    //22/ 
				free_block(dev,*p);                //23/ 释放一次间接块中的所有直接逻辑块号指定的直接块——利用HASH表在高速缓冲区中寻找设备号dev和逻辑块号*p指定的缓冲块，如果找到就将其释放(引用计数递减1)，唤醒buffer_wait指向的任务，如果引用计数递减1后为0，则复位该缓冲块的已修改标志和更新标志；复位逻辑块号*p在逻辑块位图中的比特位，置位相应逻辑块位图所在缓冲块已修改标志
		brelse(bh);                                        //24/ 等待不为空的bh指定的缓冲块解锁，将bh指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
	}                                                          //25/ 
	free_block(dev,block);                                     //26/ 释放设备上的一次间接块——利用HASH表在高速缓冲区中寻找设备号dev和一次间接逻辑块号block指定的缓冲块，如果找到就将其释放(引用计数递减1)，唤醒buffer_wait指向的任务，如果引用计数递减1后为0，则复位该缓冲块的已修改标志和更新标志；复位一次间接逻辑块号block在逻辑块位图中的比特位，置位相应逻辑块位图所在缓冲块已修改标志
}                                                                  //27/ 
                                                                   //28/ 
static void free_dind(int dev,int block)                           //29/ [b;]用于释放设备号dev指定的设备上的二次间接逻辑块号block指定的二次间接块，以及该二次间接块中的所有一次间接逻辑块号指定的一次间接块，以及这些一次间接块中的所有直接逻辑块号指定的直接块
{                                                                  //30/ 
	struct buffer_head * bh;                                   //31/ 
	unsigned short * p;                                        //32/ 
	int i;                                                     //33/ 
                                                                   //34/ 
	if (!block)                                                //35/ 判断二次间接逻辑块号block为0
		return;                                            //36/ 则返回
	if (bh=bread(dev,block)) {                                 //37/ 从设备号dev指定的设备上读取二次间接逻辑块号block指定的数据块到高速缓冲块中，返回指向该缓冲块对应缓冲头的指针赋给bh
		p = (unsigned short *) bh->b_data;                 //38/ 将bh指定的高速缓冲块的物理基地址赋给p
		for (i=0;i<512;i++,p++)                            //39/ 
			if (*p)                                    //40/ 
				free_ind(dev,*p);                  //41/ 释放二次间接块中的所有一次间接逻辑块号指定的一次间接块——用于释放设备号dev指定的设备上的一次间接逻辑块号*p指定的一次间接块以及该一次间接块中的所有直接块号指定的直接块
		brelse(bh);                                        //42/ 等待不为空的bh指定的缓冲块解锁，将bh指定的缓冲块释放(引用计数递减1)，唤醒buffer_wait指向的任务
	}                                                          //43/ 
	free_block(dev,block);                                     //44/ 释放设备上的二次间接块——利用HASH表在高速缓冲区中寻找设备号dev和二次间接逻辑块号block指定的缓冲块，如果找到就将其释放(引用计数递减1)，唤醒buffer_wait指向的任务，如果引用计数递减1后为0，则复位该缓冲块的已修改标志和更新标志；复位二次间接逻辑块号block在逻辑块位图中的比特位，置位相应逻辑块位图所在缓冲块已修改标志
}                                                                  //45/ 
                                                                   //46/ 
void truncate(struct m_inode * inode)                              //47/ [b;]将inode指向的内存中i节点对应的文件所占用的所有逻辑块(包括所有的直接块、一次和二次间接块)都释放，将该文件的长度置零，置位该i节点的已修改标志，将该i节点的文件修改时间和i节点改变时间设置为当前时间
{                                                                  //48/ 
	int i;                                                     //49/ 
                                                                   //50/ 
	if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode)))   //51/ 如果inode指向的内存中i节点不是常规文件或目录文件的i节点
		return;                                            //52/ 则返回
	for (i=0;i<7;i++)                                          //53/ 遍寻inode指向的内存中i节点的7个直接逻辑块号(i_zone[0]-i_zone[6])
		if (inode->i_zone[i]) {                            //54/ 
			free_block(inode->i_dev,inode->i_zone[i]); //55/ 利用HASH表在高速缓冲区中寻找设备号(inode->i_dev)和直接逻辑块号(inode->i_zone[i])指定的缓冲块，如果找到就将其释放(引用计数递减1)，唤醒buffer_wait指向的任务，如果引用计数递减1后为0，则复位该缓冲块的已修改标志和更新标志；复位直接逻辑块号(inode->i_zone[i])在逻辑块位图中的比特位，置位相应逻辑块位图所在缓冲块已修改标志
			inode->i_zone[i]=0;                        //56/ 将直接块号inode->i_zone[i]清0
		}                                                  //57/ 
	free_ind(inode->i_dev,inode->i_zone[7]);                   //58/ 用于释放设备号(inode->i_dev)指定的设备上的一次间接块号(inode->i_zone[7])指定的一次间接块以及该一次间接块中的所有直接块号指定的直接块
	free_dind(inode->i_dev,inode->i_zone[8]);                  //59/ 用于释放设备号(inode->i_dev)指定的设备上的二次间接逻辑块号(inode->i_zone[8])指定的二次间接块，以及该二次间接块中的所有一次间接逻辑块号指定的一次间接块，以及这些一次间接块中的所有直接逻辑块号指定的直接块
	inode->i_zone[7] = inode->i_zone[8] = 0;                   //60/ 将一次间接块号inode->i_zone[7]和二次间接块号inode->i_zone[8]清0
	inode->i_size = 0;                                         //61/ 将inode指向的内存中i节点对应的文件的长度置零
	inode->i_dirt = 1;                                         //62/ 置位inode指向的内存中i节点的已修改标志
	inode->i_mtime = inode->i_ctime = CURRENT_TIME;            //63/ 将inode指向的内存中i节点的文件修改时间和i节点改变时间设置为当前时间
}                                                                  //64/ 
                                                                   //65/ 
