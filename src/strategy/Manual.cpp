#include "Manual.h"


Manual::Manual(void) {
	name = "Manual";
}


Manual::~Manual(void) {
}

void Manual::onStart(){
	currentStrategy = portfolio["Manual"];
	Logging::getInstance()->log("Manual mode activated");
}
