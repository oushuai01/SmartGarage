/*
* 语音合成（Text To Speech，TTS）技术能够自动将任意文字实时转换为连续的
* 自然语音，是一种能够在任何时间、任何地点，向任何人提供语音信息服务的
* 高效便捷手段，非常符合信息时代海量数据、动态更新和个性化查询的需求。
*/

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
#include "../../include/qtts.h"
#include "../../include/msp_cmn.h"
#include "../../include/msp_errors.h"
typedef int SR_DWORD;
typedef short int SR_WORD ;

/* wav音频头部格式 */
typedef struct _wave_pcm_hdr
{
	char            riff[4];                // = "RIFF"
	int				size_8;                 // = FileSize - 8
	char            wave[4];                // = "WAVE"
	char            fmt[4];                 // = "fmt "
	int				fmt_size;				// = 下一个结构体的大小 : 16

	short int       format_tag;             // = PCM : 1
	short int       channels;               // = 通道数 : 1
	int				samples_per_sec;        // = 采样率 : 8000 | 6000 | 11025 | 16000
	int				avg_bytes_per_sec;      // = 每秒字节数 : samples_per_sec * bits_per_sample / 8
	short int       block_align;            // = 每采样点字节数 : wBitsPerSample / 8
	short int       bits_per_sample;        // = 量化比特数: 8 | 16

	char            data[4];                // = "data";
	int				data_size;              // = 纯数据长度 : FileSize - 44 
} wave_pcm_hdr;

/* 默认wav音频头部数据 */
wave_pcm_hdr default_wav_hdr = 
{
	{ 'R', 'I', 'F', 'F' },
	0,
	{'W', 'A', 'V', 'E'},
	{'f', 'm', 't', ' '},
	16,
	1,
	1,
	16000,
	32000,
	2,
	16,
	{'d', 'a', 't', 'a'},
	0  
};
/* 文本合成 */
int text_to_speech(const char* src_text, const char* des_path, const char* params)
{
	int          ret          = -1;
	FILE*        fp           = NULL;
	const char*  sessionID    = NULL;
	unsigned int audio_len    = 0;
	wave_pcm_hdr wav_hdr      = default_wav_hdr;
	int          synth_status = MSP_TTS_FLAG_STILL_HAVE_DATA;

	if (NULL == src_text || NULL == des_path)
	{
		printf("params is error!\n");
		return ret;
	}
	fp = fopen(des_path, "wb");
	if (NULL == fp)
	{
		printf("open %s error.\n", des_path);
		return ret;
	}
	/* 开始合成 */
	sessionID = QTTSSessionBegin(params, &ret);
	if (MSP_SUCCESS != ret)
	{
		printf("QTTSSessionBegin failed, error code: %d.\n", ret);
		fclose(fp);
		return ret;
	}
	ret = QTTSTextPut(sessionID, src_text, (unsigned int)strlen(src_text), NULL);
	if (MSP_SUCCESS != ret)
	{
		printf("QTTSTextPut failed, error code: %d.\n",ret);
		QTTSSessionEnd(sessionID, "TextPutError");
		fclose(fp);
		return ret;
	}
	fwrite(&wav_hdr, sizeof(wav_hdr) ,1, fp); //添加wav音频头，使用采样率为16000
	while (1) 
	{
		/* 获取合成音频 */
		const void* data = QTTSAudioGet(sessionID, &audio_len, &synth_status, &ret);
		if (MSP_SUCCESS != ret)
			break;
		if (NULL != data)
		{
			fwrite(data, audio_len, 1, fp);
		    wav_hdr.data_size += audio_len; //计算data_size大小
		}
		if (MSP_TTS_FLAG_DATA_END == synth_status)
			break;
	}
	printf("\n");
	if (MSP_SUCCESS != ret)
	{
		printf("QTTSAudioGet failed, error code: %d.\n",ret);
		QTTSSessionEnd(sessionID, "AudioGetError");
		fclose(fp);
		return ret;
	}
	/* 修正wav文件头数据的大小 */
	wav_hdr.size_8 += wav_hdr.data_size + (sizeof(wav_hdr) - 8);
	
	/* 将修正过的数据写回文件头部,音频文件为wav格式 */
	fseek(fp, 4, 0);
	fwrite(&wav_hdr.size_8,sizeof(wav_hdr.size_8), 1, fp); //写入size_8的值
	fseek(fp, 40, 0); //将文件指针偏移到存储data_size值的位置
	fwrite(&wav_hdr.data_size,sizeof(wav_hdr.data_size), 1, fp); //写入data_size的值
	fclose(fp);
	fp = NULL;
	/* 合成完毕 */
	ret = QTTSSessionEnd(sessionID, "Normal");
	if (MSP_SUCCESS != ret)
	{
		printf("QTTSSessionEnd failed, error code: %d.\n",ret);
	}

	return ret;
}

void *setDate(void *arg)
{
	while(1)
	{
		system("date -s \"2016/11/11\"");
		usleep(200*1000);
	}
}

int main(int argc, char* argv[])
{
	int         ret                  = MSP_SUCCESS;
	const char* login_params         = "appid = 5d24053b, work_dir = .";//登录参数,appid与msc库绑定,请勿随意改动
	/*
	* rdn:           合成音频数字发音方式
	* volume:        合成音频的音量
	* pitch:         合成音频的音调
	* speed:         合成音频对应的语速
	* voice_name:    合成发音人
	* sample_rate:   合成音频采样率
	* text_encoding: 合成文本编码格式
	*
	*/
	const char* session_begin_params = "engine_type = purextts,voice_name=xiaoyan, text_encoding = UTF8, tts_res_path = fo|res/xtts/xiaoyan.jet;fo|res/xtts/common.jet, sample_rate = 16000, speed = 50, volume = 50, pitch = 50, rdn = 2";
	const char* filename             = "a.wav"; //合成的语音文件名称

	/* 用户登录 */
	ret = MSPLogin(NULL, NULL, login_params); //第一个参数是用户名，第二个参数是密码，第三个参数是登录参数，用户名和密码可在http://www.xfyun.cn注册获取
	if (MSP_SUCCESS != ret)
	{
		printf("MSPLogin failed, error code: %d.\n", ret);
		goto exit ;//登录失败，退出登录
	}

	
    // 1，创建UDP通信端点
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1)
    {
        perror("创建UDP端点失败");
        exit(0);
    }
    else
        printf("创建UDP端点成功\n");


    // 2，准备好地址结构体和相应的地址信息
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    bzero(&addr, len);

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr("192.168.9.199");
    addr.sin_port        = htons(50001); // 转换成网络字节序

    // 3，绑定地址(IP + PORT)
    if(bind(sockfd, (struct sockaddr *)&addr, len) != 0)
    {
        perror("绑定地址失败");
        exit(0);
    }
    else
        printf("绑定地址成功\n");

    // pthread_t t;
    // pthread_create(&t, NULL, setDate, NULL);

    // 4，坐等对方待合成文本
    char text[500];
    while(1)
    {
        // A. 准备好开发板的地址结构体和相应的地址信息
        struct sockaddr_in armAddr;
        len = sizeof(armAddr);
        bzero(&armAddr, len);

        // B. 接收开发板发来的信息，并且得到对方的地址
        bzero(text, 500);
        recvfrom(sockfd, text, 500, 0, (struct sockaddr *)&armAddr, &len);
        printf("收到【%s:%hu】待合成文本:%s\n",
                inet_ntoa(armAddr.sin_addr),
                ntohs(armAddr.sin_port), text);

        // C. 使用TTS语音合成引擎转换成wav文件
	 	/* 文本合成 */
	printf("开始合成 ...\n");
	while(text_to_speech(text, filename, session_begin_params)!=MSP_SUCCESS)
	{
		printf("text_to_speech failed, error code: %d.\n", ret);
		sleep(1);
	}
	printf("合成完毕\n");

	// D. 取得语音文件的大小
	int fd = open("a.wav", O_RDWR);

	// 将文件位置（相当于光标）调整到文件的末尾
	// 并获取当前位置
	uint32_t size = lseek(fd, 0, SEEK_END);

	// 将文件位置（相当于光标）重新调整到开头
	// 以便于可以正常读取内容
	lseek(fd, 0, SEEK_SET);

	// E. 将文件大小发给开发板
	int n = sendto(sockfd, &size, sizeof(size), 0,
			(struct sockaddr *)&armAddr, len);
	if(n <= 0)
	{
		perror("发送sendto失败");
		exit(0);
	}
	else
		printf("已经成功发送%d个字节数据\n", n);

	// F. 取得语音文件的内容并将之发给开发板
	char *buf = malloc(10*1024); // 申请10k的内存缓冲区
	while(1)
	{
		// 从文件中读取n个字节的数据
		bzero(buf, 10*1024);
		int n = read(fd, buf, 10*1024);

		// 什么都没读到，即已经将文件读完了
		if(n == 0)
			break;

		// 将这个n个字节发给开发板
		sendto(sockfd, buf, n, 0,
				(struct sockaddr *)&armAddr, len);
	}

	// G. 释放相关的资源
	free(buf);
	close(fd);
    }

exit:
    printf("按任意键退出 ...\n");
    getchar();
    MSPLogout(); //退出登录

    return 0;
}

