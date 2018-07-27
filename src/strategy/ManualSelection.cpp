#include "ManualSelection.h"
#include "UnitInfoManager.h"

ManualSelection::ManualSelection(void) {
	name = "ManualSelection";
}


ManualSelection::~ManualSelection(void) {
}

void ManualSelection::onStart(){
	//Set base behavior as current strategy
	currentStrategy = portfolio["BaseBehavior"];
	Logging::getInstance()->log("ManualSelection initializing BaseBehavior");

	//Display welcome message and options
	Logging::getInstance()->log("Welcome to the ManualSelection!");
	Logging::getInstance()->log("Type the strategy name you want to select");


	currentStrategy->onStart();
}

void ManualSelection::onSendText(std::string text){
		vector<string> strategies = getPortfolioName();
		
		//Switch strategy selected
		if(std::find(strategies.begin(), strategies.end(), text) != strategies.end())
		{
			Logging::getInstance()->log("Will attempt a strategy switch to %s", text.c_str());
			forceStrategy(text);
		}
}