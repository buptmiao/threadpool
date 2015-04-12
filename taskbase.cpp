

#include "taskbase.h"

/* 功能：	抽象接口，任务执行入口
 * 参数：	文件输出流
 * 返回值：	无
 */
void TaskBase::run(ofstream& os){
	if(!has_work)
		os << "this is a empty work..."<<endl;
	else
		os << "working on " << in_arg <<endl;
}