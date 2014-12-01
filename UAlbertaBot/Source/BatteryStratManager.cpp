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
	//if we have not setup the chokepoint do so once
	if (wallChokepoint == NULL)
	{
		wallChokepoint = getNearestChokepoint();
	}	
	
	numCannons = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Photon_Cannon);
	numForge = BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Protoss_Forge);
	flagBattery = false;
	batteryUnit = NULL;

	//start the custom building for battery strategy after a set time, checks every 2s for update to reduce load
	if (BWAPI::Broodwar->getFrameCount() >= FRAME_START && (BWAPI::Broodwar->getFrameCount() % BUILD_FREQ == 0))
	{
		BOOST_FOREACH(BWAPI::Unit * found, BWAPI::Broodwar->getAllUnits()) {
			if (found->getPlayer() == BWAPI::Broodwar->self())
			{
				//set unit/flags for built shield battery
				if (found->getType() == BWAPI::UnitTypes::Protoss_Shield_Battery && found->isCompleted())
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
		else if ((BWAPI::Broodwar->getFrameCount() % 720 == 0) && numCannons < CANNON_LIMIT && numForge > 0)
		{
			customBuild(BWAPI::UnitTypes::Protoss_Photon_Cannon);
		}
	}

	//clear squads
	defSquad.clearSquadData();
	UnitVector newSquad;

	//manage defense units by either defending chokepoint or going to shield battery for recharge
	BOOST_FOREACH(BWAPI::Unit * found, defenseUnits) {
		if (found->getShields() < RECHARGE_THRESHOLD && haveBattery)
		{
			if (batteryUnit->getEnergy() > BATTERY_THRESHOLD)
			{
				found->follow(batteryUnit);
			}
			else
			{
				newSquad.push_back(found);
			}			
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
	BWAPI::Position ourBase = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition();
	int newx = ourBase.x();
	int newy = ourBase.y();

	if (newx < (BWAPI::Broodwar->mapWidth() * TILE_SIZE) / 2)
	{
		newx += BASE_SHIFT;
	}
	else
	{
		newx -= BASE_SHIFT;
	}

	if (newy < (BWAPI::Broodwar->mapHeight() * TILE_SIZE) / 2)
	{
		newy += BASE_SHIFT;
	}
	else
	{
		newy -= BASE_SHIFT;
	}

	return BWTA::getNearestChokepoint(BWAPI::Position(newx,newy));
}

void BatteryStratManager::customBuild(BWAPI::UnitType b)
{
	BuildingManager::Instance().addBuildingTask(b, BWAPI::TilePosition(wallChokepoint->getCenter()));
}
