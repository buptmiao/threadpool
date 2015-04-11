#include "taskqueue.h"


TaskQueue::TaskQueue():queue_size(0),lock_flag(true){
	if(!initial_locks())
		lock_flag = false;
}

TaskQueue::~TaskQueue(){
	if(!destroy_locks())
		;
}

bool TaskQueue::initial_locks(){
	if(   0 != pthread_cond_init(&worker_cond,NULL)
		||0 != pthread_cond_init(&producer_cond,NULL)
		||0 != pthread_mutex_init(&queue_lock,NULL))
			return false;
			
	return true;
}

bool TaskQueue::destroy_locks(){
	if(	  0 != pthread_cond_destroy(&worker_cond)
		||0 != pthread_cond_destroy(&producer_cond)
		||0 != pthread_mutex_destroy(&queue_lock))
			return false;
			
	return true;
}

bool TaskQueue::add_task(TaskBase *task){
	TaskBase *tmp = task->clone();
	return do_add_task(tmp);
}

bool TaskQueue::add_task(TaskBase &task){
	TaskBase *tmp = task.clone();
	return do_add_task(tmp);
}

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
	/*唤  醒*/
	pthread_cond_signal(&worker_cond);
	
	return true;
}

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
	
	/*唤 醒*/
	pthread_cond_signal(&producer_cond);
	
	return task;
}
