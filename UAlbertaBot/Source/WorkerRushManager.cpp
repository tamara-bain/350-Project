#include "Common.h"
#include "WorkerRushManager.h"
#include "InformationManager.h"


WorkerRushManager::WorkerRushManager() : enemyRegion(NULL), enemyBase(NULL), 
    enemyGeyser(NULL), ourBase(NULL), leader(NULL) { }

void WorkerRushManager::update(const std::set<BWAPI::Unit *> & workerUnits) {
	
    // save the location of our base if we haven't yet
	if (ourBase == NULL)
		ourBase = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self());

    // intialize our worker list and their states once we have the required number of workers
	if (workerUnits.size() == Options::Macro::WORKER_RUSH_COUNT) {

		BOOST_FOREACH (BWAPI::Unit * unit, workerUnits) {

			if (workerRushers.count(unit) == 0) {
                
				workerRushers.insert(unit);

                // set  initial states
                int id = unit -> getID();
				workerFleeing[id] = false;
                workerMovingFromFleeing[id] = false;
                combatReady[id] = true;

                // set initial radius data
                threatsInRadius[id] = 0;
                combatReadyInRadius[id] = 0;
                fleeingInRadius[id] = 0;
			}

		}
	}

	if (workerRushers.size() != 0) {

        // determine the region that the enemy is in if we haven't already
        if (enemyRegion == NULL) {
	        enemyBase = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());
	        enemyRegion = enemyBase ? enemyBase->getRegion() : NULL;
        }

        // grab a leader for all the workers to follow
        leader = getLeader();

        BWTA::Region * leaderRegion = NULL;
        if (leader != NULL) {
            BWAPI::TilePosition leaderTile(leader->getPosition());
	        BWTA::Region * leaderRegion = leaderTile.isValid() ? BWTA::getRegion(leaderTile) : NULL;
        }

        // while we're not in the enemy region, keep moving towards it
        // discount if fleeing in case we accidently flee outside the region
	    if (leaderRegion != NULL && leaderRegion != enemyRegion) {
            if (!attackIfEncounterBlockade())
                move();
        }
        else {
            updateStates();
	        allInAttack();
        }

        drawDebugData();
	}
}

BWAPI::Unit* WorkerRushManager::getLeader() {

    // Get a leader for our rushers to follow
	BWAPI::Unit * currentLeader = NULL;

	BOOST_FOREACH (BWAPI::Unit * unit, workerRushers) {
		if (unit==NULL || !unit->exists() || !unit->getPosition().isValid()) {
			continue;
        }
	    
        // if this worker isn't preoccupied then use it as our leader
        if (workerFleeing[unit->getID()] == false && workerMovingFromFleeing[unit->getID()] == false) {
			currentLeader = unit;
            break;
        }
	}

	return currentLeader;
}

void WorkerRushManager::move() {
	moveLeader();
	moveFollowers();
}

void WorkerRushManager::moveLeader() {
    if (leader == NULL)
	    return;

	// for each start location in the level
	if (!enemyRegion)
	{
		BOOST_FOREACH (BWTA::BaseLocation * startLocation, BWTA::getStartLocations()) 
		{
			// if we haven't explored it yet
			if (!BWAPI::Broodwar->isExplored(startLocation->getTilePosition())) 
			{
				smartMove(leader, BWAPI::Position(startLocation->getTilePosition()));			
				return;
			}
		}
	} else {
		smartMove(leader, enemyBase->getPosition());	
	}
}

void WorkerRushManager::moveFollowers() {
    if (leader == NULL)
	    return;

	BWAPI::UnitCommand leaderCommand(leader->getLastCommand());
	BWAPI::Position leaderPosition(leaderCommand.getTargetPosition());

	// if our leader isn't moving neither are we
	if (leaderCommand.getType() != BWAPI::UnitCommandTypes::Move) {
		return;
	}

	BOOST_FOREACH (BWAPI::Unit * unit, workerRushers) {
		if (unit == leader)
			continue;

		if (!unit || !unit->exists() || !unit->getPosition().isValid()) {
			continue;
		}

		// if we have issued a command to this unit already this frame, ignore this one
		if (unit->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount())
			continue;

		// get the unit's current command
		BWAPI::UnitCommand currentCommand(unit->getLastCommand());

		// if we've already told this unit to move to the leaders position, ignore this command
		if (currentCommand.getType() == BWAPI::UnitCommandTypes::Move 
			&& currentCommand.getTargetPosition() == leaderPosition) {
			continue;
		}

		// follow the leader
		unit->issueCommand(leaderCommand);
	}
}

bool WorkerRushManager::attackIfEncounterBlockade() {
    BWAPI::Unit * target = NULL;

    // If we encounter an nonmoving target in range of us on the way to the base
    // then attack it
    BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) {
        BOOST_FOREACH(BWAPI::Unit * rusher, workerRushers) {
            if (!unit->getType().isWorker())
                continue;

            int range = rusher->getType().groundWeapon().maxRange();

		    if (unit->getDistance(rusher) < range + WorkerRushOptions::STAY_ON_TARGET_RANGE &&
                unit->isMoving() == false) {

			    target = unit;
                break;
            }
        }
    }
    
    if (target != NULL) {
        BOOST_FOREACH(BWAPI::Unit * rusher, workerRushers) {
            smartAttack(rusher, target);
        }
        return true;
    }
    return false;
}

void WorkerRushManager::updateStates() {

    // find out which workers are being attacked
    // and what their surroundings are like
    target = NULL;
	BOOST_FOREACH (BWAPI::Unit * unit, workerRushers) {

        int id = unit -> getID();

        // update radius data
        threatsInRadius[id] = getAttackingEnemyWorkers(unit);
        combatReadyInRadius[id] = getNonFleeingWorkers(unit);
        fleeingInRadius[id] = fleeingWorkerInRadius(unit, unit->getPosition());


        // if there are more drones attacking us then their are combat ready
        // allies near us then we should start fleeing
        if (unit->isUnderAttack() && tooDangerous(unit)) {
            workerFleeing[id] = true;
        }

        // otherwise save their attacker as a target
        // and flee only if our health gets low
		else if (unit->isUnderAttack()) {

            target = getAttacker(unit);

            // we want to back out if the number of attacking enemies in our radius
            // could kill us if they focus fired
			if (unit->getHitPoints() + unit->getShields() < WorkerRushOptions::MIN_HP) {
				workerFleeing[id] = true;
			}
		}

        // otherwise if we have allies near us that are trying to flee
        // set us as moving out of their way
        if(fleeingInRadius[id]) 
            workerMovingFromFleeing[id] = true;
        else 
            workerMovingFromFleeing[id] = false;

        // stop fleeing if there are no longer any threats in our radius
		if ( (threatsInRadius[id] == 0) || 
                (unit->getHitPoints()) + unit->getShields() > (WorkerRushOptions::MIN_HP)
                    && threatsInRadius[id] < combatReadyInRadius[id]) {
			workerFleeing[id] = false;
		}

        // set us a combat ready and change our attack to 'first'
        if(readyForCombat(unit)) {
            combatReady[id] = true;
        }
	}
}

bool WorkerRushManager::tooDangerous(BWAPI::Unit * worker) {
    int id = worker -> getID();

    if (threatsInRadius[id] > combatReadyInRadius[id] + WorkerRushOptions::SAFETY_THRESH)
        return true;

    return false;
}

bool WorkerRushManager::readyForCombat(BWAPI::Unit * worker) {
    int id = worker -> getID();

    if (!workerFleeing[id] && !workerMovingFromFleeing[id])
        return true;

    return false;
}

void WorkerRushManager::allInAttack() {

    leader = getLeader();

    if (leader != NULL && target == NULL)
        target = getWorkerRushTarget(leader);

    // move workers that are blocking the way first
    BOOST_FOREACH (BWAPI::Unit * unit, workerRushers) {
        int id = unit -> getID();
        if (workerMovingFromFleeing[id]) {
            flee(unit);
            continue;
        }
    }

    // then move those that are trying to get away
    BOOST_FOREACH (BWAPI::Unit * unit, workerRushers) {
        int id = unit -> getID();
        if (workerFleeing[id] && workerMovingFromFleeing[id] == false) {
            flee(unit);
            continue;
        }
    }

    // finally move the rest
	BOOST_FOREACH (BWAPI::Unit * unit, workerRushers) {
        int id = unit -> getID();
        if (workerFleeing[id] || workerMovingFromFleeing[id])
            continue;

		// attack a target if one exists
		if (target != NULL && target->exists() && target->getPosition().isValid()) {
			smartAttack(unit, target);
			continue;
		}

		// if we've reached the enemy base with no target
		if (unit->getDistance(enemyBase->getPosition()) < 20) {
			WorkerManager::Instance().finishedWithRushWorker(unit);
			continue;
		}

		// otherwise keep moving into the enemy base
		smartMove(unit, enemyBase->getPosition());
	}
}

void WorkerRushManager::flee(BWAPI::Unit * worker) {
    // figure out where to move to
    std::vector<GroundThreat> groundThreats;
    fillGroundThreats(groundThreats, worker);
    BWAPI::Position fleeTo = calcFleePosition(groundThreats, worker);

    // excute move command
    smartMove(worker, fleeTo);
}

int WorkerRushManager::getAttackingEnemyWorkers(BWAPI::Unit * worker) {
    
    int attackingWorkers = 0;
    
    BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) {

        // if they're not in the enemy region we don't care or they're a worker doing workery things we don't care
        if (worker->getDistance(unit->getPosition()) > WorkerRushOptions::THREAT_DIST)
            continue;

        // add any worker that is not doing workery stuff (building/repairing/gathering)
        // as someone who is potentially coming to attack us
        if (unit->getType().isWorker() && !(isDoingWorkerStuff(unit)))
		    ++attackingWorkers;
    }

    return attackingWorkers;
}

int WorkerRushManager::getNonFleeingWorkers(BWAPI::Unit * worker) {
    int nonFleeingWorkers = 0;
    
    BOOST_FOREACH(BWAPI::Unit * ally, workerRushers) {

        // if they're not able to engage in combat
        // or if they're too far away than don't count them
        if (!readyForCombat(ally) || 
            worker->getDistance(ally->getPosition()) > WorkerRushOptions::ALLY_DIST)
            continue;

        nonFleeingWorkers++;
    }

    return nonFleeingWorkers;
}


BWAPI::Unit * WorkerRushManager::getAttacker(BWAPI::Unit * worker) {

	BWAPI::Position position = worker->getPosition();

    // if the previous target of the worker is attacking, then stay on target
	// we don't want to keep switching to the newest attacker
	BWAPI::Unit * currentTarget = getLastTarget(worker);
	if (currentTarget != NULL && (currentTarget->isAttacking() || currentTarget->isRepairing()))
		return currentTarget;

	// for each enemy worker find the closest or one in weapons range
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) {
		if ((unit->isAttacking() || unit->isRepairing()) && worker->isInWeaponRange(unit)) {
			currentTarget = unit;
			break;
		}
	}

	return currentTarget;
}

BWAPI::Unit * WorkerRushManager::getLastTarget(BWAPI::Unit * worker) {

	BWAPI::Position position = worker->getPosition();
	int range = worker->getType().groundWeapon().maxRange();

    // if the last command was an attack try to get the last target
	BWAPI::UnitCommand lastCommand(worker->getLastCommand());

	if (lastCommand.getType() == BWAPI::UnitCommandTypes::Attack_Unit) {

		BWAPI::Unit * target = lastCommand.getTarget();

		// we're only interested in staying on target if the last target was a valid enemy unit
		if (target != NULL && target->exists() && target->getPosition().isValid() && target->getType().canMove()) {
           if (tooDangerous(worker) && !isDoingWorkerStuff(target))
                return NULL;

			// only stay on target if the unit stays within range of one of us
            BOOST_FOREACH(BWAPI::Unit * rusher, workerRushers) {
			    if (target->getDistance(rusher) < range + WorkerRushOptions::STAY_ON_TARGET_RANGE)
				    return target;
            }
		}
	}

	return NULL;
}

BWAPI::Unit * WorkerRushManager::getWorkerRushTarget(BWAPI::Unit * worker)
{
    BWAPI::Unit * closestEnemyWorker = NULL;

    if (!worker || !worker->exists() || !worker->getPosition().isValid())
        return NULL;

    int lowestHealth = 100000;

    if (closestEnemyWorker != NULL)
        return closestEnemyWorker;

	// try to get the last target
	closestEnemyWorker = getLastTarget(worker);

    // if we have a previous target that is still close, return that one
	if (closestEnemyWorker != NULL)
		return closestEnemyWorker;

	// return the closest worker or building if no worker is found
	
	double closestDist = 100000;
    double closestDistOther = 100000;
    BWAPI::Unit * closestEnemyOther = NULL;
	BWAPI::Position position = worker->getPosition();
	int range = worker->getType().groundWeapon().maxRange();


	// for each enemy worker find the closest one
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) {

		double dist = unit->getDistance(position);
		double distToBase = unit -> getDistance(enemyBase->getPosition());

        // we don't care about ones that are really far from base
		if (distToBase > WorkerRushOptions::BASE_ATTACK_RADIUS)
			continue;

        if (unit->getType().isWorker()) {

            // don't attack workers who aren't doing workery stuff
            // if the environment is too dangerous, they might be on their
            // way to attack us
            if (tooDangerous(worker) && !isDoingWorkerStuff(unit))
                continue;

            int unitHealth = unit->getHitPoints() + unit->getShields();

            // otherwise record the closest enemy outside of our range
            // or the lowest health enemy in range
			if (!closestEnemyWorker || (dist < closestDist && dist > range) || 
                (dist <= range && unitHealth < lowestHealth)) {
				closestEnemyWorker = unit;
				closestDist = dist;
                lowestHealth = unitHealth;
			}
        }
        // otherwise at this stage of the game it's a building so ignore it
		else if (!closestEnemyWorker) {
            if ((!closestEnemyOther || dist < closestDistOther) && !(unit->isLifted()) 
                && unit->getType() != BWAPI::UnitTypes::Zerg_Larva && unit->getType() != BWAPI::UnitTypes::Zerg_Egg) {
				closestEnemyOther = unit;
                closestDistOther = dist;
			}
		}
	}

    // return the closest worker or building if no worker is found
	if (closestEnemyWorker != NULL)
		return closestEnemyWorker;

	return closestEnemyOther;
}

BWAPI::Unit * WorkerRushManager::getMineralWorker(BWAPI::Unit * worker)
{
	// if the last command was an attack try to get the last target
	BWAPI::Unit * closestEnemyWorker = getLastTarget(worker);

    if (closestEnemyWorker != NULL && closestEnemyWorker->isGatheringMinerals())
		return closestEnemyWorker;

	// return the closest mineral gathering worker
	BWAPI::Unit * closestEnemyOther = NULL;
	double closestDist = 100000;
	BWAPI::Position position = worker->getPosition();
	int range = worker->getType().groundWeapon().maxRange();

	// for each enemy worker find the closest or one in weapons range
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) {

        double dist = unit->getDistance(position);

        if (unit->isGatheringMinerals()) {

			// if an enemy worker is in range we don't have to search anymore
			if (dist <= range)
				return unit;

			if (!closestEnemyWorker || dist < closestDist) {
				closestEnemyWorker = unit;
				closestDist = dist;
			}
		}
	}

	return closestEnemyWorker;
}

bool WorkerRushManager::isDoingWorkerStuff(BWAPI::Unit * unit) {
    if (unit->isGatheringMinerals() || unit->isGatheringGas() || unit->isConstructing())
        return true;
    return false;
}

BWAPI::Unit * WorkerRushManager::getEnemyGeyser()
{
	// return the geyser if we already have it's location
	if (enemyGeyser != NULL)
		return enemyGeyser;

	// return the position of they geyser
	BOOST_FOREACH (BWAPI::Unit * unit, enemyBase->getGeysers()) {
		enemyGeyser = unit;
		break;
	}

	if (enemyGeyser == NULL) {
		return NULL;
	}
	return enemyGeyser;
}

bool WorkerRushManager::enemyInRadius(BWAPI::Unit * worker)
{
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) {

		if (unit->getType().canAttack() && (unit->getDistance(worker) < 175)
            && !isDoingWorkerStuff(unit)) {
			return true;
		}
	}

	return false;
}

bool WorkerRushManager::fleeingWorkerInRadius(BWAPI::Unit * worker, BWAPI::Position pos)
{
	BWAPI::Unit * target = NULL;

	BOOST_FOREACH(BWAPI::Unit * unit, workerRushers) {

		if (unit != worker && unit->getDistance(worker) < WorkerRushOptions::MAKE_WAY_RADIUS && workerFleeing[unit->getID()]) {
			return true;
		}
	}

	return false;
}

void WorkerRushManager::fillGroundThreats(std::vector<GroundThreat> & threats, BWAPI::Unit * worker)
{

    BOOST_FOREACH(BWAPI::Unit* ally, workerRushers) {
        if (workerFleeing[ally->getID()] && ally != worker 
            && ally->getDistance(worker) > WorkerRushOptions::ALLY_AVOIDANCE_FACTOR) {
		    GroundThreat threat;
		    threat.unit		= ally;
		    threat.weight	= WorkerRushOptions::ALLY_AVOIDANCE_FACTOR;
		    threats.push_back(threat);
        }
    }
   
    if (workerMovingFromFleeing[worker->getID()])
        return;

    // get the ground threats in the enemy region
	BOOST_FOREACH (BWAPI::Unit * enemy, BWAPI::Broodwar->enemy()->getUnits())
	{
		// if they're not in the enemy region we don't care or they're a worker doing workery things we don't care
        if (worker->getDistance(enemy->getPosition()) > WorkerRushOptions::THREAT_DIST)
            continue;

		// get the weapon of the unit
		BWAPI::WeaponType groundWeapon(enemy->getType().groundWeapon());

		// weight the threat based on the highest DPS
		if(groundWeapon == BWAPI::WeaponTypes::None)
            continue;

		GroundThreat threat;
		threat.unit		= enemy;
		threat.weight	= (static_cast<double>(groundWeapon.damageAmount()) / groundWeapon.damageCooldown());;
		threats.push_back(threat);
	}
}

void WorkerRushManager::smartMove(BWAPI::Unit * worker, BWAPI::Position targetPosition)
{
    if (!worker || !(worker->exists()))
        return;
	// if we have issued a command to this unit already this frame, ignore this one
	if (worker->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount())
		return;

    //if (Options::Debug::DRAW_WORKER_RUSH_DEBUG) BWAPI::Broodwar->drawLineMap(worker->getPosition().x(), worker->getPosition().y(), targetPosition.x(), targetPosition.y(), BWAPI::Colors::Yellow);
	// if nothing prevents it then move
	worker->move(targetPosition);
}

void WorkerRushManager::smartAttack(BWAPI::Unit * attacker, BWAPI::Unit * target)
{
	// if we have issued a command to this unit already this frame, ignore this one
	if (attacker->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount())
		return;

    // don't interrupt attack that is currently occuring
    if (attacker->isAttackFrame()) {
        return;
    }

	// attack the target
	attacker->attack(target);
}


BWAPI::Position WorkerRushManager::calcFleePosition(const std::vector<GroundThreat> & threats, BWAPI::Unit * worker) 
{
	// calculate the standard flee vector
    BWAPI::Position pos = worker->getPosition();


    int mult = 1;
    if (worker->getID() % 2)
        mult = -1;

    double2 fleeVector(0,0);
    fleeVector = getFleeVector(threats, pos);


	double2 newFleeVector;
	int iterations = 0;

    // offset non fleeing workers at a slight angle
    // so they get out of the way faster
    int r = WorkerRushOptions::FLEE_ANGLE;
    if (workerFleeing[worker->getID()])
        r = 5;

    int move_distance = WorkerRushOptions::FLEE_MOVE_DIST;

	// keep rotating the vector until the new position is able to be walked on
	while (iterations < 73) 
	{
		// rotate the flee vector
        newFleeVector = fleeVector;
        newFleeVector.rotate(mult*r);


		// re-normalize it
		newFleeVector.normalise();

		// the position we will attempt to go to
		BWAPI::Position test(pos + newFleeVector * WorkerRushOptions::FLEE_MOVE_DIST);
        if (iterations == 1 && Options::Debug::DRAW_WORKER_RUSH_DEBUG)
            BWAPI::Broodwar->drawLineMap(worker->getPosition().x(), worker->getPosition().y(), test.x(), test.y(), BWAPI::Colors::Yellow);
		// if the position is able to be walked on, break out of the loop
		if (isValidFleePosition(test, worker, newFleeVector)) {
            BWAPI::Broodwar->drawLineMap(worker->getPosition().x(), worker->getPosition().y(), test.x(), test.y(), BWAPI::Colors::Green);
			return test;
        }

        if (iterations == 72)
            BWAPI::Broodwar->printf("No move found.");

        if (r >=0)
            r = -(r+5);
        else
            r = -r;

        ++iterations;
	}

	// go to the calculated 'good' position
	BWAPI::Position fleeTo(pos + fleeVector);
	
	return fleeTo;
}

double2 WorkerRushManager::getFleeVector(const std::vector<GroundThreat> & threats, BWAPI::Position pos)
{
	double2 fleeVector(0,0);

	BOOST_FOREACH (const GroundThreat & threat, threats)
	{
		// Get direction from enemy
		const double2 direction(pos - threat.unit->getPosition());

		const double distanceSq(direction.lenSq());
		if(distanceSq > 0)
		{
			// Enemy influence is direction to flee from enemy weighted by danger posed by enemy
			const double2 enemyInfluence( (direction / distanceSq) * threat.weight );
			fleeVector = fleeVector + enemyInfluence;
		}
	}

	if(fleeVector.lenSq() == 0 || threats.size()==0) {
		// Flee towards our base
		fleeVector = double2(ourBase->getPosition().x(),ourBase->getPosition().y());	
	}

	fleeVector.normalise();

	return fleeVector;
}

bool WorkerRushManager::isValidFleePosition(BWAPI::Position pos, BWAPI::Unit * worker, double2 fleeVector) {

    // if it's not walkable throw it out
	if (!BWAPI::Broodwar->isWalkable(pos.x()/8, pos.y()/8)) 
        return false;

    // check the attempted position
    BWAPI::TilePosition tile(pos);

    std::set<BWAPI::Unit*> occupiers = BWAPI::Broodwar->getUnitsOnTile(tile.x(), tile.y());
	// don't go anywhere that is occupied (but we'' try to force our way through for workers)
	BOOST_FOREACH(BWAPI::Unit * unit, occupiers) {
        if	(!unit->isLifted() && !unit->getType().isFlyer() && !unit->getType().isWorker())
		    return false;
    }

    if(occupiers.size() > 2) {
        return false;
    }

    // stay away from base;
    BWAPI::Position base_pos = enemyBase->getPosition();
    double dist_base = base_pos.getDistance(pos);

	if (dist_base < WorkerRushOptions::AVOID_BASE) {
		return false;
    }
    
    BWTA::Polygon poly = enemyRegion->getPolygon();
    BWAPI::Position center = poly.getCenter();
    double dist_center = center.getDistance(pos);

    BWTA::BaseLocation * enemyLocation = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());
    BWAPI::Unit * geyser = getEnemyGeyser();
    // stay away from geysers
	if (geyser->getDistance(worker->getPosition()) > WorkerRushOptions::AVOID_GEYSER && geyser->getDistance(pos) < WorkerRushOptions::AVOID_GEYSER)
		return false;

    // stay away from dangerous minerals
	BOOST_FOREACH (BWAPI::Unit * mineral, enemyLocation->getMinerals()) {
		if (mineral->getDistance(pos) < WorkerRushOptions::AVOID_MINERAL)
			return false;
    }
    

   
    // If we're outside of our desired radius for our center region, then don't move any further away
   // if (dist_center > WorkerRushOptions::REGION_KITE_RADIUS && dist_base > WorkerRushOptions::BASE_KITE_RADIUS) {
    //    if (dist_center > worker->getPosition().getDistance(center))
    //        return false;
    //}

    // check area around the worker, we don't want it going into spaces that are too tight
    double2 r1(fleeVector);
    double2 r2(fleeVector);

    r1.rotate(WorkerRushOptions::CHECK_ANGLE);
    r2.rotate(-WorkerRushOptions::CHECK_ANGLE);

    r1.normalise();
    r2.normalise();

    int checkRadius = WorkerRushOptions::FLEE_MOVE_DIST;
    if (enemyBase->getPosition().getDistance(pos) > WorkerRushOptions::BASE_KITE_RADIUS) {
        checkRadius = WorkerRushOptions::CHECK_DIST;
    }

    double angleDistance = (checkRadius/cos((double)WorkerRushOptions::CHECK_ANGLE));
    BWAPI::Position test1(worker->getPosition()  + r1 * angleDistance);
    BWAPI::Position test2(worker->getPosition()  + r2 * angleDistance);
    BWAPI::Position test3(worker->getPosition()  + fleeVector * checkRadius);

    if (Options::Debug::DRAW_WORKER_RUSH_DEBUG) {
        BWAPI::Broodwar->drawCircleMap(worker->getPosition().x(), worker->getPosition().y(), checkRadius, BWAPI::Colors::Red, false);
    }

    // check the position and the positions around our worker
    if (testPosition(pos) && test3 && testPosition(test1) && testPosition(test2))
        return true;

    // debug vectors that failed our tests
    if (Options::Debug::DRAW_WORKER_RUSH_DEBUG) {
        BWAPI::Broodwar->drawLineMap(worker->getPosition().x(), worker->getPosition().y(), test1.x(), test1.y(), BWAPI::Colors::Red);
        BWAPI::Broodwar->drawLineMap(worker->getPosition().x(), worker->getPosition().y(), test2.x(), test2.y(), BWAPI::Colors::Red);
        BWAPI::Broodwar->drawLineMap(worker->getPosition().x(), worker->getPosition().y(), test3.x(), test2.y(), BWAPI::Colors::Red);
    }

    return false;
}

bool WorkerRushManager::testPosition(BWAPI::Position pos) {
    
    if (!pos.isValid())
        return false;

    // stay within the enemy region
    if (BWTA::getRegion(BWAPI::TilePosition(pos)) != enemyRegion)
		return false;

	// otherwise it's okay
	return true;
}

void WorkerRushManager::drawDebugData() {

    if (!Options::Debug::DRAW_WORKER_RUSH_DEBUG) 
        return;

    BOOST_FOREACH (BWAPI::Unit * worker, workerRushers) {
        int x = worker->getPosition().x();
        int y = worker->getPosition().y();
        int id = worker->getID();

        BWAPI::Unit * target = worker->getTarget();
	   
        // draw target
        if(target) {
            int tx = target->getPosition().x();
            int ty = target->getPosition().y();

            if (Options::Debug::DRAW_WORKER_RUSH_DEBUG) BWAPI::Broodwar->drawLineMap(x, y, tx, ty, BWAPI::Colors::Purple);
        }

        // draw state
        if (worker -> isStuck()) {
             BWAPI::Broodwar->drawTextMap(x, y, "Stuck!!!");
        }

	    else if(workerFleeing[id]) {
	        BWAPI::Broodwar->drawTextMap(x, y, "Fleeing!");
            BWAPI::Broodwar->drawCircleMap(x, y, WorkerRushOptions::THREAT_DIST, BWAPI::Colors::Purple, false);
        }

        else if(workerMovingFromFleeing[id]) {
	        BWAPI::Broodwar->drawTextMap(x, y, "Moving!");
            BWAPI::Broodwar->drawCircleMap(x, y, WorkerRushOptions::MAKE_WAY_RADIUS, BWAPI::Colors::Red, false);
        }

        else if(combatReady[id]) {
	        BWAPI::Broodwar->drawTextMap(x, y, "Ready!");
            BWAPI::Broodwar->drawCircleMap(x, y, WorkerRushOptions::ALLY_DIST, BWAPI::Colors::Green, false);
        }

        // draw health and sheild bars (courtesy of XIMPBOT)
        BWAPI::Position maxBar =  BWAPI::Position((static_cast<int>((double)TILE_SIZE*0.7)), TILE_SIZE/10);
        BWAPI::Position barPosition = BWAPI::Position(x-maxBar.x()/2, y-maxBar.y()/2);
        int currentHP = static_cast<int>(((double)(worker->getHitPoints()) / (double)(worker->getType().maxHitPoints())) * maxBar.x());
        
        BWAPI::Broodwar->drawBoxMap(barPosition.x(), barPosition.y(), barPosition.x()+maxBar.x(), barPosition.y()+maxBar.y(), BWAPI::Colors::Red, true);
        BWAPI::Broodwar->drawBoxMap(barPosition.x(), barPosition.y(), barPosition.x()+currentHP, barPosition.y()+maxBar.y(), BWAPI::Colors::Green, true);

        if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Protoss) {
            int currentShields = static_cast<int>(((double)(worker->getShields()) / (double)(worker->getType().maxShields())) * maxBar.x());

            BWAPI::Broodwar->drawBoxMap(barPosition.x(), barPosition.y()-maxBar.y(), barPosition.x()+maxBar.x(), barPosition.y(), BWAPI::Colors::Grey, true);
            BWAPI::Broodwar->drawBoxMap(barPosition.x(), barPosition.y()-maxBar.y(), barPosition.x()+currentShields, barPosition.y(), BWAPI::Colors::Blue, true);
        }
    }

    // draw mineral radius
    BWTA::BaseLocation * enemyLocation = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());
	BOOST_FOREACH (BWAPI::Unit * mineral, enemyLocation->getMinerals()) {
        BWAPI::Broodwar->drawCircleMap(mineral->getPosition().x(), mineral->getPosition().y(), WorkerRushOptions::AVOID_MINERAL, BWAPI::Colors::Red);
    }
        
    // draw geyser radius
    BWAPI::Broodwar->drawCircleMap(getEnemyGeyser()->getPosition().x(), getEnemyGeyser()->getPosition().y(), WorkerRushOptions::AVOID_GEYSER, BWAPI::Colors::Red);
        
    // draw base radius
    BWAPI::Broodwar->drawCircleMap(enemyBase->getPosition().x(), enemyBase->getPosition().y(), WorkerRushOptions::AVOID_BASE, BWAPI::Colors::Red);

    
    // draw outlines of enemy region
    BWTA::Polygon poly = enemyRegion->getPolygon();
    BWAPI::Position center = poly.getCenter();

    BWAPI::Broodwar->drawCircleMap(center.x(), center.y(), WorkerRushOptions::REGION_KITE_RADIUS, BWAPI::Colors::Red);
    BWAPI::Broodwar->drawCircleMap(enemyBase->getPosition().x(), enemyBase->getPosition().y(), WorkerRushOptions::BASE_KITE_RADIUS, BWAPI::Colors::Red);

    for (int j = 0; j < (int)poly.size(); ++j) {
        BWAPI::Position point1 = poly[j];
        BWAPI::Position point2 = poly[(j+1) % poly.size()];   

        BWAPI::Broodwar->drawLine(BWAPI::CoordinateType::Map, point1.x(), point1.y(), point2.x(), point2.y(), BWAPI::Colors::Green);
        BWAPI::Broodwar->drawLine(BWAPI::CoordinateType::Map, point1.x(), point1.y(), point2.x(), point2.y(), BWAPI::Colors::Green);
    }

    BOOST_FOREACH (BWTA::Chokepoint * chokepoint, enemyRegion->getChokepoints()) {
        BWAPI::Position point1 = chokepoint->getSides().first;
        BWAPI::Position point2 = chokepoint->getSides().second;
        BWAPI::Broodwar->drawLine(BWAPI::CoordinateType::Map, point1.x(), point1.y(), point2.x(), point2.y(), BWAPI::Colors::Red);
    }
}