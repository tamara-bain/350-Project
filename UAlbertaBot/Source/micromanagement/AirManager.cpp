#include "Common.h"
#include "AirManager.h"

AirManager::AirManager() { }

void AirManager::executeMicro(const UnitVector & targets) 
{
	const UnitVector & airUnits = getUnits();

	// figure out targets
	UnitVector airUnitTargets;
	for (size_t i(0); i<targets.size(); i++) 
	{
		// conditions for targeting
		if (targets[i]->isVisible() &&
		   !(targets[i]->getType() == BWAPI::UnitTypes::Zerg_Larva) && 
		   !(targets[i]->getType() == BWAPI::UnitTypes::Zerg_Egg)) 
		{
			airUnitTargets.push_back(targets[i]);
		}
	}

	// for each air unit
	BOOST_FOREACH(BWAPI::Unit * airUnit, airUnits)
	{
		// if the order is to attack or defend
		if (order.type == order.Attack || order.type == order.Defend || order.type == order.BaseAssault) {

			// if there are targets
			if (!airUnitTargets.empty())
			{
				// find the best target for this unit
				BWAPI::Unit * target = getTarget(airUnit, airUnitTargets);

				// attack it
				kiteTarget(airUnit, target);
				//smartAttackUnit(airUnit, target);
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (airUnit->getDistance(order.position) > 100)
				{
					if(BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Protoss_Scout) < 3)
					{
						// need to get to the base quick to maintain air superiority
						smartMove(airUnit, order.position);
					}
					else 
					{
						smartAttackMove(airUnit, order.position);
					}
				}
			}
		}

		if (Options::Debug::DRAW_UALBERTABOT_DEBUG) 
		{
			BWAPI::Broodwar->drawLineMap(airUnit->getPosition().x(), airUnit->getPosition().y(), 
				airUnit->getTargetPosition().x(), airUnit->getTargetPosition().y(), Options::Debug::COLOR_LINE_TARGET);
		}
	}
}

void AirManager::kiteTarget(BWAPI::Unit * airUnit, BWAPI::Unit * target)
{
	
	double range(airUnit->getType().airWeapon().maxRange());

	// determine whether the target can be kited
	if (range <= target->getType().airWeapon().maxRange())
	{
		// if we can't kite it, there's no point
		smartAttackUnit(airUnit, target);
		return;
	}

	double		minDist(64);
	bool		kite(true);
	double		dist(airUnit->getDistance(target));
	double		speed(airUnit->getType().topSpeed());

	double	timeToEnter = std::max(0.0,(dist - range) / speed); // This is pretty broken, since scouts have acceleration. FIX ME!
	
	if(!target->getType().isFlyer())
	{
		// only kite flying units
		kite = false;
	}
	
	if ((timeToEnter >= airUnit->getAirWeaponCooldown()) && (dist >= minDist))
	{
		kite = false;
	}

	if (target->getType().isBuilding() && target->getType() != BWAPI::UnitTypes::Terran_Bunker)
	{
		kite = false;
	}

	if (airUnit->isSelected())
	{
		BWAPI::Broodwar->drawCircleMap(airUnit->getPosition().x(), airUnit->getPosition().y(), 
			(int)range, BWAPI::Colors::Cyan);
	}

	// if we can't shoot, run away
	if (kite)
	{
		BWAPI::Position fleePosition(airUnit->getPosition() - target->getPosition() + airUnit->getPosition());

		BWAPI::Broodwar->drawLineMap(airUnit->getPosition().x(), airUnit->getPosition().y(), 
			fleePosition.x(), fleePosition.y(), BWAPI::Colors::Cyan);

		smartMove(airUnit, fleePosition);
	}
	// otherwise shoot
	else
	{
		smartAttackUnit(airUnit, target);
	}
}

// get a target for the zealot to attack
BWAPI::Unit * AirManager::getTarget(BWAPI::Unit * airUnit, UnitVector & targets)
{
	int range(airUnit->getType().groundWeapon().maxRange());

	int highestInRangePriority(0);
	int highestNotInRangePriority(0);
	int lowestInRangeHitPoints(10000);
	int lowestNotInRangeDistance(10000);

	BWAPI::Unit * inRangeTarget = NULL;
	BWAPI::Unit * notInRangeTarget = NULL;

	BOOST_FOREACH(BWAPI::Unit * unit, targets)
	{
		int priority = getAttackPriority(airUnit, unit);
		int distance = airUnit->getDistance(unit);

		// if the unit is in range, update the target with the lowest hp
		if (airUnit->getDistance(unit) <= range)
		{
			if (priority > highestInRangePriority ||
				(priority == highestInRangePriority && unit->getHitPoints() < lowestInRangeHitPoints))
			{
				lowestInRangeHitPoints = unit->getHitPoints();
				highestInRangePriority = priority;
				inRangeTarget = unit;
			}
		}
		// otherwise it isn't in range so see if it's closest
		else
		{
			if (priority > highestNotInRangePriority ||
				(priority == highestNotInRangePriority && distance < lowestNotInRangeDistance))
			{
				lowestNotInRangeDistance = distance;
				highestNotInRangePriority = priority;
				notInRangeTarget = unit;
			}
		}
	}

	// if there is a highest priority unit in range, attack it first
	return (highestInRangePriority >= highestNotInRangePriority) ? inRangeTarget : notInRangeTarget;
}

	// get the attack priority of a type in relation to a zergling
int AirManager::getAttackPriority(BWAPI::Unit * airUnit, BWAPI::Unit * target) 
{
	BWAPI::UnitType airUnitType = airUnit->getType();
	BWAPI::UnitType targetType = target->getType();

	bool canAttackUs = targetType.airWeapon() != BWAPI::WeaponTypes::None;

	if(target->isRepairing())
	{
		return 8;
	}

	// highest priority is air defense structures
	if(targetType ==  BWAPI::UnitTypes::Zerg_Spore_Colony     || 
	   targetType ==  BWAPI::UnitTypes::Terran_Missile_Turret || 
	   targetType ==  BWAPI::UnitTypes::Protoss_Photon_Cannon || 
	   targetType ==  BWAPI::UnitTypes::Terran_Bunker)
	{
		return 7;
	}
	// second highest priority is something that can attack us or aid in combat
	else if (targetType == BWAPI::UnitTypes::Terran_Medic   || 
			 targetType == BWAPI::UnitTypes::Terran_Goliath ||
			 canAttackUs) 
	{
		return 6;
	}
	// next priority is worker
	else if (targetType.isWorker()) 
	{
		// prevent buildings from being constructed/repaired
		if(target->isConstructing()) 
		{
			BWAPI::Unit * beingBuilt = target->getBuildUnit();
			if( beingBuilt && (beingBuilt->getType() == BWAPI::UnitTypes::Terran_Armory ||
				beingBuilt->getType() == BWAPI::UnitTypes::Terran_Missile_Turret)) 
			{
				return 5;
			}
			
			return 4;
			
		}
		
		return 3;
	}
	// then everything else
	// next is buildings that cost gas
	else if (targetType.gasPrice() > 0)
	{
		return 2;
	}
	else 
	{
		return 1;
	}
}

BWAPI::Unit * AirManager::closestAirUnit(BWAPI::Unit * target, std::set<BWAPI::Unit *> & airUnitsToAssign)
{
	double minDistance = 0;
	BWAPI::Unit * closest = NULL;

	BOOST_FOREACH (BWAPI::Unit * airUnit, airUnitsToAssign)
	{
		double distance = airUnit->getDistance(target);
		if (!closest || distance < minDistance)
		{
			minDistance = distance;
			closest = airUnit;
		}
	}
	
	return closest;
}