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
	int							numForge;
	SquadData					defSquad;
	BWAPI::Unit	*				batteryUnit;
	
	static const int			BASE_SHIFT = 200;
	static const int			RECHARGE_THRESHOLD = 25;
	static const int			BATTERY_THRESHOLD = 10;
	static const int			CANNON_LIMIT = 5;

	static const int			FRAME_START = 5000;
	static const int			BUILD_FREQ = 48;

public:

	BatteryStratManager();
	~BatteryStratManager() {};
	void				update(const std::set<BWAPI::Unit *> & defenseUnits);
	BWTA::Chokepoint *	getNearestChokepoint();
	void				customBuild(BWAPI::UnitType b);
};
