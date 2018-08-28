#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>

#include "MetaBot.h"
#include "StarcraftUnitInfo/UnitInfo/Source/UnitInfoManager.h"
#include "strategy/MetaStrategy.h"
#include "strategy/MetaStrategyManager.h"
#include "data/Configuration.h"
#include "data/MatchData.h"
#include "utils/Logging.h"
#include <cstdlib>
#include <ctime>

using namespace BWAPI;
using namespace scutil;

MetaBot::MetaBot() : acknowledged(false) {
    
	logger = Logging::getInstance();

    //Configuration::getInstance()->parseConfig(); (moved to onStart)

    MatchData::getInstance()->registerEnemyBehaviorName("Unknown");
    enemyBehaviorName = "Unknown";

	//srand (static_cast <unsigned> (time(0)));
}

void MetaBot::onStart() {
    // Uncomment to enable complete map information
    //Broodwar->enableFlag(Flag::CompleteMapInformation);

    MatchData::getInstance()->registerMatchBegin();
    Configuration::getInstance()->parseConfig();

	// old code to test skynet
	//metaStrategy = MetaStrategyManager::getMetaStrategy();
	//currentStrategy = metaStrategy->loadAIModule("Skynet");
	//currentStrategy->onStart();

	//initializes and registers meta strategy (strategy selector)
	metaStrategy = MetaStrategyManager::getMetaStrategy();
	MatchData::getInstance()->registerMetaStrategy(metaStrategy->getName());
	metaStrategy->onStart();

	//retrieves strategy to begin match
	currentStrategy = metaStrategy->getCurrentStrategy();
	
	logger->log("Game started with '%s'!", metaStrategy->getCurrentStrategyName().c_str());

    MatchData::getInstance()->registerMyBehaviorName(metaStrategy->getCurrentStrategyName());
    // initializes crash counter
	MatchData::getInstance()->writeToCrashFile();
	
	//initializes the UnitInfoManager
	UnitInfoManager::getInstance();
	
    //sets user input, speed and GUI 
    Broodwar->enableFlag(Flag::UserInput);

    int speed = Configuration::getInstance()->speed;
	logger->log("Setting speed to %d.", speed);
    Broodwar->setLocalSpeed(speed);

    bool gui = Configuration::getInstance()->enableGUI;
	logger->log("Setting GUI to %s.", gui ? "enabled" : "disabled");
    Broodwar->setGUI(gui);
}

void MetaBot::onEnd(bool isWinner) {
	string strategyName = metaStrategy->getCurrentStrategyName();

    logger->log("Game ended well with %s.", strategyName.c_str());
    
	int result;

    //if (Broodwar->elapsedTime() / 60 >= 80) result = MatchData::DRAW;
    //tournament rules for draw are 86400 frames, but we reduce 100 to ensure counting
    if (Broodwar->getFrameCount() >= 86300) {
		result = MatchData::DRAW;
		logger->log("Draw. Finishing behavior: %s.", strategyName.c_str());
	}
	else if (isWinner) {
        result = MatchData::WIN;
        logger->log("Victory! Winner behavior: %s.", strategyName.c_str());
    }
	else {
		result = MatchData::LOSS;
		logger->log("Defeat :( Finishing behavior: %s.", strategyName.c_str());
	}
	
    MatchData::getInstance()->registerMatchFinish(result);
	MatchData::getInstance()->writeSummary();
    MatchData::getInstance()->updateScoresFile();
    MatchData::getInstance()->updateCrashFile();	//TODO: valid only for epsilon-greedy!
	
	currentStrategy->onEnd(isWinner);
	logger->log("Finished.");
}

void MetaBot::onFrame() {

	if(Broodwar->getFrameCount() == 0){
		logger->log("BEGIN: first onFrame.");
	}
	//just prints 'Alive...' so that we know things are ok
	if (Broodwar->getFrameCount() % 100 == 0) {
		Logging::getInstance()->log("Alive.");
    }

	if (Broodwar->elapsedTime() / 60 >= 81) {	//leave stalled game
		logger->log("Leaving long game (81 minutes)");
		Broodwar->leaveGame();
        return;
    }
	
	if(Broodwar->getFrameCount() == 0){ logger->log("first metaStrategy->onFrame()"); }
	metaStrategy->onFrame();	//might switch strategy so I update currentStrategy below

	if(Broodwar->getFrameCount() == 0){ logger->log("first strategy->onFrame()"); }
	currentStrategy = metaStrategy->getCurrentStrategy();
	
	currentStrategy->onFrame();
	
	if(Broodwar->getFrameCount() == 0){ logger->log("first UnitInfoManager->onFrame()"); }
	UnitInfoManager::getInstance().onFrame();

	//draws some text
    Broodwar->drawTextScreen(240, 20, "\x0F MetaBot v1.0.2");
	Broodwar->drawTextScreen(240, 35, "\x0F Meta strategy: %s", metaStrategy->getName().c_str());
	Broodwar->drawTextScreen(240, 50, "\x0F Strategy: %s", metaStrategy->getCurrentStrategyName().c_str());
    Broodwar->drawTextScreen(240, 65, "\x0F Enemy: %s", Broodwar->enemy()->getName().c_str());
	Broodwar->drawTextScreen(240, 80, "Frame count %d.", Broodwar->getFrameCount());
    Broodwar->drawTextScreen(240, 95, "Seconds: %d.", Broodwar->elapsedTime());

	if(Broodwar->getFrameCount() == 0){
		logger->log("END: first onFrame.");
	}
}

void MetaBot::onSendText(std::string text) {
	metaStrategy->onSendText(text);
	currentStrategy->onSendText(text);
}

void MetaBot::onReceiveText(BWAPI::Player* player, std::string text) {
    currentStrategy->onReceiveText(player, text);
	//handshake(text);
}

void MetaBot::onPlayerLeft(BWAPI::Player* player) {
    currentStrategy->onPlayerLeft(player);
}

void MetaBot::onNukeDetect(BWAPI::Position target) {
    currentStrategy->onNukeDetect(target);
}

void MetaBot::onUnitDiscover(BWAPI::Unit* unit) {
	UnitInfoManager::getInstance().onUnitDiscover(unit);
    currentStrategy->onUnitDiscover(unit);
}

void MetaBot::onUnitEvade(BWAPI::Unit* unit) {
	UnitInfoManager::getInstance().onUnitEvade(unit);
    currentStrategy->onUnitEvade(unit);
}

void MetaBot::onUnitShow(BWAPI::Unit* unit) {
	UnitInfoManager::getInstance().onUnitShow(unit);
    currentStrategy->onUnitShow(unit);
}

void MetaBot::onUnitHide(BWAPI::Unit* unit) {
	UnitInfoManager::getInstance().onUnitHide(unit);
    currentStrategy->onUnitHide(unit);
}

void MetaBot::onUnitCreate(BWAPI::Unit* unit) {
	UnitInfoManager::getInstance().onUnitCreate(unit);
    currentStrategy->onUnitCreate(unit);
}

void MetaBot::onUnitDestroy(BWAPI::Unit* unit) {
	logger->log(
		"onUnitDestroy with %s (id=%d) of player %s", 
		unit->getType().getName().c_str(),
		unit->getID(),
		unit->getPlayer()->getName().c_str()
	);

	UnitInfoManager::getInstance().onUnitDestroy(unit);
    currentStrategy->onUnitDestroy(unit);
}

void MetaBot::onUnitMorph(BWAPI::Unit* unit) {
    UnitInfoManager::getInstance().onUnitMorph(unit);
	currentStrategy->onUnitMorph(unit);
}

void MetaBot::onUnitRenegade(BWAPI::Unit* unit) {
	UnitInfoManager::getInstance().onUnitRenegade(unit);
    currentStrategy->onUnitRenegade(unit);
}

void MetaBot::onSaveGame(std::string gameName) {
    currentStrategy->onSaveGame(gameName);
}

void MetaBot::onUnitComplete(BWAPI::Unit *unit) {
    UnitInfoManager::getInstance().onUnitComplete(unit);
	currentStrategy->onUnitComplete(unit);
}

/*
string MetaBot::myBehavior() {
    return myBehaviorName;
}

string MetaBot::enemyBehavior() {
    return enemyBehaviorName;
}
*/
void MetaBot::handshake(string text){
	//behavior message recognition: checks whether text ends with 'on!'
    if (enemyBehaviorName == "Unknown" && text.substr(max(3, int(text.size())) - 3) == string("on!")) {
        //splits text in spaces and gets 1st part: this is enemy's name
        istringstream iss(text);
        vector<string> tokens;
        copy(
            istream_iterator<string>(iss),
            istream_iterator<string>(),
            back_inserter(tokens)
        );
        //the 'magic' above is from: http://stackoverflow.com/a/237280/1251716

        enemyBehaviorName = tokens[0];

        Broodwar->printf(">>>>> Enemy recognized: %s <<<<<", enemyBehaviorName.c_str());
        MatchData::getInstance()->registerEnemyBehaviorName(enemyBehaviorName);

        //sends text back to enemy to acknowledge recognition
        Broodwar->sendText("ACKNOWLEDGED!");
    }
    //recognizes that enemy has acknowledged my strategy
    else if (text == "ACKNOWLEDGED!") {
        acknowledged = true;
    }
}

