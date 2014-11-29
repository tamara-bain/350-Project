#pragma once

#include "Common.h"
#include "micromanagement/MicroManager.h"
#include "micromanagement/MicroUtil.h"
#include "StrategyManager.h"
#include "base/BuildingManager.h"
#include "CombatCommander.h"
#include "InformationManager.h"
#include "Squad.h"
#include "SquadData.h"
#include "SquadOrder.h"

class BatteryStratManager {

	BWTA::Chokepoint *			wallChokepoint;
	bool						buildBattery;
	bool						haveBattery;
	bool						flagBattery;
	int							numCannons;
	SquadData					defSquad;
	BWAPI::Unit	*				batteryUnit;

public:

	BatteryStratManager();
	~BatteryStratManager() {};
	void				update(const std::set<BWAPI::Unit *> & defenseUnits);
	BWTA::Chokepoint *	getNearestChokepoint();
	void				workerScoutDefense();
	void				customBuild(BWAPI::UnitType b)
};
