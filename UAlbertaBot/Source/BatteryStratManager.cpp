#include "Common.h"
#include "BatteryStratManager.h"


BatteryStratManager::BatteryStratManager()
{
	wallChokepoint = NULL;
	haveBattery = false;
	flagBattery = false;
	buildBattery = false;
	numCannons = 0;
	defSquad.clearSquadData();
	batteryUnit = NULL;
}

void BatteryStratManager::update(const std::set<BWAPI::Unit *> & defenseUnits) 
{	
	//if we have not setup the checkpoint do so once
	if (wallChokepoint == NULL)
	{
		wallChokepoint = getNearestChokepoint();		
	}	
	
	numCannons = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon);
	flagBattery = false;
	batteryUnit = NULL;

	//start the custom building for battery strategy after a set time, checks every 2s for update to reduce load
	if (BWAPI::Broodwar->getFrameCount() >= 6000 && (BWAPI::Broodwar->getFrameCount() % 48 == 0))
	{
		BOOST_FOREACH(BWAPI::Unit * found, BWAPI::Broodwar->getAllUnits()) {
			if (found->getPlayer() == BWAPI::Broodwar->self())
			{
				//set unit/flags for built shield battery
				if (found->getType() == BWAPI::UnitTypes::Protoss_Shield_Battery)
				{
					haveBattery = true;
					flagBattery = true;
					buildBattery = false;
					batteryUnit = found;
				}		
			}
		}
		//if no battery was found unset flag
		if (!flagBattery)
		{
			haveBattery = false;
		}

		//no battery built yet try to build near chokepoint
		if (!haveBattery && !buildBattery)
		{
			customBuild(BWAPI::UnitTypes::Protoss_Shield_Battery);
			buildBattery = true;
		}
		//build cannons near chokepoint every 30s until we have 5
		else if ((BWAPI::Broodwar->getFrameCount() % 720 == 0) && numCannons < 5)
		{
			customBuild(BWAPI::UnitTypes::Protoss_Photon_Cannon);
		}
	}

	//clear squads
	defSquad.clearSquadData();
	UnitVector newSquad;
	//manager worker defense if scout harassment
	workerScoutDefense();	

	//manage defense units by either defending chokepoint or going to shield battery for recharge
	BOOST_FOREACH(BWAPI::Unit * found, defenseUnits) {
		if (found->getShields() < 25 && haveBattery)
		{
			found->follow(batteryUnit);
		}
		else
		{
			newSquad.push_back(found);
		}
	}
	//set defense squad and update their squad manager
	defSquad.addSquad(Squad(newSquad, SquadOrder(SquadOrder::Defend, wallChokepoint->getCenter(), 1000, "Defend Idle")));
	defSquad.update();
}

BWTA::Chokepoint* BatteryStratManager::getNearestChokepoint()
{
	return BWTA::getNearestChokepoint(InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition());
}

void BatteryStratManager::customBuild(BWAPI::UnitType b)
{
	BuildingManager::Instance().addBuildingTask(b, BWAPI::TilePosition(wallChokepoint->getCenter()));
}

void BatteryStratManager::workerScoutDefense()
{
	BOOST_FOREACH(BWTA::Region * myRegion, InformationManager::Instance().getOccupiedRegions(BWAPI::Broodwar->self()))
	{
		BWAPI::Position regionCenter = myRegion->getCenter();
		if (!regionCenter.isValid())
		{
			continue;
		}

		// all of the enemy units in this region
		std::set<BWAPI::Unit *> enemyUnitsInRegion;
		BOOST_FOREACH(BWAPI::Unit * enemyUnit, BWAPI::Broodwar->enemy()->getUnits())
		{
			if (BWTA::getRegion(BWAPI::TilePosition(enemyUnit->getPosition())) == myRegion)
			{
				enemyUnitsInRegion.insert(enemyUnit);
			}
		}

		// special case: figure out if the only attacker is a worker, the enemy is scouting
		if (enemyUnitsInRegion.size() == 1 && (*enemyUnitsInRegion.begin())->getType().isWorker())
		{
			// the enemy worker that is attacking us
			BWAPI::Unit * enemyWorker = *enemyUnitsInRegion.begin();

			// get our worker unit that is mining that is closest to it
			BWAPI::Unit * workerDefender = WorkerManager::Instance().getClosestMineralWorkerTo(enemyWorker);
			// grab it from the worker manager
			WorkerManager::Instance().setCombatWorker(workerDefender);

			// put it into a unit vector
			UnitVector workerDefenseForce;
			workerDefenseForce.push_back(workerDefender);

			// make a squad using the worker to defend
			defSquad.addSquad(Squad(workerDefenseForce, SquadOrder(SquadOrder::Defend, regionCenter, 1000, "Get That Scout!")));
			return;
		}
	}
}
