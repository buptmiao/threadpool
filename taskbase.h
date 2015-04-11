#ifndef _TASKBASE_H_
#define _TASKBASE_H_

#include <string>
#include <fstream>
using namespace std;

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
	string in_arg;
	string out_arg;
	bool has_work;
	
};

#endif
