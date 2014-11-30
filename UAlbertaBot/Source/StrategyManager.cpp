#include "Common.h"
#include "StrategyManager.h"


// constructor
StrategyManager::StrategyManager() 
	: firstAttackSent(false)
	, currentStrategy(0)
	, selfRace(BWAPI::Broodwar->self()->getRace())
	, enemyRace(BWAPI::Broodwar->enemy()->getRace())
{
	addStrategies();
	setStrategy();
}

// get an instance of this
StrategyManager & StrategyManager::Instance() 
{
	static StrategyManager instance;
	return instance;
}

void StrategyManager::addStrategies() 
{
	protossOpeningBook = std::vector<std::string>(NumProtossStrategies);
	terranOpeningBook  = std::vector<std::string>(NumTerranStrategies);
	zergOpeningBook    = std::vector<std::string>(NumZergStrategies);

	protossOpeningBook[ProtossProbeRush]	= "0 0 0 0 1 0 3 0 0 0 4";
	protossOpeningBook[ProtossAirRush]		= "0 0 0 0 1 0 9 0 0 10 0 10 0 10 0 7 0 1 0 3 0 10 0 5 0 10 0 17 1 17 18 18 10 30 1 10 10 18 18 1";
    protossOpeningBook[ProtossZealotRush]	= "0 0 0 0 1 0 3 3 0 0 4 1 4 4 0 4 4 0 1 4 3 0 1 0 4 0 4 4 4 4 1 0 4 4 4";
    protossOpeningBook[ProtossDarkTemplar]	= "0 0 0 0 1 0 3 0 7 0 5 0 12 0 13 3 22 22 1 22 22 0 1 0";
	protossOpeningBook[ProtossDragoons]		= "0 0 0 0 1 0 0 3 0 7 0 0 5 0 0 3 8 6 1 6 6 0 3 1 0 6 6 6";
	terranOpeningBook[TerranMarineRush]		= "0 0 0 0 0 1 0 0 3 0 0 3 0 1 0 4 0 0 0 6";
	zergOpeningBook[ZergZerglingRush]		= "0 0 0 0 0 1 0 0 0 2 3 5 0 0 0 0 0 0 1 6";

	if (selfRace == BWAPI::Races::Protoss)
	{
		results = std::vector<IntPair>(NumProtossStrategies);

		if(BWTA::getStartLocations().size() == 2) {
			usableStrategies.push_back(ProtossProbeRush);
		}

		if (enemyRace == BWAPI::Races::Protoss)
		{
			usableStrategies.push_back(ProtossAirRush);
			usableStrategies.push_back(ProtossZealotRush);
			usableStrategies.push_back(ProtossDarkTemplar);
			usableStrategies.push_back(ProtossDragoons);
		}
		else if (enemyRace == BWAPI::Races::Terran)
		{
			usableStrategies.push_back(ProtossAirRush);
			usableStrategies.push_back(ProtossZealotRush);
			usableStrategies.push_back(ProtossDarkTemplar);
			usableStrategies.push_back(ProtossDragoons);
		}
		else if (enemyRace == BWAPI::Races::Zerg)
		{
			usableStrategies.push_back(ProtossAirRush);
			usableStrategies.push_back(ProtossZealotRush);
			usableStrategies.push_back(ProtossDragoons);
		}
		else
		{
			BWAPI::Broodwar->printf("Enemy Race Unknown");
			usableStrategies.push_back(ProtossProbeRush);
			usableStrategies.push_back(ProtossAirRush);
			usableStrategies.push_back(ProtossZealotRush);
			usableStrategies.push_back(ProtossDragoons);
		}
	}
	else if (selfRace == BWAPI::Races::Terran)
	{
		results = std::vector<IntPair>(NumTerranStrategies);
		usableStrategies.push_back(TerranMarineRush);
	}
	else if (selfRace == BWAPI::Races::Zerg)
	{
		results = std::vector<IntPair>(NumZergStrategies);
		usableStrategies.push_back(ZergZerglingRush);
	}

	if (Options::Modules::USING_STRATEGY_IO)
	{
		readResults();
	}
}

void StrategyManager::readResults()
{
	struct stat buf;
	std::string file = Options::FileIO::READ_DIR + "HCB-" + BWAPI::Broodwar->enemy()->getName() + ".txt";

	// if the file doesn't exist, set the results to zeros
	if (stat(file.c_str(), &buf) == -1) {
		std::fill(results.begin(), results.end(), IntPair(0,0));
	}
	else {
		std::ifstream f_in(file.c_str());
		std::string line;
		getline(f_in, line);
		//results[ProtossProbeRush].first = atoi(line.c_str());
		//getline(f_in, line);
		//results[ProtossProbeRush].second = atoi(line.c_str());
		//getline(f_in, line);
		results[ProtossAirRush].first = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossAirRush].second = atoi(line.c_str());
		f_in.close(); // Tamara is this a bug?
		getline(f_in, line);
		results[ProtossZealotRush].first = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossZealotRush].second = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossDarkTemplar].first = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossDarkTemplar].second = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossDragoons].first = atoi(line.c_str());
		getline(f_in, line);
		results[ProtossDragoons].second = atoi(line.c_str());
	}
}

void StrategyManager::writeResults()
{
	std::string file = Options::FileIO::WRITE_DIR + "HCB-" + BWAPI::Broodwar->enemy()->getName() + ".txt";
	std::ofstream f_out(file.c_str());

	f_out << results[ProtossProbeRush].first	<< "\n";
	f_out << results[ProtossProbeRush].second	<< "\n";
	f_out << results[ProtossAirRush].first      << "\n";
	f_out << results[ProtossAirRush].second     << "\n";
	f_out << results[ProtossZealotRush].first   << "\n";
	f_out << results[ProtossZealotRush].second  << "\n";
	f_out << results[ProtossDarkTemplar].first  << "\n";
	f_out << results[ProtossDarkTemplar].second << "\n";
	f_out << results[ProtossDragoons].first     << "\n";
	f_out << results[ProtossDragoons].second    << "\n";

	f_out.close();
}

void StrategyManager::setStrategy()
{
	// if we are using file io to determine strategy, do so
	if (Options::Modules::USING_STRATEGY_IO)
	{
		double bestUCB = -1;
		int bestStrategyIndex = 0;

		// UCB requires us to try everything once before using the formula
		for (size_t strategyIndex(0); strategyIndex<usableStrategies.size(); ++strategyIndex)
		{
			int sum = results[usableStrategies[strategyIndex]].first + results[usableStrategies[strategyIndex]].second;

			if (sum == 0)
			{
				currentStrategy = usableStrategies[strategyIndex];
				return;
			}
		}

		// if we have tried everything once, set the maximizing ucb value
		for (size_t strategyIndex(0); strategyIndex<usableStrategies.size(); ++strategyIndex)
		{
			double ucb = getUCBValue(usableStrategies[strategyIndex]);

			if (ucb > bestUCB)
			{
				bestUCB = ucb;
				bestStrategyIndex = strategyIndex;
			}
		}
		
		currentStrategy = usableStrategies[bestStrategyIndex];
	}
	else
	{
		currentStrategy = Options::Macro::DEFAULT_STRATEGY;
	}
}

void StrategyManager::onEnd(const bool isWinner)
{
	// write the win/loss data to file if we're using IO
	if (Options::Modules::USING_STRATEGY_IO)
	{
		// if the game ended before the tournament time limit
		if (BWAPI::Broodwar->getFrameCount() < Options::Tournament::GAME_END_FRAME)
		{
			if (isWinner)
			{
				results[getCurrentStrategy()].first = results[getCurrentStrategy()].first + 1;
			}
			else
			{
				results[getCurrentStrategy()].second = results[getCurrentStrategy()].second + 1;
			}
		}
		// otherwise game timed out so use in-game score
		else
		{
			if (getScore(BWAPI::Broodwar->self()) > getScore(BWAPI::Broodwar->enemy()))
			{
				results[getCurrentStrategy()].first = results[getCurrentStrategy()].first + 1;
			}
			else
			{
				results[getCurrentStrategy()].second = results[getCurrentStrategy()].second + 1;
			}
		}
		
		writeResults();
	}
}

const double StrategyManager::getUCBValue(const size_t & strategy) const
{
	double totalTrials(0);
	for (size_t s(0); s<usableStrategies.size(); ++s)
	{
		totalTrials += results[usableStrategies[s]].first + results[usableStrategies[s]].second;
	}

	double C		= 0.7;
	double wins		= results[strategy].first;
	double trials	= results[strategy].first + results[strategy].second;

	double ucb = (wins / trials) + C * sqrt(std::log(totalTrials) / trials);

	return ucb;
}

const int StrategyManager::getScore(BWAPI::Player * player) const
{
	return player->getBuildingScore() + player->getKillScore() + player->getRazingScore() + player->getUnitScore();
}

const std::string StrategyManager::getOpeningBook() const
{
	if (selfRace == BWAPI::Races::Protoss)
	{
		return protossOpeningBook[currentStrategy];
	}
	else if (selfRace == BWAPI::Races::Terran)
	{
		return terranOpeningBook[currentStrategy];
	}
	else if (selfRace == BWAPI::Races::Zerg)
	{
		return zergOpeningBook[currentStrategy];
	} 

	// something wrong, return the protoss one
	return protossOpeningBook[currentStrategy];
}

// when do we want to defend with our workers?
// this function can only be called if we have no fighters to defend with
const int StrategyManager::defendWithWorkers()
{
	if (!Options::Micro::WORKER_DEFENSE)
	{
		return false;
	}

	// our home nexus position
	BWAPI::Position homePosition = BWTA::getStartLocation(BWAPI::Broodwar->self())->getPosition();;

	// enemy units near our workers
	int enemyUnitsNearWorkers = 0;

	// defense radius of nexus
	int defenseRadius = 300;

	// fill the set with the types of units we're concerned about
	BOOST_FOREACH (BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits())
	{
		// if it's a zergling or a worker we want to defend
		if (unit->getType() == BWAPI::UnitTypes::Zerg_Zergling || unit->getType().isWorker())
		{
			if (unit->getDistance(homePosition) < defenseRadius)
			{
				enemyUnitsNearWorkers++;
			}
		}
	}

	// if there are enemy units near our workers, we want to defend
	return enemyUnitsNearWorkers;
}

// called by combat commander to determine whether or not to send an attack force
// freeUnits are the units available to do this attack
const bool StrategyManager::doAttack(const std::set<BWAPI::Unit *> & freeUnits)
{
	int ourForceSize = (int)freeUnits.size();

	int numUnitsNeededForAttack = 1;

	bool doAttack  = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Dark_Templar) >= 1
					|| ourForceSize >= numUnitsNeededForAttack;

	if (doAttack)
	{
		firstAttackSent = true;
	}

	return doAttack || firstAttackSent;
}

const bool StrategyManager::expandProtossAirRush() const
{
	// if there is no place to expand to, we can't expand
	if (MapTools::Instance().getNextExpansion() == BWAPI::TilePositions::None)
	{
		return false;
	}

	int numNexus =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numScouts =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Scout);
	int frame =					BWAPI::Broodwar->getFrameCount();

	// if there are more than 10 idle workers, expand
	if (WorkerManager::Instance().getNumIdleWorkers() > 10)
	{
		return true;
	}

	// 2nd Nexus Conditions:
	//		We have 10 or more scouts
	//		It is past frame 12000
	if ((numNexus < 2) && (numScouts > 10 || frame > 12000))
	{
		return true;
	}

	// 3nd Nexus Conditions:
	//		We have 20 or more scouts
	//		It is past frame 18000
	if ((numNexus < 3) && (numScouts > 20 || frame > 18000))
	{
		return true;
	}

	if ((numNexus < 4) && (numScouts > 20 || frame > 23000))
	{
		return true;
	}

	if ((numNexus < 5) && (numScouts > 20 || frame > 28000))
	{
		return true;
	}

	if ((numNexus < 6) && (numScouts > 20 || frame > 33000))
	{
		return true;
	}

	return false;
}

const bool StrategyManager::expandProtossZealotRush() const
{
	// if there is no place to expand to, we can't expand
	if (MapTools::Instance().getNextExpansion() == BWAPI::TilePositions::None)
	{
		return false;
	}

	int numNexus =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numZealots =			BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Zealot);
	int frame =					BWAPI::Broodwar->getFrameCount();

	// if there are more than 10 idle workers, expand
	if (WorkerManager::Instance().getNumIdleWorkers() > 10)
	{
		return true;
	}

	// 2nd Nexus Conditions:
	//		We have 12 or more zealots
	//		It is past frame 7000
	if ((numNexus < 2) && (numZealots > 12 || frame > 9000))
	{
		return true;
	}

	// 3nd Nexus Conditions:
	//		We have 24 or more zealots
	//		It is past frame 12000
	if ((numNexus < 3) && (numZealots > 24 || frame > 15000))
	{
		return true;
	}

	if ((numNexus < 4) && (numZealots > 24 || frame > 21000))
	{
		return true;
	}

	if ((numNexus < 5) && (numZealots > 24 || frame > 26000))
	{
		return true;
	}

	if ((numNexus < 6) && (numZealots > 24 || frame > 30000))
	{
		return true;
	}

	return false;
}

const MetaPairVector StrategyManager::getBuildOrderGoal()
{
	if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Protoss)
	{
		if (getCurrentStrategy() == ProtossAirRush)
		{
			return getProtossAirRushBuildOrderGoal();
		}
		else if (getCurrentStrategy() == ProtossZealotRush)
		{
			return getProtossZealotRushBuildOrderGoal();
		}
		else if (getCurrentStrategy() == ProtossDarkTemplar)
		{
			return getProtossDarkTemplarBuildOrderGoal();
		}
		else if (getCurrentStrategy() == ProtossDragoons)
		{
			return getProtossDragoonsBuildOrderGoal();
		}

		// if something goes wrong, use zealot goal
		return getProtossZealotRushBuildOrderGoal();
	}
	else if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Terran)
	{
		return getTerranBuildOrderGoal();
	}
	else
	{
		return getZergBuildOrderGoal();
	}
}

const MetaPairVector StrategyManager::getProtossAirRushBuildOrderGoal() const
{
	// the goal to return
	MetaPairVector goal;

	int numScouts =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Scout);
	int numProbes =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Probe);
	int numNexusCompleted =		BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numFleetBeacon =		BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Fleet_Beacon);
	int numNexusAll =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numStargates =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Stargate);
	int numCannons =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon);

	int scoutsWanted = numScouts + numStargates;
	int stargatesWanted = (numStargates < 2*numNexusCompleted)? numStargates + 2: numStargates;
	int probesWanted = numProbes + 2*numNexusCompleted;
	int cannonsWanted = numCannons + 2;

	if (InformationManager::Instance().enemyHasCloakedUnits())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));
	
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observatory, 1));
		}
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
		}
	}
	else
	{
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observatory, 1));
		}

		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
		}
	}

	if (numScouts >= 10 || BWAPI::Broodwar->getFrameCount() > 11000)
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));
		if(BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Fleet_Beacon) == 0)
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Fleet_Beacon, 1)); // hack so we don't get 2.. lol
	}
	
	if (numNexusCompleted >= 2)
	{
		goal.push_back(MetaPair(BWAPI::UpgradeTypes::Protoss_Air_Weapons, 1));
	}

	if (numNexusCompleted >= 3)
	{
		goal.push_back(MetaPair(BWAPI::UpgradeTypes::Protoss_Air_Weapons, 1));
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 2));
	}

	if (numFleetBeacon > 0)
	{
		goal.push_back(MetaPair(BWAPI::UpgradeTypes::Gravitic_Thrusters, 1));
	}

	if (expandProtossAirRush())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Nexus, numNexusAll + 1));
	}

	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Scout,		 scoutsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Photon_Cannon, cannonsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Stargate,		 stargatesWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Probe,		 std::min(90, probesWanted)));

	return goal;
}

const MetaPairVector StrategyManager::getProtossDragoonsBuildOrderGoal() const
{
		// the goal to return
	MetaPairVector goal;

	int numDragoons =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Dragoon);
	int numProbes =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Probe);
	int numNexusCompleted =		BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numNexusAll =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numCyber =				BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core);
	int numCannon =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon);

	int dragoonsWanted = numDragoons > 0 ? numDragoons + 6 : 2;
	int gatewayWanted = 3;
	int probesWanted = numProbes + 6;

	if (InformationManager::Instance().enemyHasCloakedUnits())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));
	
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observatory, 1));
		}
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
		}
	}
	else
	{
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observatory, 1));
		}

		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
		}
	}

	if (numNexusAll >= 2 || numDragoons > 6 || BWAPI::Broodwar->getFrameCount() > 9000)
	{
		gatewayWanted = 6;
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));
	}

	if (numNexusCompleted >= 3)
	{
		gatewayWanted = 8;
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 2));
	}

	if (numNexusAll > 1)
	{
		probesWanted = numProbes + 6;
	}

	if (expandProtossZealotRush())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Nexus, numNexusAll + 1));
	}

	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Dragoon,	dragoonsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Gateway,	gatewayWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Probe,	std::min(90, probesWanted)));

	return goal;
}

const MetaPairVector StrategyManager::getProtossDarkTemplarBuildOrderGoal() const
{
	// the goal to return
	MetaPairVector goal;

	int numDarkTeplar =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Dark_Templar);
	int numDragoons =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Dragoon);
	int numProbes =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Probe);
	int numNexusCompleted =		BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numNexusAll =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numCyber =				BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core);
	int numCannon =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon);

	int darkTemplarWanted = 0;
	int dragoonsWanted = numDragoons + 6;
	int gatewayWanted = 3;
	int probesWanted = numProbes + 6;

	if (InformationManager::Instance().enemyHasCloakedUnits())
	{
		
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));
		
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observatory, 1));
		}
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
		}
	}

	if (numNexusAll >= 2 || BWAPI::Broodwar->getFrameCount() > 9000)
	{
		gatewayWanted = 6;
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));
	}

	if (numDragoons > 0)
	{
		goal.push_back(MetaPair(BWAPI::UpgradeTypes::Singularity_Charge, 1));
	}

	if (numNexusCompleted >= 3)
	{
		gatewayWanted = 8;
		dragoonsWanted = numDragoons + 6;
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
	}

	if (numNexusAll > 1)
	{
		probesWanted = numProbes + 6;
	}

	if (expandProtossZealotRush())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Nexus, numNexusAll + 1));
	}

	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Dragoon,	dragoonsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Gateway,	gatewayWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Dark_Templar, darkTemplarWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Probe,	std::min(90, probesWanted)));
	
	return goal;
}

const MetaPairVector StrategyManager::getProtossZealotRushBuildOrderGoal() const
{
	// the goal to return
	MetaPairVector goal;

	int numZealots =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Zealot);
	int numDragoons =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Dragoon);
	int numProbes =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Probe);
	int numNexusCompleted =		BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numNexusAll =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Nexus);
	int numCyber =				BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Cybernetics_Core);
	int numCannon =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon);

	int zealotsWanted = numZealots + 8;
	int dragoonsWanted = numDragoons;
	int gatewayWanted = 3;
	int probesWanted = numProbes + 4;

	if (InformationManager::Instance().enemyHasCloakedUnits())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Robotics_Facility, 1));
		
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Robotics_Facility) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observatory, 1));
		}
		if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Observatory) > 0)
		{
			goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
		}
	}

	if (numNexusAll >= 2 || BWAPI::Broodwar->getFrameCount() > 9000)
	{
		gatewayWanted = 6;
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Assimilator, 1));
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Cybernetics_Core, 1));
	}

	if (numCyber > 0)
	{
		dragoonsWanted = numDragoons + 2;
		goal.push_back(MetaPair(BWAPI::UpgradeTypes::Singularity_Charge, 1));
	}

	if (numNexusCompleted >= 3)
	{
		gatewayWanted = 8;
		dragoonsWanted = numDragoons + 6;
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Observer, 1));
	}

	if (numNexusAll > 1)
	{
		probesWanted = numProbes + 6;
	}

	if (expandProtossZealotRush())
	{
		goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Nexus, numNexusAll + 1));
	}

	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Dragoon,	dragoonsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Zealot,	zealotsWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Gateway,	gatewayWanted));
	goal.push_back(MetaPair(BWAPI::UnitTypes::Protoss_Probe,	std::min(90, probesWanted)));

	return goal;
}

const MetaPairVector StrategyManager::getTerranBuildOrderGoal() const
{
	// the goal to return
	std::vector< std::pair<MetaType, UnitCountType> > goal;

	int numMarines =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Terran_Marine);
	int numMedics =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Terran_Medic);
	int numWraith =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Terran_Wraith);

	int marinesWanted = numMarines + 12;
	int medicsWanted = numMedics + 2;
	int wraithsWanted = numWraith + 4;

	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Marine,	marinesWanted));

	return (const std::vector< std::pair<MetaType, UnitCountType> >)goal;
}

const MetaPairVector StrategyManager::getZergBuildOrderGoal() const
{
	// the goal to return
	std::vector< std::pair<MetaType, UnitCountType> > goal;
	
	int numMutas  =				BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Zerg_Mutalisk);
	int numHydras  =			BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Zerg_Hydralisk);

	int mutasWanted = numMutas + 6;
	int hydrasWanted = numHydras + 6;

	goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Zerg_Zergling, 4));
	//goal.push_back(std::pair<MetaType, int>(BWAPI::TechTypes::Stim_Packs,	1));

	//goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Medic,		medicsWanted));

	return (const std::vector< std::pair<MetaType, UnitCountType> >)goal;
}

 const int StrategyManager::getCurrentStrategy()
 {
	 return currentStrategy;
 }