#include "ManualSelection.h"


ManualSelection::ManualSelection(void) {
	name = "ManualSelection";
}


ManualSelection::~ManualSelection(void) {
}

void ManualSelection::onStart(){
	currentStrategy = portfolio["BaseBehavior"];
	Logging::getInstance()->log("ManualSelection initializing BaseBehavior");

	currentStrategy->onStart();
}

void ManualSelection::onSendText(std::string text){
		vector<string> strategies = getPortfolioName();
		if(text.substr(0, 2) == "c ")
		{
		Logging::getInstance()->log("Will attempt a strategy switch to %s", text.substr(2, text.size()).c_str());
		forceStrategy(text.substr(2, text.size()));
		}else if(std::find(strategies.begin(), strategies.end(), text) != strategies.end())
		{
		Logging::getInstance()->log("Will attempt a strategy switch to %s", text.c_str());
		forceStrategy(text);
		}
}