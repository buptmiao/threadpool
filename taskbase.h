#ifndef _TASKBASE_H_
#define _TASKBASE_H_

#include <string>
#include <fstream>
using namespace std;

/* 任务基类：定义抽象接口run方法，实际任务需要继承任务基类，并定义自己的run方法
 *		
 */
class TaskBase{
	friend class TaskQueue;
public:
	TaskBase():has_work(false){}
	TaskBase(string in):in_arg(in),has_work(true){}
	virtual ~TaskBase(){}
	virtual void run(ofstream& os);
	
	virtual TaskBase *clone(){
		return new TaskBase(*this);
	}
	
	void set_path(string &arg){
		in_arg = arg;
	}
	
	bool empty(){
		return has_work;
	}
private:
	int priority;
	string in_arg;
	string out_arg;
	bool has_work;
	
};

#endif
