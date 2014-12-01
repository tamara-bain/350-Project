#include "Common.h"
#include "ScoutRushManager.h"
#include "InformationManager.h"

ScoutRushManager::ScoutRushManager() : numWorkerScouts(0), enemyRegion(NULL), enemyBase(NULL), enemyGeyser(NULL), ourBase(NULL) { }

void ScoutRushManager::update(const std::set<BWAPI::Unit *> & scoutUnits) {
	
	if (ourBase == NULL)
		ourBase = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self());

	if (scoutUnits.size() == Options::Macro::SCOUT_RUSH_COUNT) {
		// insert each scout unit into scount managers set
		BOOST_FOREACH (BWAPI::Unit * scout, scoutUnits) {
			// if the worker isn't already in the set of scouts, assign it
			if (workerScouts.count(scout) == 0) {
				numWorkerScouts++;
				workerScouts.insert(scout);
				scoutFleeing[scout->getID()] = false;
                scoutMovingFromFleeing[scout->getID()] = false;
			}
		}
	}

	if (numWorkerScouts != 0) {

		// Get a leader for our scouts to follow (do this every time in case he died)
		BWAPI::Unit * leader = NULL;
        numWorkerScouts = 0;
		BOOST_FOREACH (BWAPI::Unit * scout, workerScouts) {
			if (!scout || !scout->exists() || !scout->getPosition().isValid()) {
				continue;
            }
			
            if (leader == NULL) 
			    leader = scout;
			
            numWorkerScouts++;
		}

		if (leader == NULL)
			return;

		// determine the region that the enemy is in if we haven't already
		if (enemyRegion == NULL) {
			enemyBase = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());
			enemyRegion = enemyBase ? enemyBase->getRegion() : NULL;
		}

		// determine the region the scout is in
		BWAPI::TilePosition leaderTile(leader->getPosition());
		BWTA::Region * leaderRegion = leaderTile.isValid() ? BWTA::getRegion(leaderTile) : NULL;

		// don't attack until we're in the enemy region
		if (leaderRegion != enemyRegion && scoutFleeing[leader->getID()] != true) {
			moveLeader(leader);
			moveFollowers(leader);
		}
		// all in attack!
		else {
			allInAttack();
		}
        drawDebugData();

	}
}

void ScoutRushManager::moveLeader(BWAPI::Unit * leader) {

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

void ScoutRushManager::moveFollowers(BWAPI::Unit * leader) {

	BWAPI::UnitCommand leaderCommand(leader->getLastCommand());
	BWAPI::Position leaderPosition(leaderCommand.getTargetPosition());

	// if our leader isn't moving neither are we
	if (leaderCommand.getType() != BWAPI::UnitCommandTypes::Move) {
		return;
	}

	BOOST_FOREACH (BWAPI::Unit * scout, workerScouts) {
		if (scout == leader)
			continue;

		if (!scout || !scout->exists() || !scout->getPosition().isValid()) {
			continue;
		}

		// if we have issued a command to this unit already this frame, ignore this one
		if (scout->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount())
			continue;

		// get the unit's current command
		BWAPI::UnitCommand currentCommand(scout->getLastCommand());

		// if we've already told this unit to move to the leaders position, ignore this command
		if (currentCommand.getType() == BWAPI::UnitCommandTypes::Move 
			&& currentCommand.getTargetPosition() == leaderPosition) {
			continue;
		}

		// follow the leader
		scout->issueCommand(leaderCommand);
	}
}

void ScoutRushManager::allInAttack() {

	BWAPI::Unit * target = NULL; 

	// first find out which scouts are being attacked, and if any are low health/sheilds
	BOOST_FOREACH (BWAPI::Unit * scout, workerScouts) {
        int attackers = getAttackingWorkers(scout);
        int nonFleeingScouts = getNonFleeingScouts(scout);

        if (scout->isUnderAttack() && attackers > nonFleeingScouts) {
            scoutFleeing[scout->getID()] = true;
        }
		else if (scout->isUnderAttack()) {

            // save their attacker as the current target
			target = getAttacker(scout);
            
			// if a scout is low health/sheilds than mark them as fleeing
			if (scout->getHitPoints() + scout->getShields() < 11) {
				scoutFleeing[scout->getID()] = true;
			}


		}

        if(fleeingScoutInRadius(scout, scout->getPosition()) && attackers > nonFleeingScouts) {
            scoutMovingFromFleeing[scout->getID()] = true;
        }
        else {
            scoutMovingFromFleeing[scout->getID()] = false;
        }

		if ((!scout->isUnderAttack() && !enemyInRadius(scout)) || (((scout->getHitPoints()) + (scout->getShields())) > 10 && 
            attackers < nonFleeingScouts)) {
			scoutFleeing[scout->getID()] = false;
		}
	}

	BOOST_FOREACH (BWAPI::Unit * scout, workerScouts) {
        int attackers = getAttackingWorkers(scout);

		// if we're fleeing we don't have time to attack
		if (scoutFleeing[scout->getID()] == true || scoutMovingFromFleeing[scout->getID()] == true) {

            std::vector<GroundThreat> groundThreats;
            

	        fillGroundThreats(groundThreats, scout);
            BWAPI::Position pos = scout->getPosition();
			BWAPI::Position fleeTo = calcFleePosition(groundThreats, scout);

            if (Options::Debug::DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawLineMap(pos.x(), pos.y(), fleeTo.x(), fleeTo.y(), BWAPI::Colors::Red);
            smartMove(scout, fleeTo);
            continue;
		}

		// otherwise find a target if non exists
		if (target == NULL || !target->exists() || !target->getPosition().isValid()) {
	        target = getScoutRushTarget(scout);
        }

		// attack a target if one exists
		if (target != NULL && target->exists() && target->getPosition().isValid()) {
			smartAttack(scout, target);
			continue;
		}

		// if we've reached the enemy base with no target
		if (scout->getDistance(enemyBase->getPosition()) < 30) {
			WorkerManager::Instance().finishedWithScoutWorker(scout);
			continue;
		}

		// otherwise keep moving into the enemy base
		smartMove(scout, enemyBase->getPosition());
	}
}

int ScoutRushManager::getAttackingWorkers(BWAPI::Unit * scout) {
    int attackingWorkers = 0;
    
    BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) {

        // if they're not in the enemy region we don't care or they're a worker doing workery things we don't care
        if (scout->getDistance(unit->getPosition()) > 300)
            continue;


        if (unit->getType().isWorker() && !(isDoingWorkerStuff(unit)))
		    ++attackingWorkers;
    }
    return attackingWorkers;
}

int ScoutRushManager::getNonFleeingScouts(BWAPI::Unit * scout) {
    int nonFleeingScouts = 0;
    
    BOOST_FOREACH(BWAPI::Unit * ally, workerScouts) {

        // if they're not close by they don't could
        if (scoutFleeing[ally->getID()] == true || scout->getDistance(ally->getPosition()) > 300)
            continue;

        nonFleeingScouts++;
    }
    return nonFleeingScouts;
}

BWAPI::Unit * ScoutRushManager::getAttacker(BWAPI::Unit * scout) {
	// if the previous target of the scout is attacking, then stay on target
	// we don't want to keep switching to the newest attacker
	BWAPI::Position position = scout->getPosition();
	BWAPI::Unit * attacker = getLastTarget(scout);

	if (attacker != NULL && attacker -> isAttacking())
		return attacker;


	// for each enemy worker find the closest or one in weapons range
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) {
		if (unit->isAttacking() && scout->isInWeaponRange(unit)) {
			attacker = unit;
			break;
		}
	}

	return attacker;
}

BWAPI::Unit * ScoutRushManager::getLastTarget(BWAPI::Unit * scout) {
	// if the last command was an attack try to get the last target
	BWAPI::UnitCommand lastCommand(scout->getLastCommand());
	BWAPI::Position position = scout->getPosition();
	int range = scout->getType().groundWeapon().maxRange();

	if (lastCommand.getType() == BWAPI::UnitCommandTypes::Attack_Unit) {

		BWAPI::Unit * target = lastCommand.getTarget();
		// we're only interested in staying on target if the last target was a valid enemy unit
		if (target != NULL && target->exists() && target->getPosition().isValid() && target->getType().canMove()) {

			// only stay on target if the unit stays within range of us
            BOOST_FOREACH(BWAPI::Unit * scout, workerScouts) {
			    if (target->getDistance(scout) < range + 10)
				    return target;
            }
		}
	}

	return NULL;
}

BWAPI::Unit * ScoutRushManager::getScoutRushTarget(BWAPI::Unit * scout)
{
    if (!scout || !scout->exists() || !scout->getPosition().isValid())
        return NULL;

	// if the last command was an attack try to get the last target
	BWAPI::Unit * closestEnemyWorker = getLastTarget(scout);

	if (closestEnemyWorker != NULL)
		return closestEnemyWorker;

	// return the closest worker
	BWAPI::Unit * closestEnemyOther = NULL;
	double closestDist = 100000;
	BWAPI::Position position = scout->getPosition();
	int range = scout->getType().groundWeapon().maxRange();
    int attackers = getAttackingWorkers(scout);
    int nonFleeing = getNonFleeingScouts(scout);

	// for each enemy worker find the closest or one in weapons range
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) {

		double dist = unit->getDistance(position);
		double distToBase = unit -> getDistance(enemyBase->getPosition());

		if (distToBase > 500)
			continue;

        if (unit->getType().isWorker()) {

            if (attackers > nonFleeing && !unit->isGatheringMinerals())
                continue;

			// if an enemy worker is in range we don't have to search anymore
			if (dist <= range)
				return unit;

			if (!closestEnemyWorker || dist < closestDist) {
				closestEnemyWorker = unit;
				closestDist = dist;
			}
        }

		else if (!closestEnemyWorker) {
            if ((!closestEnemyOther || dist < closestDist) && !(unit->isLifted()) && unit->getType() != BWAPI::UnitTypes::Zerg_Larva) {
				closestEnemyOther = unit;
				closestDist = dist;
			}
		}
	}

	if (closestEnemyWorker != NULL)
		return closestEnemyWorker;

	return closestEnemyOther;
}

BWAPI::Unit * ScoutRushManager::getMineralWorker(BWAPI::Unit * scout)
{
	// if the last command was an attack try to get the last target
	BWAPI::Unit * closestEnemyWorker = getLastTarget(scout);

    if (closestEnemyWorker != NULL && closestEnemyWorker->isGatheringMinerals())
		return closestEnemyWorker;

	// return the closest mineral gathering worker
	BWAPI::Unit * closestEnemyOther = NULL;
	double closestDist = 100000;
	BWAPI::Position position = scout->getPosition();
	int range = scout->getType().groundWeapon().maxRange();

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

BWAPI::Unit * ScoutRushManager::getFleeTarget(BWAPI::Unit * scout)
{
	BWAPI::Unit * target = NULL;

	// for each enemy worker find the closest or one in weapons range
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) {
			double distToGeyser = getEnemyGeyser() -> getDistance(unit);

			if (distToGeyser > 800)
				continue;

            if (unit->getType().isWorker() && unit->isGatheringMinerals()) {
                return unit;
            }
	}
    return target;
}

BWAPI::Unit * ScoutRushManager::getEnemyGeyser()
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

bool ScoutRushManager::enemyInRadius(BWAPI::Unit * scout)
{
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) {

		if (unit->getType().canAttack() && (unit->getDistance(scout) < 175)
            && !isDoingWorkerStuff(unit)) {
			return true;
		}
	}

	return false;
}

bool ScoutRushManager::fleeingScoutInRadius(BWAPI::Unit * scout, BWAPI::Position pos)
{
	BWAPI::Unit * target = NULL;

	BOOST_FOREACH(BWAPI::Unit * unit, workerScouts) {

		if (unit != scout && unit->getDistance(scout) < 80 && scoutFleeing[unit->getID()] == true) {
			return true;
		}
	}

	return false;
}

bool ScoutRushManager::isDoingWorkerStuff(BWAPI::Unit * unit) {
    if (unit->isGatheringMinerals() || unit->isGatheringGas() || unit->isConstructing())
        return true;
    return false;
}

void ScoutRushManager::fillGroundThreats(std::vector<GroundThreat> & threats, BWAPI::Unit * scout)
{

    BOOST_FOREACH(BWAPI::Unit* scout, workerScouts) {
        if (scoutFleeing[scout->getID()] == true && scout != scout) {
		    GroundThreat threat;
		    threat.unit		= scout;
		    threat.weight	= 5;
		    threats.push_back(threat);
        }
    }
    // get the ground threats in the enemy region
	BOOST_FOREACH (BWAPI::Unit * enemy, BWAPI::Broodwar->enemy()->getUnits())
	{
		// if they're not in the enemy region we don't care or they're a worker doing workery things we don't care
        if (isDoingWorkerStuff(enemy) || scout->getDistance(enemy->getPosition()) > 175)
            continue;

		// get the air weapon of the unit
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

void ScoutRushManager::smartMove(BWAPI::Unit * scout, BWAPI::Position targetPosition)
{
    if (!scout || !(scout->exists()))
        return;
	// if we have issued a command to this unit already this frame, ignore this one
	if (scout->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount())
		return;

	// if nothing prevents it then move
	scout->move(targetPosition);
}

void ScoutRushManager::smartAttack(BWAPI::Unit * attacker, BWAPI::Unit * target)
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

void ScoutRushManager::drawDebugData() {

    if (!Options::Debug::DRAW_UALBERTABOT_DEBUG) 
        return;

    BOOST_FOREACH (BWAPI::Unit * scout, workerScouts) {
        int x = scout->getPosition().x();
        int y = scout->getPosition().y();

        BWAPI::Broodwar->drawCircleMap(enemyBase->getPosition().x(), enemyBase->getPosition().y(), 300, BWAPI::Colors::Red);


        BWAPI::Unit * target = scout->getTarget();
	   
        if(target) {
            int tx = target->getPosition().x();
            int ty = target->getPosition().y();

            if (scout->isInWeaponRange(target)) {
                BWAPI::Broodwar->drawCircleMap(x, y, scout->getType().groundWeapon().maxRange(), BWAPI::Colors::Red, false);
            }
            else {
                 BWAPI::Broodwar->drawCircleMap(x, y, scout->getType().groundWeapon().maxRange(), BWAPI::Colors::Yellow, false);
            }
        }

        if (scout -> isStuck()) {
             BWAPI::Broodwar->drawTextMap(x, y, "Stuck!!!");
        }

	    else if(scoutFleeing[scout->getID()] == true) {
	        BWAPI::Broodwar->drawTextMap(x, y, "Fleeing!");
            BWAPI::Broodwar->drawCircleMap(x, y, 200, BWAPI::Colors::Green, false);
            BWAPI::Broodwar->drawCircleMap(x, y, 100, BWAPI::Colors::Purple, false);
        }

        else if(scoutMovingFromFleeing[scout->getID()] == true) {
	        BWAPI::Broodwar->drawTextMap(x, y, "Moving Aside!");
            BWAPI::Broodwar->drawCircleMap(x, y, 20, BWAPI::Colors::Purple, false);
        }

        BWAPI::Position maxBar =  BWAPI::Position((static_cast<int>((double)TILE_SIZE*0.7)), TILE_SIZE/10);
        BWAPI::Position barPosition = BWAPI::Position(x-maxBar.x()/2, y-maxBar.y()/2);
        int currentHP = static_cast<int>(((double)(scout->getHitPoints()) / (double)(scout->getType().maxHitPoints())) * maxBar.x());
        
        BWAPI::Broodwar->drawBoxMap(barPosition.x(), barPosition.y(), barPosition.x()+maxBar.x(), barPosition.y()+maxBar.y(), BWAPI::Colors::Red, true);
        BWAPI::Broodwar->drawBoxMap(barPosition.x(), barPosition.y(), barPosition.x()+currentHP, barPosition.y()+maxBar.y(), BWAPI::Colors::Green, true);

        if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Protoss) {
            int currentShields = static_cast<int>(((double)(scout->getShields()) / (double)(scout->getType().maxShields())) * maxBar.x());

            BWAPI::Broodwar->drawBoxMap(barPosition.x(), barPosition.y()-maxBar.y(), barPosition.x()+maxBar.x(), barPosition.y(), BWAPI::Colors::Grey, true);
            BWAPI::Broodwar->drawBoxMap(barPosition.x(), barPosition.y()-maxBar.y(), barPosition.x()+currentShields, barPosition.y(), BWAPI::Colors::Blue, true);
        }
      
    }
}

BWAPI::Position ScoutRushManager::calcFleePosition(const std::vector<GroundThreat> & threats, BWAPI::Unit * scout) 
{
	// calculate the standard flee vector
    BWAPI::Position pos = scout->getPosition();


    int mult = 1;
    if (scout->getID() % 2)
        mult = -1;

    double2 fleeVector(0,0);
    fleeVector = getFleeVector(threats, pos);


	// rotate the flee vector by 30 degrees, this will allow us to circle around and come back at a target
	//fleeVector.rotate(mult*30);
	double2 newFleeVector;
	int iterations = 0;
	int r = 30;
    std::vector<double2> moves;

	// keep rotating the vector until the new position is able to be walked on
	while (iterations < 73) 
	{
		// rotate the flee vector
        newFleeVector = fleeVector;
        newFleeVector.rotate(mult*r);


		// re-normalize it
		newFleeVector.normalise();

		// the position we will attempt to go to
		BWAPI::Position test(pos + newFleeVector * 56);

		// if the position is able to be walked on, break out of the loop
		if (isValidFleePosition(test, scout, newFleeVector)) {
			break;
        }

        if (r >=0)
            r = -(r+5);
        else
            r = -r;

        ++iterations;
	}

	// go to the calculated 'good' position
	BWAPI::Position fleeTo(pos + newFleeVector * 56);
	
	return fleeTo;
}

double2 ScoutRushManager::getFleeVector(const std::vector<GroundThreat> & threats, BWAPI::Position pos)
{
	double2 fleeVector(0,0);

	BOOST_FOREACH (const GroundThreat & threat, threats)
	{
		// Get direction from enemy
		const double2 direction(pos - threat.unit->getPosition());

		// Divide direction by square distance, weighting closer enemies higher
		//  Dividing once normalises the vector
		//  Dividing a second time reduces the effect of far away units
		const double distanceSq(direction.lenSq());
		if(distanceSq > 0)
		{
			// Enemy influence is direction to flee from enemy weighted by danger posed by enemy
			const double2 enemyInfluence( (direction / distanceSq) * threat.weight );
			fleeVector = fleeVector + enemyInfluence;
		}
	}

	if(fleeVector.lenSq() == 0 || threats.size()==0)
	{
		// Flee towards our base
		fleeVector = double2(ourBase->getPosition().x(),ourBase->getPosition().y());	
	}

	fleeVector.normalise();

	return fleeVector;
}

bool ScoutRushManager::isValidFleePosition(BWAPI::Position pos, BWAPI::Unit * scout, double2 fleeVector) 
{

    if (!pos.isValid())
        return false;

	// get x and y from the position
	int x(pos.x()), y(pos.y());
    
	// walkable tiles exist every 8 pixels
	bool good = BWAPI::Broodwar->isWalkable(x/8, y/8);
	
	// if it's not walkable throw it out
	if (!good) return false;

	// don't go anywhere that is occupied (but we'' try to force our way through for workers)
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->getUnitsOnTile(x/32, y/32)) {
        if	(!unit->isLifted() && !unit->getType().isFlyer() && !unit->getType().isWorker())
		    return false;
    }

	int geyserDist = 16;
	int mineralDist = 8;
    int baseDist = 90;

	BWTA::BaseLocation * enemyLocation = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());

	BWAPI::Unit * geyser = getEnemyGeyser();
	if (Options::Debug::DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawCircleMap(geyser->getPosition().x(), geyser->getPosition().y(), geyserDist, BWAPI::Colors::Red);

    // stay away from geysers
	if (geyser->getDistance(pos) < geyserDist)
		return false;

	BOOST_FOREACH (BWAPI::Unit * mineral, enemyLocation->getMinerals()) {
        if (Options::Debug::DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawCircleMap(mineral->getPosition().x(), mineral->getPosition().y(), mineralDist, BWAPI::Colors::Red);
		if (mineral->getDistance(pos) < mineralDist)
			return false;
    }

    if (Options::Debug::DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawCircleMap(enemyBase->getPosition().x(), enemyBase->getPosition().y(), baseDist, BWAPI::Colors::Red);
    // stay away from geysers
	if (enemyBase->getPosition().getDistance(pos) < baseDist) {
		return false;
    }

    if (BWTA::getRegion(BWAPI::TilePosition(pos)) != enemyRegion)
		return false;


    double2 checkIfNarrow2(fleeVector);
    double2 checkIfNarrow3(fleeVector);

    checkIfNarrow2.rotate(45);
    checkIfNarrow3.rotate(-45);

    checkIfNarrow2.normalise();
    checkIfNarrow3.normalise();
    BWAPI::Position scoutPos = scout->getPosition();
    BWAPI::Position test2(scoutPos  + checkIfNarrow2 * 250);
    BWAPI::Position test3(scoutPos  + checkIfNarrow3 * 250);

	if (BWTA::getRegion(BWAPI::TilePosition(test2)) != enemyRegion &&
        BWTA::getRegion(BWAPI::TilePosition(test3)) != enemyRegion) {
            if (Options::Debug::DRAW_UALBERTABOT_DEBUG) {
                BWAPI::Broodwar->drawLineMap(scoutPos.x(), scoutPos.y(), test2.x(), test2.y(), BWAPI::Colors::Red);
                BWAPI::Broodwar->drawLineMap(scoutPos.x(), scoutPos.y(), test3.x(), test3.y(), BWAPI::Colors::Red);
                BWAPI::Broodwar->drawLineMap(scoutPos.x(), scoutPos.y(), pos.x(), pos.y(), BWAPI::Colors::Green);
            }
        return false;
    }
	// otherwise it's okay
	return true;
}
