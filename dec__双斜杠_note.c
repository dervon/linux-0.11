
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
	char	buf2Tmp[BUFFSIZE2] = "";
	int shu;//argv[2]
	int IsAddHang;//argv[3]
	unsigned int ascii;
	long hangshu = 0;
	long bigZifushu;

	int incFlag = 0;

void back(int n)
{
	int i;
	for(i=n;i<BUFFSIZE2;i++)
	{
		buf2[i] = buf2[i+1];
		if(buf2[i] == '\n')
		{
			buf2[i+1] = '\0';
			break;
		}
	}
}
int IsSpace(char buf2[BUFFSIZE2])	//判断是否是空行
{
	int i;
	for(i=0;i<BUFFSIZE2;i++)
	{
		if( isalpha(buf2[i]) || (buf2[i] == '}') || (buf2[i] == '{') )
		{
			return 0;
		}
		if(buf2[i] == '\n')
		{
			break;
		}
	}
	return 1;
}
int main(int argc,char *argv[])		
{
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
	
		
		
		//找出每行的行尾
		for(n=0;n<BUFFSIZE2;n++)
		{
			if(buf2[n] == '/' && buf2[n+1] == '*')
				incFlag = 1;
			
			if(buf2[n] == '*' && buf2[n+1] == '/')
			{
				incFlag = 0;
				back(n);;
				back(n);;
				n--;
				continue;
			}

			if(incFlag == 1 && buf2[n] != '\n')
			{
				back(n);
				n--;
			}

			
				

			if(buf2[n] == '/' && buf2[n+1] == '/')
			{
				buf2[n] = '\n';
				for(i=n+1; i<BUFFSIZE2; i++)
				{
					buf2[i] = '\0';
				}
				break;
			}

			if(buf2[n] == '\n')
			{
				break;
			}
		}

		if(IsSpace(buf2Tmp) == 1 && IsSpace(buf2) == 1)	//消除重复的行
			continue;
		memcpy(buf2Tmp,buf2,BUFFSIZE2);
		
		fputs(buf2,fp1);	
		
	}


	fclose(fp2);
	fclose(fp1);


	exit(0);
}

