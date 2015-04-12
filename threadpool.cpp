
#include "threadpool.h"
#include <cstdio>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <time.h>
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
	 idle_head(NULL),
	 idle_end(NULL),
	 busy_head(NULL),
	 busy_end(NULL)
{
	time(&start_time);
	pthread_mutex_init(&count_lock,NULL);//初始化互斥量
	pthread_mutex_init(&list_lock,NULL); //初始化互斥量
}
/* 功能：	初始化线程池，创建最少的线程数
 * 参数：	
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

/* 功能：	创建线程
 * 参数：	number，创建的线程数
 * 返回值：	成功返回true,失败返回false
 */
bool ThreadPool::create_thread(size_t number){
	
	if(0 != pthread_mutex_lock(&count_lock))
		return false;
	Thread *tmp = NULL;
	for(size_t i = 0;i < number && num_total < num_max_t ; ++i){
		tmp = new Thread();
		/*线程默认状态是joinable,执行完不会自动释放资源，需在子线程中调用pthread_detach，把状态改为unjoinable，线程退出自动释放资源*/
		if(0 != pthread_create(&tmp->m_id,NULL,ThreadPool::thread_run,tmp)){
			delete tmp;
			tmp = NULL;
		}
		++num_total;
	}
	pthread_mutex_unlock(&count_lock);
	return true;
}

/* 功能：	往线程池中的任务队列中添加任务
 * 参数：	TaskBase基类指针或引用
 * 返回值：	成功返回true,失败返回false
 */
bool ThreadPool::add_task(TaskBase *task){
	return task_queue.add_task(task);
}
bool ThreadPool::add_task(TaskBase &task){
	return task_queue.add_task(task);
}

/* 功能：	把一个线程添加到空闲线程队列
 * 参数：	p_thread，线程对象的指针
 * 返回值：	无
 */
void ThreadPool::add_to_idle(Thread *p_thread){
	if(p_thread->m_state == isidle)
		return;
	if(0 != pthread_mutex_lock(&list_lock))
		return;
	/*如果是busy线程*/
	if(p_thread->m_state == isbusy){
		if(num_of_busy == 1)
			busy_head = busy_end = NULL;
		else if(busy_head == p_thread){  //在链表头
			busy_head = busy_head->next;
			busy_head->prev = NULL;
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
	if(idle_head == NULL)
		idle_head = idle_end = p_thread;
	else{
		idle_end->next = p_thread;
		p_thread->prev = idle_end;
		idle_end = p_thread;
	}
	p_thread->m_state = isidle;
	num_of_idle++;
	
	pthread_mutex_unlock(&list_lock);
}

/* 功能：	把一个线程添加到忙碌线程队列
 * 参数：	p_thread，线程对象的指针
 * 返回值：	无
 */
void ThreadPool::add_to_busy(Thread *p_thread){  
	if(p_thread->m_state == isbusy)
		return;
	if(0 != pthread_mutex_lock(&list_lock))
		return;
	/*如果是idle线程*/
	if(p_thread->m_state == isidle){
		if(num_of_idle == 1)
			idle_head = idle_end = NULL;
		else if(idle_head == p_thread){
			idle_head = idle_head->next;
			idle_head->prev = NULL;
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
	if(busy_head == NULL)
		busy_head = busy_end = p_thread;
	else{
		busy_end->next = p_thread;
		p_thread->prev = busy_end;
		busy_end = p_thread;
	}
	p_thread->m_state = isbusy;
	num_of_busy++;
	
	pthread_mutex_unlock(&list_lock);
}

/* 功能：	删除某个线程
 * 参数：	p_thread，线程对象的指针
 * 返回值：	无
 */
void ThreadPool::delete_thread(Thread *p_thread){
	pthread_mutex_lock(&count_lock);
	num_total--;
	pthread_mutex_unlock(&count_lock);
	
	if(0 != pthread_mutex_lock(&list_lock))
		return;
	if(num_of_busy == 1)
		busy_head = busy_end = NULL;
	else if(busy_head == p_thread){  //在链表头
		busy_head = busy_head->next;
		busy_head->prev = NULL;
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



/* 功能：	设置decrease_flag标志，使得进入等待状态的线程退出
 * 参数：	无
 * 返回值：成功返回true，失败或者线程总数不能减少了返回false
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

/* 功能：	供线程检查自己的退出条件是否满足
 * 参数：	无
 * 返回值：	成功返回true，失败返回false
 */
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

/* 功能：	为线程创建管理者
 * 参数：	无
 * 返回值：	成功返回true，失败返回false
 */
bool ThreadPool::create_manager(){
	manager_flag = true;
	if(0 != pthread_create(&m_manager,NULL,ThreadPool::manager_run,this))
		return false;
	return true;
}

/* 功能：	管理者增加线程数
 * 参数：	文件输出流，保存log文件
 * 返回值：	无
 */
void ThreadPool::manage_increase(ofstream &os){
	/*如果任务太多，那么唤醒所有线程*/
	/* */
	if(task_queue.size() > overload_tasks){
		task_queue.wake_up_all_worker();
		os << "manager try to create new threads" <<endl;
		create_thread(num_min_t);
	}
	
}

/* 功能：	管理者减少线程数
 * 参数：	文件输出流，保存log文件
 * 返回值：	无
 */
void ThreadPool::manage_decrease(ofstream& os){
	if(task_queue.size() == 0 && num_total > num_min_t){
		os << "manager try to cancel some threads"<<endl;
		decrease_thread();
	}
}

/* 功能：	获取空闲线程的数目
 * 参数：	无
 * 返回值：	整数
 */
size_t ThreadPool::get_idle_number()
 {
     size_t ret = 0;
     pthread_mutex_lock(&list_lock);
     ret = num_of_idle;
     pthread_mutex_unlock(&list_lock);
     return ret;
 }

/* 功能：	获取忙碌线程的数目
 * 参数：	无
 * 返回值：	整数
 */
size_t ThreadPool::get_busy_number()
{
     size_t ret = 0;
     pthread_mutex_lock(&list_lock);
     ret = num_of_busy;
     pthread_mutex_unlock(&list_lock);
     
     return ret;
}

/* 功能：	获取总线程的数目
 * 参数：	无
 * 返回值：	整数
 */
size_t ThreadPool::get_total_number()
{
     size_t ret = 0;
     pthread_mutex_lock(&count_lock);
     ret = num_total;
     pthread_mutex_unlock(&count_lock);
     
     return ret;
}

/* 功能：	显示线程池状态
 * 参数：	无
 * 返回值：	无
 */
void ThreadPool::display_status(ostream &os){
	os<< "running time :   "<< get_run_time() <<" seconds"<<endl;
	pthread_mutex_lock(&list_lock);
	os<< "idle number  :   "<< num_of_idle <<endl;
	os<< "busy number  :   "<< num_of_busy <<endl;
	pthread_mutex_unlock(&list_lock);
	os<< "total number :   "<< get_total_number()<<endl;
	os<< "queue size   :   "<< get_queue_size() <<endl;
	os << endl;
}
 
/* 功能：	每个线程执行的函数，内部从任务队列中取任务执行
 * 参数：	arg，线程本身的指针
 * 返回值：	void * 无意义
 */
void *ThreadPool::thread_run(void *arg){
	
	Thread * p_thread_self = static_cast<Thread *>(arg);
	pthread_t self_id = pthread_self();
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
	
	/*当前线程退出时自动释放线程资源*/
	pthread_detach(self_id);
	p_thread_pool->delete_thread(p_thread_self);
	log_output << "thread " <<self_id<<" exit..."<<endl;
	log_output.close();
	return static_cast<void *>(0);
}

/* 功能：	管理者执行的函数，负责管理线程状态和线程数目
 * 参数：	arg，管理线程本身
 * 返回值：	void * 无意义
 */
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
		
		p_thread_pool->display_status(log_output);
		
	}
	/*当前线程退出时自动释放线程资源*/
	pthread_detach(self_id);
	p_thread_pool->clear_manager_id();
	log_output << "manager thread exit..."<<endl;
	log_output.close();
	return static_cast<void *>(0);
}

