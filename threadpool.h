#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <queue>
#include <fstream>
#include <pthread.h>
#include "taskqueue.h"
#include "time.h"
using namespace std;

const size_t MAX_THREAD_NUM  	= 	100;
const size_t TIME_PERIOD_SEC	=	1;
const size_t OVERLOAD_TASKS		=	100;
const size_t MIN_THREAD_NUM     =   5; 
/*线程状态*/
enum Threadstate{
	isbusy = 0,
	isidle,
	newcreated
};

/* 线程类：每一个线程记录线程的id和线程状态
 * 		   设置线程池类为友元
 */
class Thread{
	friend class ThreadPool;
	Thread():m_id(0),prev(NULL),next(NULL),m_state(newcreated){}
	pthread_t 	m_id;
	Thread 		*prev;
	Thread 		*next;
	Threadstate m_state;
};
 
/*线程池类：创建线程池,初始化初始线程数量，可以对idle和busy的线程进行调度
 *			使用两个链表来维护idle和busy的线程。
 * 			可以为该线程池创建管理者，负责添加和销毁线程，并可以指示线程退出
 * 			
 */

class ThreadPool{
public:
	static ThreadPool *create_instance();
	~ThreadPool();
	bool init();
	bool create_thread(size_t number);
	bool decrease_thread();
//	bool decrease_to_none();
	bool check_decrease();
	bool create_manager();
	void manage_increase(ofstream& os);
	void manage_decrease(ofstream& os);
	
	void display_status(ostream &os);
	
	unsigned int get_idle_number();
	unsigned int get_busy_number();
	unsigned int get_total_number();
	
	bool add_task(TaskBase *task);
	bool add_task(TaskBase &task);
	
	ThreadPool* set_max_number(size_t number){
		num_max_t = number;
		return this;
	}
	ThreadPool* set_manager_flag(bool flag){
		manager_flag = flag;
		return this;
	}
	ThreadPool* set_during_seconds(size_t sec){
		period_of = sec;
		return this;
	}
	ThreadPool* set_overload_tasks(size_t size){
		overload_tasks = size;
		return this;
	}
	void clear_manager_id(){
		m_manager = 0;
	}
	
	bool get_manager_flag() const{
		return manager_flag;
	}
	size_t get_during_seconds() const{
		return period_of;
	}
	size_t get_queue_size(){
		return task_queue.size();
	}
	time_t get_run_time(){
		time_t timer;
		return difftime(time(&timer),start_time);
	}
private:
	//把构造函数和复制构造函数设置为私有，禁止复制，并实现单例
	ThreadPool();
	ThreadPool(const ThreadPool&);
	ThreadPool& operator=(const ThreadPool&);
	
	void   				add_to_idle(Thread *);
	void 				add_to_busy(Thread *);
	void				delete_thread(Thread *);
	static ThreadPool*	m_instance;				//保存唯一实例的指针
	static void*		thread_run (void *arg); //线程运行函数
	static void*		manager_run(void *arg); //管理者运行函数
private:
	pthread_t 			m_manager;  //  管理者线程id
	pthread_mutex_t		count_lock; //  局部锁
	pthread_mutex_t 	list_lock;  //	列表锁
	
	bool 				initialed;
	bool				manager_flag;
	
	size_t 				period_of;  //
	size_t				overload_tasks; //任务过载数量
	
	time_t				start_time; //  线程池启动时间
	size_t 				num_total;	//  总的线程数
	size_t				num_min_t;  //  最小线程数
	size_t				num_max_t;	//  最大线程数
	size_t				num_of_idle;//空闲线程数
	size_t 				num_of_busy;//忙碌线程数
	Thread*             idle_head; // idle双向链表头
    Thread*             idle_end;
    Thread*             busy_head; // busy双向链表头
    Thread*             busy_end;

	TaskQueue			task_queue; //	任务队列
	
};

#endif
