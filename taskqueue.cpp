

#include "taskqueue.h"

/* 功能：	任务队列构造函数
 * 参数：	无
 * 返回值：	无
 */
TaskQueue::TaskQueue():queue_size(0),lock_flag(true){
	if(!initial_locks())
		lock_flag = false;
}

/* 功能：	任务队列析构函数
 * 参数：	无
 * 返回值：	无
 */
TaskQueue::~TaskQueue(){
	if(!destroy_locks())
		;
}

/* 功能：	初始化锁和互斥量
 * 参数：	无
 * 返回值：	成功返回true，失败返回false
 */
bool TaskQueue::initial_locks(){
	if(   0 != pthread_cond_init(&worker_cond,NULL)
		||0 != pthread_cond_init(&producer_cond,NULL)
		||0 != pthread_mutex_init(&queue_lock,NULL))
			return false;
			
	return true;
}

/* 功能：	销毁锁和互斥量
 * 参数：	无
 * 返回值：	成功返回true，失败返回false
 */
bool TaskQueue::destroy_locks(){
	if(	  0 != pthread_cond_destroy(&worker_cond)
		||0 != pthread_cond_destroy(&producer_cond)
		||0 != pthread_mutex_destroy(&queue_lock))
			return false;
			
	return true;
}

/* 功能：	添加任务
 * 参数：	task任务基类指针或引用
 * 返回值：	成功返回true，失败返回false
 */
bool TaskQueue::add_task(TaskBase *task){
	
	/*任务基类指针可能指向的是派生类对象，需调用虚函数clone*/
	TaskBase *tmp = task->clone();
	return do_add_task(tmp);
}

bool TaskQueue::add_task(TaskBase &task){
	
	/*任务基类引用可能绑定的是派生类对象，需调用虚函数clone*/
	TaskBase *tmp = task.clone();
	return do_add_task(tmp);
}

/* 功能：	添加实际任务到任务队列
 * 参数：	task任务基类指针
 * 返回值：	成功返回true，失败返回false
 */
bool TaskQueue::do_add_task(TaskBase* task){
	if(0 != pthread_mutex_lock(&queue_lock)){
		delete task;
		task = NULL;
		return false;
	}
	
	while(queue_size >= MAX_TASK_NUMBER)//while循环做条件判断
		pthread_cond_wait(&producer_cond,&queue_lock);
	m_task_queue.push(task);
	queue_size++;
	
	pthread_mutex_unlock(&queue_lock);
	/*唤醒阻塞在worker_cond上的所有线程*/
	pthread_cond_signal(&worker_cond);
	
	return true;
}

/* 功能：	从任务队列中获取任务
 * 参数：	无
 * 返回值：	返回任务指针
 */
TaskBase* TaskQueue::get_task(){
	TaskBase *task = NULL;
	if(0 != pthread_mutex_lock(&queue_lock))
		return task;
	while(queue_size == 0)
		pthread_cond_wait(&worker_cond,&queue_lock);
	task = m_task_queue.front();

	m_task_queue.pop();
	queue_size--;
	pthread_mutex_unlock(&queue_lock);
	
	/*唤醒阻塞在producer_cond上的所有线程*/
	pthread_cond_signal(&producer_cond);
	
	return task;
}

/* 功能：	从任务队列中获取任务
 * 参数：	无
 * 返回值：	返回任务指针
 */
 
size_t TaskQueue::size()
{
    size_t ret = 0;
    pthread_mutex_lock(&queue_lock);
    ret = queue_size;
    pthread_mutex_unlock(&queue_lock);
    
    return ret;
}

