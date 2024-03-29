#include <stdio.h>
#include <assert.h>
#include <fcntl.h> 
#include <unistd.h>
#include <termios.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <strings.h> // for bzero()
#include <stdbool.h>

#include "sqlite3.h"
#include "head.h"

bool first = true;

int fifoIN;
int fifoOUT;
int fifoToAudio;
int fifoFromVideo;

sem_t *s_ok;
sem_t *s_takePhoto;

sqlite3 *db = NULL; // 数据库的操作句柄
char *err;          // 报错信息

time_t t;

// 每当你使用SELECT得到N条记录时，就会自动调用N次以下函数
// 参数：
// arg: 用户自定义参数
// len: 列总数
// col_val: 每一列的值
// col_name: 每一列的名称（标题)
int callback(void *arg, int len, char **col_val, char **col_name)
{
    if(arg != NULL)
    {
        (*(int *)arg)++; // n++;
        return 0;
    }

    // 显示标题(只显示一次)
    if(first)
    {
        printf("\n");
        for(int i=0; i<len; i++)
        {
            printf("\r%s\t\t", col_name[i]);
        }
        printf("\n======================");
        printf("======================\n");
        first = false;
    }

    // 显示内容(一行一行输出)
    for(int i=0; i<len; i++)
    {
        printf("%s\t", col_val[i]);

    // 保存当前记录的时间的值
    if(i == 2)
        t = atol(col_val[i]);
    }
    printf("\n");

    // 返回0: 继续针对下一条记录调用本函数
    // 返回非0: 停止调用本函数
    return 0;
}

/*************************************************
功能:蜂鸣器发出报警声
*************************************************/
void beep(int times, float sec)
{
     int buz = open("/dev/beep", O_RDWR);
     if(buz <= 0)
     {
         perror("打开蜂鸣器失败");
         return;
     }

     for(int i=0; i<times; i++)
     {
         // 响
         ioctl(buz, 0, 1);
         usleep(sec*1000*1000);

         // 静
         ioctl(buz, 1, 1);
         usleep(sec*1000*1000);
     }

     close(buz);
}

/*************************************************
功能:打印当前数据库数据，方便查看
*************************************************/
void showDB()
{
    char SQL[100];

    // 显示当前数据库内的内容
    bzero(SQL, 100);
    snprintf(SQL, 100, "SELECT * FROM info;");

    first = true;
    sqlite3_exec(db, SQL, callback, NULL/*用户自定义参数*/, &err);
    printf("======================");
    printf("======================\n\n");
}

/*************************************************
功能:用户刷身份卡入车库时，将用户身份卡卡号和用户对应车牌号入库的线程
*************************************************/
void *carIn(void *arg)
{
    char SQL[100];

    int id;
    char license_plate[10];
    time_t t;
    while(1)
    {
        // A. 静静地等待RFID发来的入库卡号并读取，对应写入身份卡卡号的代码在RFID.c文件里
        read(fifoIN, &id, sizeof(id));

        // B. 检测该入库卡号是否合法
        bzero(SQL, 100);
        snprintf(SQL, 100, "SELECT * FROM info WHERE 卡号='%x';", id);
        int n = 0;
        sqlite3_exec(db, SQL, callback, &n/*用户自定义参数*/, &err);

        // B1. 数据库里查到了有n个当前传入的卡号，本次操作非法，发出警报，嘀嘀嘀……
        if(n > 0)
        {
            fprintf(stderr, "\r该卡已入场，请勿重复刷卡\n");
            beep(5, 0.05);
            continue;
        }
        // B2: 数据库里没有查到当前传入卡号，操作合法:
        else
        {
            // 当前用户身份卡符合进入车库要求，将信号量SEM_TAKEPHOTO的值+1，通知视频模块进行视频抓拍
            sem_post(s_takePhoto);

            // 等待视频抓拍的图片识别出来的车牌，视频模块识别出车牌号后会通过fifoFromVideo管道发送到这里来读取
            bzero(license_plate, 10);
            read(fifoFromVideo, license_plate, 10);

            // 通过fifoToAudio管道，给音频模块发送欢迎文本
            char msg[50];
            snprintf(msg, 50, "欢迎%s入场！", license_plate);
            write(fifoToAudio, msg, strlen(msg));

            // 将当前车牌号也入库
            snprintf(SQL, 100, "INSERT INTO info VALUES"
                            "('%x', '%s', '%lu');", id, license_plate, time(NULL));
            sqlite3_exec(db, SQL, NULL, NULL, NULL);

            showDB();
        }
    }
}

/*************************************************
功能:计算当前用户在停车场停车的时长，输出对应的语音文本
*************************************************/
const char *payment(long startTime)
{
    static char *numbers[] =
        {"零", "一", "二", "三", "四", "五",
         "六", "七", "八", "九", "十", "百"};

    static char msg[100];
    bzero(msg, 100);

    time_t t = time(NULL) - startTime;

    if(t <= 10)
        snprintf(msg, 100, "停车时长%d秒, 收费%s元", t, numbers[t]);

    else if(t < 20)
        snprintf(msg, 100, "停车时长十%s秒, 收费十%s元",
                    numbers[t-10], numbers[t-10]);

    else if(t == 20)
        snprintf(msg, 100, "停车时长二十秒, 收费二十元");

    else if(t < 30)
        snprintf(msg, 100, "停车时长二十%s秒, 收费二十%s元",
                    numbers[t-20], numbers[t-20]);

    else
        snprintf(msg, 100, "停车超时, 收费一百元");

    return msg;
}

/*************************************************
功能:用户刷身份卡出车库时，将用户身份卡卡号入库的线程
*************************************************/
void *carOut(void *arg)
{
    char SQL[100];

    // 静静地等待RFID发来的出库卡号
    int id;
    while(1)
    {
        // 获取当前刷身份卡出库的身份卡的卡号
        read(fifoOUT, &id, sizeof(id));

        // A) 查询指定要出库的卡号是否存在
        //    如果不存在，则发出警告
        //    如果存在，则获取该id对应的入库时间t
        bzero(SQL, 100);
        snprintf(SQL, 100, "SELECT * FROM info WHERE 卡号='%x';", id);
        int n = 0;
        sqlite3_exec(db, SQL, callback, &n/*用户自定义参数*/, &err);

        if(n == 0)
        {
            fprintf(stderr, "\r该卡已出场，请勿重复刷卡\n");
            beep(5, 0.05);
        }
        else
        {
            // B) 存在的情况下，删除该记录
            bzero(SQL, 100);
            snprintf(SQL, 100, "DELETE FROM info WHERE 卡号='%x';", id);

            sqlite3_exec(db, SQL, NULL, NULL, &err);

            // C) 通过fifoToAudio管道，给语音模块发送收费文本
            const char *msg = payment(t);
            write(fifoToAudio, msg, strlen(msg));

            showDB();
        }
    }
}

int main()
{
    // 1，创建、打开一个数据库文件*.db
    int ret = sqlite3_open_v2("parking.db", &db,
                              SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, NULL);
    if(ret != SQLITE_OK)
    {
        printf("创建数据库文件失败:%s\n", sqlite3_errmsg(db));
        exit(0);
    }

    // 2，创建表Table，表头是：卡号、车牌、时间
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS info"
             "(卡号 TEXT PRIMARY KEY, 车牌 TEXT, 时间 TEXT);",
                 NULL, NULL, &err);

    // 3.1，准备好与RFID模块沟通的通信管道
    fifoIN  = open(RFID2SQLiteIN,  O_RDWR);
    fifoOUT = open(RFID2SQLiteOUT, O_RDWR);

    // 3.2，准备好与视频模块沟通的通信管道 
    fifoToAudio   = open(SQLite2Audio, O_RDWR);
    fifoFromVideo = open(Video2SQLite, O_RDWR);

    if(fifoIN==-1     || fifoOUT==-1 ||
       fifoToAudio==-1|| fifoFromVideo==-1)
    {
        perror("数据库模块打开管道失败");
        exit(0);
    }

    // 准备好向主控程序通知本模块启动成功的信号量
    s_ok        = sem_open(SEM_OK, 0666);

    // 准备好与视频模块沟通的信号量
    s_takePhoto = sem_open(SEM_TAKEPHOTO, 0666);

    // 5，创建入库、出库线程
    pthread_t t1, t2;
    pthread_create(&t1, NULL, carIn,  NULL);
    pthread_create(&t2, NULL, carOut, NULL);

    // 6，延时一小会，等本进程一切就绪，向主控程序汇报子模块启动成功
    usleep(600);
    sem_post(s_ok);

    // 7，主线程退出
    pthread_exit(NULL);
}
