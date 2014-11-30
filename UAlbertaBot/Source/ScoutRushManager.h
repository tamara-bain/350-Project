#pragma once

#include "Common.h"
#include "micromanagement/MicroManager.h"
#include "micromanagement/MicroUtil.h"
#include "StrategyManager.h"

class ScoutRushManager {

	int                         numWorkerScouts;
	std::set<BWAPI::Unit *>     workerScouts;
	std::set<BWAPI::Unit *>     attackers;
	std::map<int, bool>         scoutFleeing;
    std::map<int, bool>         scoutMovingFromFleeing;
	BWTA::BaseLocation *        enemyBase;
	BWTA::BaseLocation *        ourBase;
	BWTA::Region *              enemyRegion;
	BWAPI::Unit *               enemyGeyser;

	BWAPI::Unit *               getScoutRushTarget(BWAPI::Unit * scout);
	void                        smartMove(BWAPI::Unit * scout, BWAPI::Position targetPosition);
	void                        smartAttack(BWAPI::Unit * attacker, BWAPI::Unit * target);
	bool                        enemyInRadius(BWAPI::Unit * scout);
    
    bool                        fleeingScoutInRadius(BWAPI::Unit * scout, BWAPI::Position pos);
	BWAPI::Unit *               getLastTarget(BWAPI::Unit * scout);
	BWAPI::Unit *               getAttacker(BWAPI::Unit * scout);
	void                        drawDebugData();
    int                         getAttackingWorkers(BWAPI::Unit * scout);
    int                         getNonFleeingScouts(BWAPI::Unit * scout) ;
    BWAPI::Unit *               getFleeTarget(BWAPI::Unit * scout);
    BWAPI::Unit *               getMineralWorker(BWAPI::Unit * scout);
    bool                        isDoingWorkerStuff(BWAPI::Unit * unit);

	void                        moveLeader(BWAPI::Unit * leader);
	void                        moveFollowers(BWAPI::Unit * leader);
	void                        allInAttack();

	BWAPI::Unit *               getEnemyGeyser();

    void                        fillGroundThreats(std::vector<GroundThreat> & threats, BWAPI::Unit * scout);
    bool                        isValidFleePosition(BWAPI::Position pos, BWAPI::Unit * scout, double2 fleeVector);
    BWAPI::Position             calcFleePosition(const std::vector<GroundThreat> & threats, BWAPI::Unit * scout);
    double2				        getFleeVector(const std::vector<GroundThreat> & threats, BWAPI::Position pos);


public:

	ScoutRushManager();
	~ScoutRushManager() {};

	void                    update(const std::set<BWAPI::Unit *> & scoutUnits);
};