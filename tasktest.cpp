
#include "taskbase.h"
#include "tasktest.h"
#include "unistd.h"
int tasktest::count = 0;

TaskBase *tasktest::clone(){
	return new tasktest(*this);
}
void tasktest::run(ofstream &os){
		os << "I am going to sleep now" <<endl;
		usleep(1000);
		os << "I am awake now,bye"<<endl;
}
