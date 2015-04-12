#include <stdio.h>
#include "threadpool.h"
#include "taskbase.h"
#include "tasktest.h"
#include <stdlib.h>

/*
pthread有两种状态joinable状态和unjoinable状态

 一个线程默认的状态是joinable，如果线程是joinable状态，当线程函数自己返回退出时或pthread_exit时都不会释放线程所占用堆栈和线程描述符（总计8K多）。只有当你调用了pthread_join之后这些资源才会被释放。
若是unjoinable状态的线程，这些资源在线程函数退出时或pthread_exit时自动会被释放。

unjoinable属性可以在pthread_create时指定，或在线程创建后在线程中pthread_detach自己, 如：pthread_detach(pthread_self())，将状态改为unjoinable状态，确保资源的释放。如果线程状态为 joinable,需要在之后适时调用pthread_join.
 * */
int main(int argc, char **argv)
{
	/* 创建一个线程池 */
	ThreadPool *pool = ThreadPool::create_instance();
	pool->set_manager_flag(true);
	pool->init();
	
	for(int i = 0;i < 100000;i++){
		tasktest tmp;
		pool->add_task(tmp);
	}
	sleep(100);
	return 0;
}

class sleep;