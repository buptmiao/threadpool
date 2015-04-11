
#include "threadpool.h"
#include <cstdio>
#include <iostream>
#include <fstream>
#include <unistd.h>

/* 单例模式
 * 
 */
ThreadPool *ThreadPool::m_instance = NULL;

ThreadPool *ThreadPool::create_instance(){
	if(m_instance == NULL)
		m_instance = new ThreadPool();
	/*       任务队列       */
	return m_instance;
}
/* 构造函数
 * 
 *
 */
ThreadPool::ThreadPool()
    :initialed(false),
	 decrease_flag(false),
	 manager_flag(false),
	 period_of(TIME_PERIOD_SEC),
	 overload_tasks(OVERLOAD_TASKS),
	 num_total(0),
	 num_min_t(MIN_THREAD_NUM),
	 num_max_t(MAX_THREAD_NUM),
	
	 
	 num_of_idle(0),
	 num_of_busy(0),
	 idle_header(NULL),
	 idle_end(NULL),
	 busy_header(NULL),
	 busy_end(NULL)
{
	pthread_mutex_init(&count_lock,NULL);//初始化互斥量
	pthread_mutex_init(&list_lock,NULL); //初始化互斥量
}
/* 功能：	创建线程
 * 参数：	num表示即将创建的线程池内的线程数
 * 返回值：	成功返回true,失败返回false
 */
bool ThreadPool::init(){
	if(initialed)
		return true;
	/* 创建num个线程 */
	if(!create_thread(num_min_t))
		return false;
	if(manager_flag)	
		if(!create_manager())
			cout<<"	craete manager failed!	"<<endl;
	initialed = true;
	return true;
}

bool ThreadPool::create_thread(size_t number){
	if(0 != pthread_mutex_lock(&count_lock))
		return false;
	if(num_total == MAX_THREAD_NUM){//线程数已满，返回
		pthread_mutex_unlock(&count_lock);
		return false;
	}
	Thread *tmp = NULL;
	for(size_t i = 0;i < number;++i){
		tmp = new Thread();
		if(0 == pthread_create(&tmp->m_id,NULL,ThreadPool::thread_run,tmp)){
			if(++num_total == MAX_THREAD_NUM)
				break;
		}
		else{
			delete tmp;
			tmp = NULL;
		}
	}
	pthread_mutex_unlock(&count_lock);
	return true;
}


bool ThreadPool::add_task(TaskBase *task){
	return task_queue.add_task(task);
}
bool ThreadPool::add_task(TaskBase &task){
	return task_queue.add_task(task);
}

void ThreadPool::add_to_idle(Thread *p_thread){
	if(0 != pthread_mutex_lock(&list_lock))
		return;
	/*如果是busy线程*/
	if(p_thread->m_state == isbusy){
		if(num_of_busy == 1)
			busy_header = busy_end = NULL;
		else if(busy_header == p_thread){  //在链表头
			busy_header = busy_header->next;
			busy_header->prev = NULL;
		}
		else if(busy_end == p_thread){  //在链表尾
			busy_end = busy_end->prev;
			busy_end->next = NULL;
		}else{							//
			p_thread->prev->next = p_thread->next;
			p_thread->next->prev = p_thread->prev;
		}
		num_of_busy--;
	}
	/*插入到idle链表*/
	if(idle_header == NULL)
		idle_header = idle_end = p_thread;
	else{
		idle_end->next = p_thread;
		p_thread->prev = idle_end;
		idle_end = p_thread;
	}
	p_thread->m_state = isidle;
	num_of_idle++;
	
	pthread_mutex_unlock(&list_lock);
}



void ThreadPool::add_to_busy(Thread *p_thread){  
	if(0 != pthread_mutex_lock(&list_lock))
		return;
	/*如果是idle线程*/
	if(p_thread->m_state == isidle){
		if(num_of_idle == 1)
			idle_header = idle_end = NULL;
		else if(idle_header == p_thread){
			idle_header = idle_header->next;
			idle_header->prev = NULL;
		}
		else if(idle_end == p_thread){
			idle_end = idle_end->prev;
			idle_end->next = NULL;
		}else{
			p_thread->prev->next = p_thread->next;
			p_thread->next->prev = p_thread->prev;
		}
		num_of_idle--;
	}
	/*c插入到busy链表*/
	if(busy_header == NULL)
		busy_header = busy_end = p_thread;
	else{
		busy_end->next = p_thread;
		p_thread->prev = busy_end;
		busy_end = p_thread;
	}
	p_thread->m_state = isbusy;
	num_of_busy++;
	
	pthread_mutex_unlock(&list_lock);
}


void ThreadPool::remove_thread(Thread *p_thread){
	pthread_mutex_lock(&count_lock);
	num_total--;
	pthread_mutex_unlock(&count_lock);
	
	if(0 != pthread_mutex_lock(&list_lock))
		return;
	if(num_of_busy == 1)
		busy_header = busy_end = NULL;
	else if(busy_header == p_thread){  //在链表头
		busy_header = busy_header->next;
		busy_header->prev = NULL;
	}
	else if(busy_end == p_thread){  //在链表尾
		busy_end = busy_end->prev;
		busy_end->next = NULL;
	}else{							//
		p_thread->prev->next = p_thread->next;
		p_thread->next->prev = p_thread->prev;
	}
	num_of_busy--;	
	
	delete p_thread;
	p_thread = NULL;
	pthread_mutex_unlock(&list_lock);
}



/*
 * 设置decrease_flag标志，使得进入等待状态的线程退出
 * 
 */

bool ThreadPool::decrease_thread(){
	
	if(num_total <= num_min_t)
		return false;
		
	decrease_flag = true;
	//唤醒所有线程
	if(!task_queue.wake_up_all_worker())
		return false;
	return true;
}

bool ThreadPool::check_decrease(){
	bool res = true;
	if(decrease_flag == false)
		return false;
	if(0 != pthread_mutex_lock(&count_lock))
		return false;
	if(num_total > num_min_t)
		num_total--;
	else
		res = false;
	pthread_mutex_unlock(&count_lock);
	return res;
}

bool ThreadPool::create_manager(){
	manager_flag = true;
	if(0 != pthread_create(&m_manager,NULL,ThreadPool::manager_run,this))
		return false;
	return true;
}

void ThreadPool::manage_increase(ofstream& os){
	/*如果任务太多，那么唤醒所有线程*/
	if(task_queue.size() > overload_tasks){
		task_queue.wake_up_all_worker();
	}
	sleep(10);
	/*如果任务仍然太多，创建新的线程*/
	if(task_queue.size() > overload_tasks){
		os << "manager try to create new threads" <<endl;
		create_thread(num_min_t);
	}
}

void ThreadPool::manage_decrease(ofstream& os){
	if(task_queue.size() == 0 && num_total > num_min_t){
		os << "manager try to cancel some threads"<<endl;
		decrease_thread();
	}
}


size_t ThreadPool::get_idle_number()
 {
     size_t ret = 0;
     pthread_mutex_lock(&list_lock);
     ret = num_of_idle;
     pthread_mutex_unlock(&list_lock);
     return ret;
 }
     
size_t ThreadPool::get_busy_number()
{
     size_t ret = 0;
     pthread_mutex_lock(&list_lock);
     ret = num_of_busy;
     pthread_mutex_unlock(&list_lock);
     
     return ret;
}

size_t ThreadPool::get_total_number()
{
     size_t ret = 0;
     pthread_mutex_lock(&count_lock);
     ret = num_total;
     pthread_mutex_unlock(&count_lock);
     
     return ret;
}

/*
 * 功能：每个线程的运行函数，调用任务的run方法
 * 
 * 
 */
void *ThreadPool::thread_run(void *arg){
	
	Thread * p_thread_self = static_cast<Thread *>(arg);
	pthread_t self_id = pthread_self();
	if(self_id == p_thread_self->m_id)
		cout<<"it is the same"<<endl;
	ThreadPool* p_thread_pool = ThreadPool::create_instance();
	TaskQueue &	tq = p_thread_pool->task_queue;
	
	/*创建日志文件*/
	ofstream log_output;
	char file_name[20] = {0};
	sprintf(file_name,"%u_log",static_cast<unsigned int>(self_id));
	log_output.open(file_name);
	if(!log_output)
		pthread_exit(static_cast<void *>(0));
	log_output << "thread "<<self_id<<" start..."<<endl;
	log_output << "pid " <<getpid()<<endl;
	
	while(true){
		p_thread_pool->add_to_idle(p_thread_self);
		TaskBase *t = tq.get_task();
		if(t != NULL){
			p_thread_pool->add_to_busy(p_thread_self);
			t->run(log_output);
			delete t;
			t = NULL;
			
			if(p_thread_pool->check_decrease())
				break;
		}
	}
	
	/**/
	pthread_detach(self_id);
	p_thread_pool->remove_thread(p_thread_self);
	log_output << "thread " <<self_id<<" exit..."<<endl;
	log_output.close();
	return static_cast<void *>(0);
}


void *ThreadPool::manager_run(void *arg){
	pthread_t self_id = pthread_self();
	
	ThreadPool *p_thread_pool = static_cast<ThreadPool*>(arg);
	
	ofstream log_output;
	log_output.open("manage_log",ofstream::app);
	if(!log_output)
		pthread_exit(static_cast<void *>(0));
	log_output << "manager thread start..."<<endl;
	log_output << "pid:"<<getpid()<<endl;
	
	while(true){
		/* 检查是否需要增加线程 */
		p_thread_pool->manage_increase(log_output);
		/* 是否需要减少线程 */
		p_thread_pool->manage_decrease(log_output);
		
		if(!p_thread_pool->get_manager_flag())
			break;
		log_output<< "manager try to sleep now..."<<endl;
		sleep(p_thread_pool->get_during_seconds());
		log_output<< "idle number: "<< p_thread_pool->get_idle_number()<<endl;
		log_output<< "busy number: "<< p_thread_pool->get_busy_number()<<endl;
		log_output<< "total number:"<< p_thread_pool->get_total_number()<<endl;
		log_output<< "queue size:  "<< p_thread_pool->get_queue_size() <<endl;
		
	}
	pthread_detach(self_id);
	p_thread_pool->clear_manager_id();
	log_output << "manager thread exit..."<<endl;
	log_output.close();
	return static_cast<void *>(0);
}

