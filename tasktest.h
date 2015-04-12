#ifndef _TASKTEST_H_
#define _TASKTEST_H_

#include "taskbase.h"

/*	一个任务测试类，继承自TaskBase
 * 
 */
 
class tasktest : public TaskBase{
public:
	void run(ofstream& os);
	TaskBase *clone();
private:
	static int count;
};


#endif