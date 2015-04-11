#ifndef _TASK_QUEUE_H_
#define _TASK_QUEUE_H_

#include <queue>
#include <pthread.h>
#include "taskbase.h"

using namespace std;

const size_t MAX_TASK_NUMBER = 10000;

class TaskQueue {
public:
	TaskQueue();
	virtual ~TaskQueue();
	
	bool	add_task(TaskBase *task);
	bool	add_task(TaskBase &task);
	void	wake_up_worker();
	void	wake_up_producer();
	
	TaskBase *get_task();
	
	bool wake_up_all_worker(){
		if(0 != pthread_cond_broadcast(&worker_cond))
			return false;
		return true;
	}
	
	size_t size();
	
	bool get_lock_flag() const{
		return lock_flag;
	}
private:
	TaskQueue(const TaskQueue&);
	TaskQueue& operator=(const TaskQueue&);
	
	queue<TaskBase *> m_task_queue;
	
	pthread_cond_t worker_cond;
	pthread_cond_t producer_cond;
	pthread_mutex_t queue_lock;
	
	size_t queue_size;
	bool	lock_flag;
	
	bool	initial_locks();
	bool	destroy_locks();
	
	bool	do_add_task(TaskBase *);
};

#endif
