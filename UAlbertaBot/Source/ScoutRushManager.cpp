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
			}
		}
	}

	if (numWorkerScouts != 0) {

		// Get a leader for our scouts to follow (do this every time in case he died)
		BWAPI::Unit * leader = NULL;
		BOOST_FOREACH (BWAPI::Unit * scout, workerScouts) {
			if (!scout || !scout->exists() || !scout->getPosition().isValid()) 
				continue;
			
			leader = scout;
			break;
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

    std::vector<GroundThreat> groundThreats;
	fillGroundThreats(groundThreats);

    if (getAttackingWorkers() > Options::Macro::SCOUT_RUSH_COUNT + 1) {
        BOOST_FOREACH (BWAPI::Unit * scout, workerScouts) {

            BWAPI::Position fleeTo = calcFleePosition(groundThreats, scout);
            if (Options::Debug::DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawCircleMap(fleeTo.x(), fleeTo.y(), 10, BWAPI::Colors::Red);
            smartMove(scout, fleeTo);
        }
        return;
    }

	// first find out which scouts are being attacked, and if any are low health/sheilds
	BOOST_FOREACH (BWAPI::Unit * scout, workerScouts) {
		if (scout->isUnderAttack()) {
			// if a scout is low health/sheilds than mark them as fleeing
			if (scout->getHitPoints() < 11 && scout->getShields() == 0) {
				scoutFleeing[scout->getID()] = true;
			}

			// save their attacker as the current target
			target = getAttacker(scout);
		}
		if ((!scout->isUnderAttack() && !enemyInRadius(scout)) || (((scout->getHitPoints()) + (scout->getShields())) > 10)) {
			scoutFleeing[scout->getID()] = false;
		}
	}

	BOOST_FOREACH (BWAPI::Unit * scout, workerScouts) {
		// if we're fleeing we don't have time to attack
		if (scoutFleeing[scout->getID()] == true) {
			BWAPI::Position fleeTo = calcFleePosition(groundThreats, scout);
            if (Options::Debug::DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawCircleMap(fleeTo.x(), fleeTo.y(), 10, BWAPI::Colors::Red);
            smartMove(scout, fleeTo);
			continue;
		}

		// otherwise find a target if non exists

		if (target == NULL || !target->exists() || !target->getPosition().isValid())
			target = getScoutRushTarget(scout);

		// attack a target if one exists
		if (target != NULL && target->exists() && target->getPosition().isValid()) {
			smartAttack(scout, target);
			continue;
		}

		// if we've reached the enemy base with no target
		if (scout->getDistance(enemyBase->getPosition()) < 5) {
			WorkerManager::Instance().finishedWithScoutWorker(scout);
			continue;
		}

        BWAPI::Broodwar->printf("MOVING");
		// otherwise keep moving into the enemy base
		smartMove(scout, enemyBase->getPosition());
	}
}

int ScoutRushManager::getAttackingWorkers() {
    int attackingWorkers = 0;
    
    BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) {

        // if they're not in the enemy region we don't care
		BWAPI::TilePosition tile(unit->getPosition());
		BWTA::Region * region = tile.isValid() ? BWTA::getRegion(tile) : NULL;

        if (!region || region != enemyRegion)
            continue;

        if ((unit->isAttacking() || !(unit->isGatheringMinerals())) && unit->getType().isWorker()) {
		    ++attackingWorkers;
	    }
    }
    return attackingWorkers;
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
		if (target != NULL && target->exists() && !target->getPosition().isValid() && target->getType().canMove()) {

			// only stay on target if the unit stays within range of us
			if (target->getDistance(position) < range + 20)
				return target;
		}
	}

	return NULL;
}

BWAPI::Unit * ScoutRushManager::getScoutRushTarget(BWAPI::Unit * scout)
{
	// if the last command was an attack try to get the last target
	BWAPI::Unit * closestEnemyWorker = getLastTarget(scout);

	if (closestEnemyWorker != NULL)
		return closestEnemyWorker;

	// return the closest mineral gathering worker
	BWAPI::Unit * closestEnemyOther = NULL;
	double closestDist = 100000;
	BWAPI::Position position = scout->getPosition();
	int range = scout->getType().groundWeapon().maxRange();

	// for each enemy worker find the closest or one in weapons range
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->enemy()->getUnits()) {

			double dist = unit->getDistance(position);
			double distToGeyser = getEnemyGeyser() -> getDistance(unit);

			if (distToGeyser > 800)
				continue;

		if (unit->getType().isWorker()) {

			// if an enemy worker is in range we don't have to search anymore
			if (dist <= range)
				return unit;

			if (!closestEnemyWorker || dist < closestDist) {
				closestEnemyWorker = unit;
				closestDist = dist;
			}
		}

		else if (!closestEnemyWorker) {
			if ((!closestEnemyOther || dist < closestDist) && !(unit->isLifted())) {
				closestEnemyOther = unit;
				closestDist = dist;
			}
		}
	}

	if (closestEnemyWorker != NULL)
		return closestEnemyWorker;

	return closestEnemyOther;
}

BWAPI::Unit * ScoutRushManager::getFleeTarget(BWAPI::Unit * scout)
{
	// if the last command was an attack try to get the last target
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

		if (unit->getType().canAttack() && (unit->getDistance(scout) < 200)) {
			return true;
		}
	}

	return false;
}

void ScoutRushManager::fillGroundThreats(std::vector<GroundThreat> & threats)
{
    // get the ground threats in the enemy region
	BOOST_FOREACH (BWAPI::Unit * enemy, BWAPI::Broodwar->enemy()->getUnits())
	{
		// if they're not in the enemy region we don't care
		BWAPI::TilePosition tile(enemy->getPosition());
		BWTA::Region * region = tile.isValid() ? BWTA::getRegion(tile) : NULL;

        if (!region || region != enemyRegion)
            return;

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

        BWAPI::Unit * target = scout->getTarget();
	    BWAPI::Broodwar->drawCircleMap(x, y, scout->getType().groundWeapon().maxRange(), BWAPI::Colors::Yellow, false);
        if(target) {
            int tx = target->getPosition().x();
            int ty = target->getPosition().y();

            if (scout->isInWeaponRange(target)) 
                BWAPI::Broodwar->drawCircleMap(x, y, scout->getType().groundWeapon().maxRange()+20, BWAPI::Colors::Red, false);
             BWAPI::Broodwar->drawLineMap(x, y, tx, ty, BWAPI::Colors::Cyan);
        }

	    if(scoutFleeing[scout->getID()] == true) {
	        BWAPI::Broodwar->drawTextMap(x, y, "Fleeing!");
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
	double2 fleeVector = getFleeVector(threats, scout);

	// vector to the target, if one exists
	double2 targetVector(0,0);

	int mult = 1;
	if (scout->getID() % 2) 
		mult = -1;

	// rotate the flee vector by 30 degrees, this will allow us to circle around and come back at a target
	//fleeVector.rotate(mult*30);
	double2 newFleeVector;

	int r = 0;
	int sign = 1;
	int iterations = 0;
		
	// keep rotating the vector until the new position is able to be walked on
	while (r <= 360) 
	{
		// rotate the flee vector
		fleeVector.rotate(mult*r);

		// re-normalize it
		fleeVector.normalise();

		// new vector should semi point back at the target
		newFleeVector = fleeVector * 2 + targetVector;

		// the position we will attempt to go to
		BWAPI::Position test(scout->getPosition() + newFleeVector * 24);

		if (Options::Debug::DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawLineMap(scout->getPosition().x(), scout->getPosition().y(), test.x(), test.y(), BWAPI::Colors::Cyan);


		// if the position is able to be walked on, break out of the loop
		if (isValidFleePosition(test))
			break;

		r = r + sign * (r + 10);
		sign = sign * -1;

		if (++iterations > 36)
			break;
	}

	// go to the calculated 'good' position
	BWAPI::Position fleeTo(scout->getPosition() + newFleeVector * 24);
	
	
	if (Options::Debug::DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawLineMap(scout->getPosition().x(), scout->getPosition().y(), fleeTo.x(), fleeTo.y(), BWAPI::Colors::Orange);
	

	return fleeTo;
}

double2 ScoutRushManager::getFleeVector(const std::vector<GroundThreat> & threats, BWAPI::Unit * scout)
{
	double2 fleeVector(0,0);

	BOOST_FOREACH (const GroundThreat & threat, threats)
	{
		// Get direction from enemy to mutalisk
		const double2 direction(scout->getPosition() - threat.unit->getPosition());

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

	if(fleeVector.lenSq() == 0)
	{
		// Flee towards our base
		fleeVector = double2(1,0);	
	}

	fleeVector.normalise();

	return fleeVector;
}

bool ScoutRushManager::isValidFleePosition(BWAPI::Position pos) 
{

	// get x and y from the position
	int x(pos.x()), y(pos.y());

	// walkable tiles exist every 8 pixels
	bool good = BWAPI::Broodwar->isWalkable(x/8, y/8);
	
	// if it's not walkable throw it out
	if (!good) return false;
	
	// for each of those units, if it's a building or an attacking enemy unit we don't want to go there
	BOOST_FOREACH(BWAPI::Unit * unit, BWAPI::Broodwar->getUnitsOnTile(x/32, y/32)) 
	{
		if	(unit->getType().isBuilding() || unit->getType().isResourceContainer() || 
			(unit->getPlayer() != BWAPI::Broodwar->self() && unit->getType().groundWeapon() != BWAPI::WeaponTypes::None)) 
		{		
				return false;
		}
	}

	int geyserDist = 50;
	int mineralDist = 32;

	BWTA::BaseLocation * enemyLocation = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());

	BWAPI::Unit * geyser = getEnemyGeyser();
	if (Options::Debug::DRAW_UALBERTABOT_DEBUG) BWAPI::Broodwar->drawCircleMap(geyser->getPosition().x(), geyser->getPosition().y(), geyserDist, BWAPI::Colors::Red);

	if (geyser->getDistance(pos) < geyserDist)
	{
		return false;
	}

	BOOST_FOREACH (BWAPI::Unit * mineral, enemyLocation->getMinerals())
	{
		if (mineral->getDistance(pos) < mineralDist)
		{
			return false;
		}
	}

	BWTA::Region * enemyRegion = enemyLocation->getRegion();
	if (enemyRegion && BWTA::getRegion(BWAPI::TilePosition(pos)) != enemyRegion)
	{
		return false;
	}

	// otherwise it's okay
	return true;
}
