#include "taskbase.h"

void TaskBase::run(ofstream& os){
	if(!has_work)
		os << "this is a empty work..."<<endl;
	else
		os << "working on " << in_arg <<endl;
}