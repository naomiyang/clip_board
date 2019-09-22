#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/time.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <string.h>
#include <sys/shm.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtkclipboard.h>
#include "cpbmonitor.h"
#define MAXLEN 512
#define MAX_THREAD 20
#define cal_date(){ gettimeofday(&now,NULL);\
    outtime.tv_sec = now.tv_sec + 10;\
    outtime.tv_nsec = now.tv_usec * 1000;\
    pthread_mutex_init(&mutex,NULL);\
    pthread_cond_init(&cond,NULL);\
    pthread_mutex_lock(&mutex);\
    printf("cal___date\n");\
}

const key_t key = 0x12345;
const key_t key1 = 0x1234;
pthread_cond_t cond;
int current_hash = 0;
int old_hash = 0;
int i = 0;
int shmid;
int shmid1;
//shared mem for cmd
int *shm;
//shared mem for str
char *share;
char *replace;
//GdkColorspace color;
GtkClipboard* clipboard;
//GdkPixbuf* pixbuf_tmp;

//上发的结构体
struct result res;
//接收的结构体
struct receive recv;


ClipboardMonitorFuncPtr callbk;
unsigned int hash_string(char *str)
{
	unsigned int seed = 131;
	unsigned int hash = 0;
	while(*str){
		hash = hash * seed + (*str++);
	}
	return (hash & 0x7FFFFFFF);
}

void send_cmd(int *cmd)
{
    printf("addr shm:%p\n", shm);
    *shm = *cmd;
}

void * thread_send(void *arg)
{
    struct result *resp = &res;
    if ( share != NULL && NULL != &cond ) {
        printf("thread_send:%s\n",share);
        strcpy(resp->msg, (char*)share);
        printf("clear share\n");
        memset(share, 0, MAXLEN);
        resp->cond = &cond;
        sleep(12);
        //回调函数
        callbk(resp);
    }
}

int Init(ClipboardMonitorFuncPtr callback)
{
    gtk_init(NULL, NULL);
    shmid = shmget(key, sizeof(int),0666 | IPC_CREAT);
    shm = (int *)shmat(shmid,0,0);
    shmid1 = shmget(key1,MAXLEN,0666 | IPC_CREAT);
    share =(char *)shmat(shmid1,0,0);
    callbk = callback; 
    clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  //  pixbuf_tmp = gdk_pixbuf_new(color,FALSE,8,100,100);
}

int Start()
{
    time_t start,end;
    int cost;
    struct timeval now;
    struct timespec outtime;
    while(1){
        current_hash = hash_string((char *)share);
        pthread_mutex_t mutex;
        if(old_hash != current_hash){
            time(&start);
            cal_date();
            pthread_t tid;
            pthread_create(&tid,NULL,thread_send,NULL);
            pthread_cond_timedwait(&cond,&mutex,&outtime);
            time(&end);
            pthread_mutex_unlock(&mutex);
            cost = difftime(end,start);
            if(cost == 10){
                printf("等待事件：10秒");
                gtk_clipboard_set_text(clipboard,share,strlen(share));
                old_hash = current_hash;
                //memset(share,0,MAXLEN);
                printf("share:%s\n",share);
            }
        }
        gtk_main();
    }

}

int Stop()
{
    gtk_main_quit();
}

int ResponseActionResult(struct receive recv)
{
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex,NULL);
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(recv.cond);
    *shm = recv.reply;
    if(*shm == 0){
        gtk_clipboard_set_text(clipboard,share,strlen(share));
        old_hash = current_hash;
      //  memset(share,0,MAXLEN);
//        gtk_clipboard_set_image(clipboard,share);
    }else if(*shm == 1){
        gtk_clipboard_set_text(clipboard,"",0);
        old_hash = current_hash;
  //      gtk_clipboard_set_image(clipboard,pixbuf_tmp);
    }
    memset(share,0,MAXLEN);
    pthread_mutex_unlock(&mutex);
}
