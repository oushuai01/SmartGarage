#include "head.h"

pid_t p1, p2, p3, p4;

/*************************************************
功能:如果刷卡、数据库、语音、视频这4个子进程启动失败就调用该函数
将所有打开的子进程都杀死并退出程序
*************************************************/
void quit(int sig)
{
	printf("子模块运行失败，再见！\n");
	if(p1!=0)kill(p1, SIGKILL);
	if(p2!=0)kill(p2, SIGKILL);
	if(p3!=0)kill(p3, SIGKILL);
	if(p4!=0)kill(p4, SIGKILL);
	exit(0);
}

/*************************************************
功能:如果主控程序退出就调用该函数，将所有打开的子进程都杀死并退出程序
*************************************************/
void cleanup(int sig)
{
	printf("全部退出，再见！\n");
	if(p1!=0)kill(p1, SIGKILL);
	if(p2!=0)kill(p2, SIGKILL);
	if(p3!=0)kill(p3, SIGKILL);
	if(p4!=0)kill(p4, SIGKILL);
	exit(0);
}

/*************************************************
功能:一直打印句号，可感受到程序一直在运行，感观上更好
*************************************************/
bool printDot = false;
void *routine(void *arg)
{
	while(1)
	{
		if(printDot)
		{
			fprintf(stderr, ".");
		}

		usleep(200*1000);
	}
}

int main(int argc, char **argv)
{
	if(argc != 3)
	{
		printf("参数错误，请指定串口和摄像头\n");
		printf("例如: ./main /dev/ttySAC2 /dev/video7\n");
		exit(0);
	}

	// 创建各种管道，用于各模块之间的数据交互
	// 创建 4 个命名管道
	mkfifo(RFID2SQLiteIN, 0777);
	mkfifo(RFID2SQLiteOUT,0777);
	mkfifo(SQLite2Audio,  0777);
	mkfifo(Video2SQLite,  0777);

	// 创建信号量，用于各模块之间的互相通知
	remove(SEM_OK);
	remove(SEM_TAKEPHOTO);
	sem_open(SEM_OK, O_CREAT, 0777, 0);
	sem_open(SEM_TAKEPHOTO, O_CREAT, 0777, 0);

	sem_t *s = sem_open(SEM_OK, 0666);

	// 如果有子模块启动失败，则主控程序也相应地退出
	signal(SIGCHLD, quit);

	// 如果主控程序退出，那么也依次退出清除所有的子模块
	signal(SIGINT/*ctrl+c*/, cleanup);

	// 为了体验度更佳，专门创建一个打印...的线程
	pthread_t t;
	pthread_create(&t, NULL, routine, NULL);


	// 创建子进程: 执行刷卡
	if((p1=fork()) == 0)
	{
		execl("./RFID", "RFID", argv[1], NULL);
	}
	fprintf(stderr, "启动RFID模块中..");
	printDot = true;
	sem_wait(s); // 等待子模块启动成功
	printDot = false;
	printf("成功\n");


	// 创建子进程: 执行数据库
	if((p2=fork()) == 0)
	{
		execl("./SQLite", "SQLite", NULL);
	}
	fprintf(stderr, "启动数据库模块中..");
	printDot = true;
	sem_wait(s); // 等待子模块启动成功
	printDot = false;
	printf("成功\n");


	// 创建子进程: 执行语音
	if((p3=fork()) == 0)
	{
		execl("./Audio", "Audio", NULL);
	}
	fprintf(stderr, "启动音频模块中..");
	printDot = true;
	sem_wait(s); // 等待子模块启动成功
	printDot = false;
	printf("成功\n");


	// 创建子进程: 执行视频
	if((p4=fork()) == 0)
	{
		execl("./Video", "Video", argv[2], NULL);
	}
	fprintf(stderr, "启动视频模块中..");
	printDot = true;
	sem_wait(s); // 等待子模块启动成功
	printDot = false;
	printf("成功\n");

    // 主程序不能退出
    pause();

	return 0;
}
