#pragma once;

#include <Common.h>
#include "MicroManager.h"

class MicroManager;

class AirManager : public MicroManager
{
public:

	AirManager();
	~AirManager() {}
	void executeMicro(const UnitVector & targets);

	BWAPI::Unit * chooseTarget(BWAPI::Unit * airUnit, const UnitVector & targets, std::map<BWAPI::Unit *, int> & numTargeting);
	BWAPI::Unit * closestAirUnit(BWAPI::Unit * target, std::set<BWAPI::Unit *> & airUnitsToAssign);

	int getAttackPriority(BWAPI::Unit * airUnit, BWAPI::Unit * target);
	BWAPI::Unit * getTarget(BWAPI::Unit * airUnit, UnitVector & targets);

	void kiteTarget(BWAPI::Unit * airUnit, BWAPI::Unit * target);
};
