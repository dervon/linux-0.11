
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<math.h>

#define	BUFFSIZE	2000
#define	BUFFSIZE1	2000
#define	BUFFSIZE2	2000

#define BUFFSIZE3	2000



	int	i,m,n,j,k,t;
	FILE    *fp,*fp0,*fp1,*fp2;
	unsigned char	buf[BUFFSIZE];
	char	buf1[BUFFSIZE1];
	char	buf2[BUFFSIZE2];
	int shu;//argv[2]
	int IsAddHang;//argv[3]
	unsigned int ascii;
	long hangshu = 0;
	long bigZifushu;

int main(int argc,char *argv[])		//argv[0]-可执行文件名    argv[1]-目标文件名    argv[2]-!号的位置在哪一列   argv[3]-0：不加行数 1：加行数
{
	if (argc < 2)
	{
		printf("出错：可执行文件名 目标文件名 !号的位置在哪一列(默认靠近代码，如果太小可以再加) 是1否0加行数(默认加行数)\n");
		exit(0);
	}

	//计算出目标文件总的行数
	fp0=fopen(argv[1],"r");
	while(1)
	{
		if(fgets(buf,BUFFSIZE2,fp0) == NULL)
		{
			break;
		}
		else
		{
			int all = 0,add = 0;
			for(i=0;i<strlen(buf);i++)
			{
				if(buf[i] > 128)
				{
					printf("禁止处理：检测到汉字，可能已注释过\n");
					return;
				}
				if(buf[i] == '\t')
				{
					all = all + (8 - (i+add)%8);
					add = 7 - i%8;
				}
				else
					all++;
			}	
			bigZifushu = bigZifushu > all ? bigZifushu : all;
			hangshu++;
		}
	}
	fclose(fp0);
	
	//先将目标文件的内容复制到临时文件tmp
	fp=fopen("tmp","w");
	fp0=fopen(argv[1],"r");
	for(j=0;j<hangshu;j++)
	{
		bzero(buf2,2000);
		fgets(buf2,BUFFSIZE2,fp0);
		fputs(buf2,fp);
	}
	fclose(fp);
	fclose(fp0);
	
	//用tmp文件的内容作为原始文件，将新内容写入目标文件
	fp2=fopen("tmp","r");
	fp1=fopen(argv[1],"w");
	
	for(j=0;j<hangshu;j++)
	{
		bzero(buf2,2000);
		fgets(buf2,BUFFSIZE2,fp2);	//读取一行
		
		if (argc >= 3)
		{
			shu = atoi(argv[2]);	//在第shu列输入一个!
		}
		else
			shu = bigZifushu + 0;	//默认在靠近代码的位置输入一个!
		
		t = 0;
		
		//找出每行的行尾
		for(n=0;n<BUFFSIZE2;n++)
		{
			if(buf2[n] == '\n')
			{
				break;
			}
		}

		//算出在有tab键的情况下应该少加多少个空格
		for(k=0;k<n;k++)
		{
			if(buf2[k] == '\t')
			{	
				shu = shu - (7 - ((k+1+t)-1)%8) ;
				t = t + (7 - ((k+1+t)-1)%8);
			}
		}
		
		//检查\n之前的字符，是\r回车换行就将其变成空格
		if(buf2[n-1] == '\r')
			buf2[n-1] = ' ';
		
		//加空格	
		for(m=n;m<shu;m++)
		{
			buf2[m] = ' ';
		}
		
		//加行数
		if (argc >= 4)
		{
			IsAddHang = atoi(argv[3]);	
		}
		else
			IsAddHang = 1;
		if(IsAddHang == 1)	//是否将行数写进注释
		{
			buf2[shu] = '!';
			shu++;
			
			
			if(hangshu < 10)
			{
				buf2[shu] = ((j+1)%10 + '0');//个位
				shu++;
			}
			else if(hangshu < 100)
			{
				buf2[shu] = (j+1)/10 == 0 ? ' ' : ((j+1)/10 + '0');		//十位
				shu++;
				buf2[shu] = ((j+1)%10 + '0');//个位
				shu++;
			}
			else if(hangshu < 1000)
			{
				buf2[shu] = (j+1)/100 == 0 ? ' ' : ((j+1)/100 + '0');		//百位
				shu++;
				buf2[shu] = ((j+1)/100 == 0 && (j+1)%100/10 == 0) ? ' ' : ((j+1)%100/10 + '0');//十位
				shu++;
				buf2[shu] = ((j+1)%10 + '0');//个位
				shu++;
			}
			else if(hangshu >= 1000)
			{
				buf2[shu] = (j+1)/1000 == 0 ? ' ' : ((j+1)/1000 + '0');		//千位
				shu++;
				buf2[shu] = ((j+1)/1000 == 0 && (j+1)%1000/100 == 0) ? ' ' : ((j+1)%1000/100 + '0');//百位
				shu++;
				buf2[shu] = ((j+1)/1000 == 0 && (j+1)%1000/100 == 0 && (j+1)%100/10 == 0) ? ' ' : ((j+1)%100/10 + '0');//十位
				shu++;
				buf2[shu] = ((j+1)%10 + '0');//个位
				shu++;
			}
			
		}
		
		//加！
		buf2[shu] = '!';
		shu++;
		//加空格
		buf2[shu] = ' ';
		//末尾换行
		buf2[shu+1] = '\n';
		
		fputs(buf2,fp1);	
		
	}


	fclose(fp2);
	fclose(fp1);


	exit(0);
}

