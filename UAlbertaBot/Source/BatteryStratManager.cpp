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
	numForge = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Forge);
	flagBattery = false;
	batteryUnit = NULL;

	//start the custom building for battery strategy after a set time, checks every 2s for update to reduce load
	if (BWAPI::Broodwar->getFrameCount() >= 5000 && (BWAPI::Broodwar->getFrameCount() % 48 == 0))
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
		else if ((BWAPI::Broodwar->getFrameCount() % 720 == 0) && numCannons < 5 && numForge > 0)
		{
			customBuild(BWAPI::UnitTypes::Protoss_Photon_Cannon);
		}
	}

	//clear squads
	defSquad.clearSquadData();
	UnitVector newSquad;

	//manage defense units by either defending chokepoint or going to shield battery for recharge
	BOOST_FOREACH(BWAPI::Unit * found, defenseUnits) {
		if (found->getShields() < 25 && haveBattery && batteryUnit->getEnergy() > 10)
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
