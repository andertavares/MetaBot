#include "EpsilonGreedy.h"
#include <BWAPI.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include "../utils/tinyxml2.h"
#include "../utils/Logging.h"
#include "MetaStrategy.h"

using namespace tinyxml2;
using namespace BWAPI;

EpsilonGreedy::EpsilonGreedy(void) : MetaStrategy() {
	name = "Epsilon-greedy";
	srand(time(NULL));
}


EpsilonGreedy::~EpsilonGreedy(void) {
}

/**
 * Performs epsilon-greedy selection of strategy
 */ 
void EpsilonGreedy::onStart() {

	boost::random::mt19937 gen(static_cast<unsigned int>(std::time(0)));
    boost::random::uniform_real_distribution<> dist(0.0, 1.0);

	Logging::getInstance()->log(
        "EpsilonGreedy parameters: alpha=%.2f; epsilon=%.2f.",
        Configuration::getInstance()->alpha,
        Configuration::getInstance()->epsilon
    );

    double lucky =  dist(gen); 
    double epsilon = Configuration::getInstance()->epsilon; //alias for easy reading
    if (lucky < epsilon) { // epsilon
		Logging::getInstance()->log(
            "Choosing randomly: (%.3f < %.3f)", lucky, epsilon
        );
		name += " - random choice";
		currentStrategy = randomUniform();
    }
    else { // greedy
        Logging::getInstance()->log(
            "Choosing greedily: (%.3f > %.3f)", lucky, epsilon
        );
		name += " - greedy choice";

        //file to read is scores_MetaBot-vs-enemy.xml
        string inputFile = Configuration::getInstance()->enemyInformationInputFile();

        tinyxml2::XMLDocument doc;
        XMLError inputParseCode = doc.LoadFile(inputFile.c_str());

        if (inputParseCode == XMLError::XML_NO_ERROR) { //file parsed successfully
            //BWAPI::Player* enemy = Broodwar->enemy();
            string enemy_name = Broodwar->enemy()->getName();
            XMLElement* rootNode = doc.FirstChildElement("scores");

            if (rootNode != NULL) { //scores are there
                
                string best_name;
                float best_score = -1.0f;

				// initializes map of names to scores with ones
				// score found in XML file override the initial value
				map<string, float> scoresMap;
				initializeMap<float>(scoresMap, 1);

                //scoresMap[MetaStrategy::SKYNET] = 0;
                //scoresMap[MetaStrategy::XELNAGA] = 0;
				// looks up all scores in XML and sets it in scoresMap
				XMLElement* candidate = rootNode->FirstChildElement();
                while (candidate != NULL) {
                    float score = -FLT_MAX;
                    candidate->QueryFloatText(&score);

                    scoresMap[candidate->Name()] = score;
                    candidate = candidate->NextSiblingElement();
                }

				// finds the strategy with the best score
                for (std::map<string, float>::iterator it = scoresMap.begin(); it != scoresMap.end(); ++it) {
                    if (it->second > best_score) {
                        best_name = it->first;
                        best_score = it->second;
                    }
                }

                if (!best_name.empty()) {
					Logging::getInstance()->log(
						"Choosing '%s' greedily. Score: %f.", best_name.c_str(), best_score
					);
					currentStrategy = portfolio[best_name];
                }
                else { // if unable to determine best strategy for some reason, choose randomly
					Logging::getInstance()->log("Best strategy could not be determined. Choosing randomly");
					name += " failed. Best score not found";
					currentStrategy = randomUniform();
                }
            } 
            else { // if xml file has no <scores> tag, choose randomly
                Logging::getInstance()->log("Enemy information not found, choosing strategy randomly");
				name += " failed. Enemy info not found";
                currentStrategy = randomUniform();
            }
        }
        else { // if xml parsing has failed, prints error
            Logging::getInstance()->log(
                "Error while parsing scores file '%s': '%s'. Choosing randomly.",
                inputFile.c_str(),
                doc.ErrorName()
            );
			name += " failed. File not found?";
            currentStrategy = randomUniform();
        }
	}
	Logging::getInstance()->log("%s: onStart() - executed in EpsilonGreedy::onStart", getCurrentStrategyName().c_str());
	currentStrategy->onStart();
}

void EpsilonGreedy::onFrame() {
    return; //does nothing, because epsilon-greedy does not change strategy during the match

	/*
	int thisFrame = Broodwar->getFrameCount();
    myBehaviorName = MetaStrategy::getInstance()->getStrategy();

    MatchData::getInstance()->registerMyBehaviorName(myBehaviorName);
    currentBehavior = behaviors[myBehaviorName];
    currentBehavior->onFrame();
    logger->log("%s on!", myBehaviorName.c_str());

    if (Broodwar->elapsedTime() / 60 >= 81) {	//leave stalled game
        Broodwar->leaveGame();
        return;
    }

    currentBehavior->onFrame();
	
    //draws some text
    Broodwar->drawTextScreen(240, 20, "\x0F MetaBot v1.0.2");
    Broodwar->drawTextScreen(240, 35, "\x0F Strategy: %s", myBehaviorName.c_str());
    //Broodwar->drawTextScreen(5, 25, "\x0F Enemy behavior: %s", enemyBehaviorName.c_str());
    Broodwar->drawTextScreen(240, 45, "\x0F Enemy: %s", Broodwar->enemy()->getName().c_str());
    Broodwar->drawTextScreen(240, 60, "Frame count %d.", thisFrame);
    Broodwar->drawTextScreen(240, 75, "Seconds: %d.", Broodwar->elapsedTime());
	*/
}

void EpsilonGreedy::discountCrashes() {
    using namespace tinyxml2;

    //file to read is scores_MetaBot-vs-enemy.xml
    string inputFile = Configuration::getInstance()->enemyInformationInputFile();
    string outputFile = Configuration::getInstance()->enemyInformationOutputFile();
    string crashFile = Configuration::getInstance()->crashInformationInputFile();

	tinyxml2::XMLDocument crashesDoc;
    XMLError crashParseCode = crashesDoc.LoadFile(crashFile.c_str());

    tinyxml2::XMLDocument scoresDoc;
    XMLError inputParseCode = scoresDoc.LoadFile(inputFile.c_str());

    if (crashParseCode == XMLError::XML_NO_ERROR) {
        XMLElement* rootNode = crashesDoc.FirstChildElement("crashes");
        if (rootNode != NULL) {
            XMLElement* behavior = rootNode->FirstChildElement();

			// initializes crashesMap with zeros
			// they will be overridden if found in files
            map<string, int> crashesMap;
			initializeMap<int>(crashesMap, 0);
            //crashesMap[MetaStrategy::XELNAGA] = 0;

			// traverses all behaviors in xml file, filling the crash map
            while (behavior != NULL) {
                int crashes;
				behavior->QueryIntText(&crashes);
				crashesMap[behavior->Name()] = crashes;
                behavior = behavior->NextSiblingElement();
            }

            if (inputParseCode == XMLError::XML_NO_ERROR) { //scores file successfully opened
                XMLElement* inputRootNode = scoresDoc.FirstChildElement("scores");

                if (inputRootNode != NULL) {
                    XMLElement* input_behavior = inputRootNode->FirstChildElement();

					// initializes scores with ones
                    map<string, float> scoresMap;
					initializeMap<float>(scoresMap, 1);
                    //scoresMap[MetaStrategy::NUSBot] = 0;

					// for each behavior in scores file, discounts the crashes by simulating Q-learning updates
                    while (input_behavior != NULL) {
                        // finds the score stored in the scores file
						float score;
						input_behavior->QueryFloatText(&score);

						// simulates Q-learning updates with reward of loss to punish the crashes
                        float alpha = Configuration::getInstance()->alpha;
                        //if (score > 0 && input_behavior == behavior) {
						//one Q-learning update per crash
						for (int i = crashesMap[input_behavior->Name()]; i > 0; i--) {
							score = (1 - alpha)*score + alpha * (-1); //crash is a loss, thus -1 is the reward
						}

                        //}
						// puts the score back in the xml element
                        scoresMap[input_behavior->Name()] = score;
                        input_behavior->SetText(score);
                        input_behavior = input_behavior->NextSiblingElement();
                    }
                }
				// saves the updated scores back to the scores file
                scoresDoc.SaveFile(outputFile.c_str());
            }
            else { // if unable to parse score file, prints error
				Logging::getInstance()->log(
                    "Error while parsing score file '%s'. Error: '%s'",
                    Configuration::getInstance()->enemyInformationInputFile().c_str(),
                    scoresDoc.ErrorName()
                );
            }
        } // (rootNode != NULL)
    }
    else { // if unable to parse crash file, prints error
        Logging::getInstance()->log(
            "Error while parsing crash file '%s'. Error: '%s'",
            Configuration::getInstance()->crashInformationInputFile().c_str(),
            crashesDoc.ErrorName()
        );
    }
}

