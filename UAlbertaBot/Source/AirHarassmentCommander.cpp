#include "Common.h"
#include "AirHarassmentCommander.h"
#include "StrategyManager.h"

AirHarassmentCommander::AirHarassmentCommander() 
	: attacking(false)
	, foundEnemy(false)
	, attackSent(false) 
{
	
}

bool AirHarassmentCommander::squadUpdateFrame()
{
	return BWAPI::Broodwar->getFrameCount() % 24 == 0;
}

void AirHarassmentCommander::update(std::set<BWAPI::Unit *> unitsToAssign)
{
	if(squadUpdateFrame())
	{
		// clear all squad data
		squadData.clearSquadData();

		// assign the harassment squad
		assignHarassmentSquads(unitsToAssign);
		assignIdleSquads(unitsToAssign);
	}

	squadData.update();
}

void AirHarassmentCommander::assignIdleSquads(std::set<BWAPI::Unit *> & unitsToAssign)
{
	if (unitsToAssign.empty()) { return; }

	UnitVector combatUnits(unitsToAssign.begin(), unitsToAssign.end());
	unitsToAssign.clear();

	squadData.addSquad(Squad(combatUnits, SquadOrder(SquadOrder::Defend, BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation()), 1000, "Defend Idle")));
}

void AirHarassmentCommander::assignHarassmentSquads(std::set<BWAPI::Unit *> & unitsToAssign)
{
	if (unitsToAssign.empty()) { return; }

		assignBaseAssault(unitsToAssign);				// assault main base to maintain air superiority
		// fallback strategies
		assignAttackRegion(unitsToAssign);				// attack occupied enemy region
		assignAttackVisibleUnits(unitsToAssign);		// attack visible enemy units
		assignAttackKnownBuildings(unitsToAssign);		// attack known enemy buildings
		assignAttackExplore(unitsToAssign);				// attack and explore for unknown units

}

void AirHarassmentCommander::assignBaseAssault(std::set<BWAPI::Unit *> & unitsToAssign) {
	
	if (unitsToAssign.empty()) { return; }
	if (StrategyManager::Instance().doAirRush() == false) { return; }

	BWTA::BaseLocation * enemyBase = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());
	BWTA::Region * enemyRegion = enemyBase->getRegion();


	if (enemyBase && enemyRegion->getCenter().isValid()) 
	{
		UnitVector combatUnits(unitsToAssign.begin(), unitsToAssign.end());
		unitsToAssign.clear();

		squadData.addSquad(Squad(combatUnits, SquadOrder(SquadOrder::BaseAssault, enemyRegion->getCenter(), 1000, "Base Assault")));
	}

}

void AirHarassmentCommander::assignAttackRegion(std::set<BWAPI::Unit *> & unitsToAssign) 
{
	if (unitsToAssign.empty()) { return; }

	BWTA::Region * enemyRegion = getClosestEnemyRegion();

	if (enemyRegion && enemyRegion->getCenter().isValid()) 
	{
		UnitVector oppUnitsInArea, ourUnitsInArea;
		MapGrid::Instance().GetUnits(oppUnitsInArea, enemyRegion->getCenter(), 800, false, true);
		MapGrid::Instance().GetUnits(ourUnitsInArea, enemyRegion->getCenter(), 200, true, false);

		if (!oppUnitsInArea.empty())
		{
			UnitVector combatUnits(unitsToAssign.begin(), unitsToAssign.end());
			unitsToAssign.clear();

			squadData.addSquad(Squad(combatUnits, SquadOrder(SquadOrder::Attack, enemyRegion->getCenter(), 1000, "Attack Region")));
		}
	}
}

BWTA::Region * AirHarassmentCommander::getClosestEnemyRegion()
{
	BWTA::Region * closestEnemyRegion = NULL;
	double closestDistance = 100000;

	// for each region that our opponent occupies
	BOOST_FOREACH (BWTA::Region * region, InformationManager::Instance().getOccupiedRegions(BWAPI::Broodwar->enemy()))
	{
		double distance = region->getCenter().getDistance(BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation()));

		if (!closestEnemyRegion || distance < closestDistance)
		{
			closestDistance = distance;
			closestEnemyRegion = region;
		}
	}

	return closestEnemyRegion;
}

void AirHarassmentCommander::assignAttackVisibleUnits(std::set<BWAPI::Unit *> & unitsToAssign) 
{
	if (unitsToAssign.empty()) { return; }

	BOOST_FOREACH (BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits())
	{
		if (unit->isVisible())
		{
			UnitVector combatUnits(unitsToAssign.begin(), unitsToAssign.end());
			unitsToAssign.clear();

			squadData.addSquad(Squad(combatUnits, SquadOrder(SquadOrder::Attack, unit->getPosition(), 1000, "Attack Visible")));

			return;
		}
	}
}

void AirHarassmentCommander::assignAttackKnownBuildings(std::set<BWAPI::Unit *> & unitsToAssign) 
{
	if (unitsToAssign.empty()) { return; }

	FOR_EACH_UIMAP_CONST (iter, InformationManager::Instance().getUnitInfo(BWAPI::Broodwar->enemy()))
	{
		const UnitInfo ui(iter->second);
		if(ui.type.isBuilding())
		{
			UnitVector combatUnits(unitsToAssign.begin(), unitsToAssign.end());
			unitsToAssign.clear();

			squadData.addSquad(Squad(combatUnits, SquadOrder(SquadOrder::Attack, ui.lastPosition, 1000, "Attack Known")));
			return;	
		}
	}
}

void AirHarassmentCommander::assignAttackExplore(std::set<BWAPI::Unit *> & unitsToAssign) 
{
	if (unitsToAssign.empty()) { return; }

	UnitVector combatUnits(unitsToAssign.begin(), unitsToAssign.end());
	unitsToAssign.clear();

	squadData.addSquad(Squad(combatUnits, SquadOrder(SquadOrder::Attack, MapGrid::Instance().getLeastExplored(), 1000, "Attack Explore")));
}

BWAPI::Unit* AirHarassmentCommander::findClosestDefender(std::set<BWAPI::Unit *> & enemyUnitsInRegion, const std::set<BWAPI::Unit *> & units) 
{
	BWAPI::Unit * closestUnit = NULL;
	double minDistance = 1000000;

	BOOST_FOREACH (BWAPI::Unit * enemyUnit, enemyUnitsInRegion) 
	{
		BOOST_FOREACH (BWAPI::Unit * unit, units)
		{
			double dist = unit->getDistance(enemyUnit);
			if (!closestUnit || dist < minDistance) 
			{
				closestUnit = unit;
				minDistance = dist;
			}
		}
	}

	return closestUnit;
}

BWAPI::Position AirHarassmentCommander::getDefendLocation()
{
	return BWTA::getRegion(BWTA::getStartLocation(BWAPI::Broodwar->self())->getTilePosition())->getCenter();
}

void AirHarassmentCommander::drawSquadInformation(int x, int y)
{
	squadData.drawSquadInformation(x, y);
}
