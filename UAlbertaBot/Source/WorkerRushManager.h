/* Implements and manages Worker Rush Strategy */

#pragma once

#include "Common.h"
#include "micromanagement/MicroManager.h"
#include "micromanagement/MicroUtil.h"
#include "StrategyManager.h"

class WorkerRushManager {

    // core unit of worker
	std::set<BWAPI::Unit *>     workerRushers;

    // hold various states of rush workers
	std::map<int, bool>         workerFleeing;
    std::map<int, bool>         workerMovingFromFleeing;
    std::map<int, bool>         fleeingInRadius;
    std::map<int, bool>         combatReady;

    // holds counts of enemies vs friend targets nearby
    std::map<int, int>          threatsInRadius;
    std::map<int, int>          combatReadyInRadius;

    // target for focus fire
    BWAPI::Unit*                target;

    // various units used frequently by worker rush
	BWTA::BaseLocation *        enemyBase;
	BWTA::BaseLocation *        ourBase;
	BWTA::Region *              enemyRegion;
	BWAPI::Unit *               enemyGeyser;
    BWAPI::Unit *               leader;

    BWAPI::Unit*                getLeader();

    // move into the enemy base, following the leader
    void                        move();
    void                        moveLeader();
	void                        moveFollowers();
    bool                        attackIfEncounterBlockade();

    // updating and setting states
    void                        updateStates();
    bool                        tooDangerous(const BWAPI::Unit * worker);
    bool                        readyForCombat(const BWAPI::Unit * worker);
    bool                        enemyInRadius(const BWAPI::Unit * worker);
    bool                        fleeingWorkerInRadius(const BWAPI::Unit * worker,  const BWAPI::Position pos);
    int                         getAttackingEnemyWorkers(const BWAPI::Unit * worker);
    int                         getNonFleeingWorkers(const BWAPI::Unit * worker);

    // movement and attacking functions
    void                        allInAttack();
    void                        flee(BWAPI::Unit * worker);
    void                        smartMove(BWAPI::Unit * worker, BWAPI::Position targetPosition);
	void                        smartAttack(BWAPI::Unit * attacker, BWAPI::Unit * target);

    // targetting functions
    BWAPI::Unit *               getWorkerFarthestFromGeyser(const BWAPI::Unit * worker);
	BWAPI::Unit *               getWorkerRushTarget(const BWAPI::Unit * worker);
	BWAPI::Unit *               getLastTarget(const BWAPI::Unit * worker);
	BWAPI::Unit *               getAttacker(const BWAPI::Unit * worker);
    BWAPI::Unit *               getFleeTarget(const BWAPI::Unit * worker);
    BWAPI::Unit *               getMineralWorker(const BWAPI::Unit * worker);
    bool                        isDoingWorkerStuff(const BWAPI::Unit * unit);

    // flee logic
	BWAPI::Unit *               getEnemyGeyser();
    void                        fillGroundThreats(std::vector<GroundThreat> & threats, const BWAPI::Unit * worker);
    BWAPI::Position             calcFleePosition(const std::vector<GroundThreat> & threats, const BWAPI::Unit * worker);
    double2				        getFleeVector(const std::vector<GroundThreat> & threats, BWAPI::Position pos);
    bool                        isValidFleePosition(const BWAPI::Position pos, const BWAPI::Unit * worker, const double2 fleeVector);
    bool                        testPosition(const BWAPI::Position pos);

    void                        drawDebugData();

public:

	WorkerRushManager();
	~WorkerRushManager() {};

	void                        update(const std::set<BWAPI::Unit *> & workerUnits);
};


// various distances and thresholds in one place for easy tweaking
namespace WorkerRushOptions
    {
        // min HP before our workers attempt to flee
	    const int MIN_HP = 16;
        // modifiable threshold for the buffer of workers
        // before we consider a radius dangerous
        const int SAFETY_THRESH = 1;
        // radius in which we consider threats
        const int THREAT_DIST = 175;
        // radius in which we count ally units
        const int ALLY_DIST = 200;
        // distance we will travel to stay on the same target
        const int STAY_ON_TARGET_RANGE = 10;
        // radius near the base we prefer to attack
        const int BASE_ATTACK_RADIUS = 300;

        // weighting to vectors when avoiding allies
        const double ALLY_AVOIDANCE_FACTOR = 2;
        // weighting to vectors when avoiding resources
        const double RESOURCE_AVOIDANCE_FACTOR = 0.5;

        // angle we start fleeing at
        const int FLEE_ANGLE = 20;
        // distance we move when fleeing
        const int FLEE_MOVE_DIST = 56;
        // distance we move when making way for
        // fleeing workers
        const int MAKE_WAY_RADIUS = 70;

        // radius near base elements that we avoid
        const int AVOID_GEYSER = 50;
        const int AVOID_BASE = 70;
        const int AVOID_MINERAL = 16;

        // peripheral distances and angles we check
        // when seeing if a move is valid (so we don't enter a corner)
        const int CHECK_DIST = 64;
        const int CHECK_ANGLE = 45;

        // radius used when testing to determine the ideal zones
        // for kiting
        const int RESOURCE_AVOIDANCE_RADIUS = 175;
        const int REGION_KITE_RADIUS = 400;
        const int BASE_KITE_RADIUS = 200;
    }
