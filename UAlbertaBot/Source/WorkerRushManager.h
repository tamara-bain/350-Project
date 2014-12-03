#pragma once

#include "Common.h"
#include "micromanagement/MicroManager.h"
#include "micromanagement/MicroUtil.h"
#include "StrategyManager.h"

class WorkerRushManager {

	std::set<BWAPI::Unit *>     workerRushers;

	std::map<int, bool>         workerFleeing;
    std::map<int, bool>         workerMovingFromFleeing;
    std::map<int, bool>         combatReady;
    std::map<int, bool>         fleeingInRadius;

    std::map<int, int>          threatsInRadius;
    std::map<int, int>          combatReadyInRadius;
    BWAPI::Unit*                target;

	BWTA::BaseLocation *        enemyBase;
	BWTA::BaseLocation *        ourBase;
	BWTA::Region *              enemyRegion;
	BWAPI::Unit *               enemyGeyser;
    BWAPI::Unit *               leader;
    std::set<BWAPI::Unit *>     targets; 

    BWAPI::Unit*                getLeader();
    void                        move();

    void                        updateStates();
    bool                        tooDangerous(BWAPI::Unit * worker);
    bool                        readyForCombat(BWAPI::Unit * worker);
    bool                        attackIfEncounterBlockade();
    BWAPI::Unit *               getWorkerFarthestFromGeyser(BWAPI::Unit * worker);

    void                        flee(BWAPI::Unit * worker);

	BWAPI::Unit *               getWorkerRushTarget(BWAPI::Unit * worker);

	void                        smartMove(BWAPI::Unit * worker, BWAPI::Position targetPosition);
	void                        smartAttack(BWAPI::Unit * attacker, BWAPI::Unit * target);
	bool                        enemyInRadius(BWAPI::Unit * worker);
    
    bool                        fleeingWorkerInRadius(BWAPI::Unit * worker, BWAPI::Position pos);
	BWAPI::Unit *               getLastTarget(BWAPI::Unit * worker);
	BWAPI::Unit *               getAttacker(BWAPI::Unit * worker);
	void                        drawDebugData();
    int                         getAttackingEnemyWorkers(BWAPI::Unit * worker);
    int                         getNonFleeingWorkers(BWAPI::Unit * worker) ;
    BWAPI::Unit *               getFleeTarget(BWAPI::Unit * worker);
    BWAPI::Unit *               getMineralWorker(BWAPI::Unit * worker);
    bool                        isDoingWorkerStuff(BWAPI::Unit * unit);
    bool                        testPosition(BWAPI::Position pos);

	void                        moveLeader();
	void                        moveFollowers();
	void                        allInAttack();

	BWAPI::Unit *               getEnemyGeyser();

    void                        fillGroundThreats(std::vector<GroundThreat> & threats, BWAPI::Unit * worker);
    bool                        isValidFleePosition(BWAPI::Position pos, BWAPI::Unit * worker, double2 fleeVector);
    BWAPI::Position             calcFleePosition(const std::vector<GroundThreat> & threats, BWAPI::Unit * worker);
    double2				        getFleeVector(const std::vector<GroundThreat> & threats, BWAPI::Position pos);

public:

	WorkerRushManager();
	~WorkerRushManager() {};

	void                        update(const std::set<BWAPI::Unit *> & workerUnits);
};


namespace WorkerRushOptions
    {
	    const int MIN_HP = 16;
        const int SAFETY_THRESH = 1;
        const int THREAT_DIST = 175;
        const int ALLY_DIST = 200;
        const int STAY_ON_TARGET_RANGE = 10;
        const int BASE_ATTACK_RADIUS = 300;

        const double ALLY_AVOIDANCE_FACTOR = 2;
        const double RESOURCE_AVOIDANCE_FACTOR = 0.5;

        const int FLEE_ANGLE = 20;
        const int FLEE_MOVE_DIST = 56;
        const int MAKE_WAY_RADIUS = 70;

        const int AVOID_GEYSER = 50;
        const int AVOID_BASE = 70;
        const int AVOID_MINERAL = 16;

        const int CHECK_DIST = 64;
        const int CHECK_ANGLE = 45;

        const int RESOURCE_AVOIDANCE_RADIUS = 175;
        const int REGION_KITE_RADIUS = 400;
        const int BASE_KITE_RADIUS = 200;
    }
