#include <time.h>
#include <wait.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <linux/fb.h>
#include <linux/input.h>

#include <arpa/inet.h>

#include "head.h"

int main(int argc, char const *argv[])
{
    // 1，创建UDP通信端点
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1)
    {
        perror("创建UDP端点失败");
        exit(0);
    }

    // 2，准备好虚拟机的地址结构体和相应的地址信息
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    bzero(&addr, len);

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr("192.168.9.100");
    addr.sin_port        = htons(50001); // 转换成网络字节序

    // 3，准备好管道
    int fifo = open(SQLite2Audio,  O_RDWR);
    if(fifo == -1)
    {
	    perror("音频模块打开管道失败");
	    exit(0);
    }

    // 4，向主控程序汇报子模块启动成功
    sem_t *s = sem_open(SEM_OK, 0666);
    sem_post(s); // 将信号量SEM_OK的值+1

    // 5，随时准备将SQLite发来的文本转成语音并播放出来
    char text[500];
    while(1)
    {
        // A. 静静地等待SQLite发来的语音文本
        bzero(text, 500);
        read(fifo, text, 500);

        // B. 发送给SDK去帮忙合成语音
        int n = sendto(sockfd, text, strlen(text), 0,
                        (const struct sockaddr *)&addr, len);
        if(n <= 0)
        {
            perror("发送数据失败");
            exit(0);
        }

    	// C. 静静地等待对方发来对应语音文件的大小
    	uint32_t size;
    	recvfrom(sockfd, &size, sizeof(size), 0, NULL, NULL);

    	// D. 创建一个专门用来存储对方语音数据的wav文件
    	int fd = open("a.wav", O_RDWR|O_CREAT|O_TRUNC, 0777);

    	// E. 开始不断接收SDK的语音文件数据
    	char *wav = malloc(10*1024);
    	while(size > 0)
    	{
    		// 从SDK接收最多不超过10KB个字节的数据
    		// 返回值 n 代表真正读取到的数据字节数
    		int n = recvfrom(sockfd, wav, 10*1024, 0, NULL, NULL);

    		// 将这 n 个字节的数据，妥善地保存到文件a.wav中
    		write(fd, wav, n);

    		size -= n;
    	}

    	// F. 关闭相应资源
    	free(wav);
    	close(fd);

    	// G. 播放合成的语音（在C程序中可以使用system来执行命令）
        printf("\r播报语音：%s\n", text);
    	system("aplay a.wav");
    }

    return 0;
}
