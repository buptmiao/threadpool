
#include "taskbase.h"
#include "tasktest.h"

int tasktest::count = 0;

TaskBase *tasktest::clone(){
	return new tasktest(*this);
}
void tasktest::run(ofstream &os){
		os << "I am going to sleep now" <<endl;
		sleep(3);
		os << "I am awake now,bye"<<endl;
}
