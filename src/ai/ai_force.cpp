//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name ai_force.c - AI force functions. */
//
//      (c) Copyright 2001-2004 by Lutz Sammer
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; version 2 dated June, 1991.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//
//      $Id$

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"
#include "unittype.h"
#include "unit.h"
#include "ai_local.h"
#include "actions.h"
#include "map.h"
#include "depend.h"
#include "pathfinder.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

global int UnitTypeEquivs[UnitTypeMax + 1]; /// equivalence between unittypes

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/


/**
**  Remove any equivalence between unittypes
*/
global void AiResetUnitTypeEquiv(void)
{
	int i;

	for (i = 0; i <= UnitTypeMax; i++) {
		UnitTypeEquivs[i] = i;
	}
}

/**
**  Make two unittypes equivalents from the AI's point of vue
**
**  @param a  the first unittype
**  @param b  the second unittype
*/
global void AiNewUnitTypeEquiv(UnitType* a, UnitType* b)
{
	int find;
	int replace;
	int i;

	find = UnitTypeEquivs[a->Type];
	replace = UnitTypeEquivs[b->Type];

	// Always record equivalences with the lowest unittype.
	if (find < replace) {
		i = find;
		find = replace;
		replace = i;
	}

	// Then just find & replace in UnitTypeEquivs...
	for (i = 0; i <= UnitTypeMax; ++i) {
		if (UnitTypeEquivs[i] == find) {
			UnitTypeEquivs[i] = replace;
		}
	}
}


/**
**  Find All unittypes equivalent to a given one
**
**  @param unittype  the unittype to find equivalence for
**  @param result    int array which will hold the result. (Size UnitTypeMax+1)
**
**  @return          the number of unittype found
*/
global int AiFindUnitTypeEquiv(const UnitType* unittype, int* result)
{
	int i;
	int search;
	int count;

	search = UnitTypeEquivs[unittype->Type];
	count = 0;

	for (i = 0; i < UnitTypeMax + 1; ++i) {
		if (UnitTypeEquivs[i] == search) {
			// Found one
			result[count] = i;
			count++;
		}
	}

	return count;
}

/**
**  Find All unittypes equivalent to a given one, and which are available
**  UnitType are returned in the prefered order ( ie palladin >> knight... )
**
**  @param unittype     The unittype to find equivalence for
**  @param usabelTypes  int array which will hold the result. (Size UnitTypeMax+1)
**
**  @return          the number of unittype found
*/
global int AiFindAvailableUnitTypeEquiv(const UnitType* unittype, int* usableTypes)
{
	int usableTypesCount;
	int i;
	int j;
	int tmp;
	int playerid;
	int bestlevel;
	int curlevel;

	// 1 - Find equivalents
	usableTypesCount = AiFindUnitTypeEquiv(unittype,  usableTypes);

	// 2 - Remove unavailable unittypes
	for (i = 0; i < usableTypesCount; ) {
		if (!CheckDependByIdent(AiPlayer->Player, UnitTypes[usableTypes[i]]->Ident)) {
			// Not available, remove it
			usableTypes[i] = usableTypes[usableTypesCount - 1];
			usableTypesCount--;
		} else {
			i++;
		}
	}

	// 3 - Sort by level
	playerid = AiPlayer->Player->Player;

	// We won't have usableTypesCount>4, so simple sort should do it
	for (i = 0; i < usableTypesCount-1; i++) {
		bestlevel = UnitTypes[usableTypes[i]]->Priority;
		for (j = i + 1; j < usableTypesCount; j++) {
			curlevel = UnitTypes[usableTypes[j]]->Priority;

			if (curlevel > bestlevel) {
				// Swap
				tmp = usableTypes[j];
				usableTypes[j] = usableTypes[i];
				usableTypes[i] = tmp;

				bestlevel = curlevel;
			}
		}
	}
	DebugLevel3Fn("prefered order for %s is " _C_ unittype->Ident);
	for (i = 0; i < usableTypesCount; i++) {
		DebugLevel3(" %s" _C_ UnitTypes[usableTypes[i]]->Ident);
	}
	DebugLevel3("\n");

	return usableTypesCount;
}

/**
**  Count available units by type in a force.
**
**  The returned array will map UnitType=>number of unit
**
**  @param force        the force to count unit
**  @param countByType  array[UnitTypeMax+1] of int
*/
global void AiForceCountUnits(int force, int* countByType)
{
	int type;
	AiUnit* aiunit;

	memset(countByType, 0, sizeof (int) * (UnitTypeMax + 1));

	aiunit = AiPlayer->Force[force].Units;
	while (aiunit) {
		if ((!aiunit->Unit->Destroyed) &&
			(aiunit->Unit->HP) && (aiunit->Unit->Orders[0].Action != UnitActionDie)) {
			type = UnitTypeEquivs[aiunit->Unit->Type->Type];

			DebugCheck((type < 0) || (type > UnitTypeMax));
			countByType[type]++;
		}
		aiunit = aiunit->Next;
	}
}

/**
**  Substract wanted unit in a force. ( from the result of AiForceCountUnits )
**
**  @param force        the force to count unit
**  @param countByType  array[UnitTypeMax+1] of int
**
**  @return             The number of missing unit
*/
global int AiForceSubstractWant(int force, int* countByType)
{
	int missing;
	int type;
	const AiUnitType* aitype;

	missing = 0;
	aitype = AiPlayer->Force[force].UnitTypes;
	while (aitype) {
		type=UnitTypeEquivs[aitype->Type->Type];
		countByType[type] -= aitype->Want;
		if (countByType[type] < 0) {
			missing -= countByType[type];
		}
		aitype = aitype->Next;
	}

	return missing;
}

/**
**  Complete dst force with units from src force.
**
**  @todo FIXME : should check that unit can reach dst force's hotspot.
**
**  @param src  the force from which units are taken
**  @param dst  the force into which units go
*/
global void AiForceTransfert(int src, int dst)
{
	AiUnit** prev;
	AiUnit* aiunit;
	int type;
	int counter[UnitTypeMax + 1];
	int missing;

	//
	// Count units in dest force.
	//
	AiForceCountUnits(dst, counter);

	//
	// Check the dest force requirements.
	//
	if ((missing = AiForceSubstractWant(dst, counter)) == 0) {
		// Nothing missing => mark completed & abort.
		AiPlayer->Force[dst].Completed = 1;
		return;
	}

	// Iterate the source force, moving needed units into dest...
	prev = &AiPlayer->Force[src].Units;
	while (*prev) {
		aiunit = (*prev);
		type = UnitTypeEquivs[aiunit->Unit->Type->Type];
		if (counter[type] < 0) {
			// move in dest force...
			*prev = aiunit->Next;

			aiunit->Next = AiPlayer->Force[dst].Units;
			AiPlayer->Force[dst].Units = aiunit;

			counter[type]++;
			missing--;

			if (!missing) {
				AiPlayer->Force[dst].Completed = 1;
				return;
			}
		} else {
			// Just iterate
			prev = &aiunit->Next;
		}
	}
}

/**
**  Complete dst force with overflow units in src force.
**
**  @todo FIXME : should check that unit can reach dst force's hotspot.
**
**  @param src  the force from which units are taken
**  @param dst  the force into which units go
*/
global void AiForceTransfertOverflow(int src, int dst)
{
	AiUnit** prev;
	AiUnit* aiunit;
	int type;
	int counter[UnitTypeMax + 1];
	int overflow[UnitTypeMax + 1];
	int missing;

	//
	// Count units in dest force.
	//
	AiForceCountUnits(dst, counter);

	//
	// Check the dest force requirements.
	//
	if ((missing = AiForceSubstractWant(dst, counter)) == 0) {
		// Nothing missing => abort.
		AiPlayer->Force[dst].Completed = 1;
		return;
	}

	//
	// Find overflow units in src force
	//
	AiForceCountUnits(src, overflow);
	AiForceSubstractWant(src, overflow);

	// Iterate the source force, moving needed units into dest...
	prev = &AiPlayer->Force[src].Units;
	while (*prev) {
		aiunit = (*prev);
		type = UnitTypeEquivs[aiunit->Unit->Type->Type];
		if (counter[type] < 0 && overflow[type] > 0) {
			// move in dest force...
			*prev = aiunit->Next;

			aiunit->Next = AiPlayer->Force[dst].Units;
			AiPlayer->Force[dst].Units = aiunit;

			++counter[type];
			--overflow[type];
			--missing;
			if (!missing) {
				AiPlayer->Force[dst].Completed = 1;
				return;
			}
		} else {
			// Just iterate
			prev = &aiunit->Next;
		}
	}
}

/**
**  Ai clean units in a force.
**
**  @param force  Force number.
*/
global void AiCleanForce(int force)
{
	AiUnit** prev;
	AiUnit* aiunit;
	int counter[UnitTypeMax + 1];
	int unit_released;

	//
	// Release all killed units.
	//
	prev = &AiPlayer->Force[force].Units;
	while ((aiunit = *prev)) {
		if (aiunit->Unit->Destroyed) {
			RefsDecrease(aiunit->Unit);
			*prev = aiunit->Next;
			free(aiunit);
			continue;
		} else if (!aiunit->Unit->HP || aiunit->Unit->Orders[0].Action == UnitActionDie) {
			RefsDecrease(aiunit->Unit);
			*prev = aiunit->Next;
			free(aiunit);
			continue;
		}
		prev = &aiunit->Next;
	}

	//
	// Count units in force.
	//
	AiForceCountUnits(force, counter);

	//
	// Look if the force is complete.
	//
	AiPlayer->Force[force].Completed = (AiForceSubstractWant(force, counter) == 0);

	// Don't prune the 0 force in any case
	if (force > 0) {
		//
		// Release units too much in force.
		//
		unit_released = 0;
		prev = (&AiPlayer->Force[force].Units);
		while ((aiunit = (*prev))) {
			if (counter[aiunit->Unit->Type->Type] > 0) {
				DebugLevel3Fn("Release unit %s\n" _C_ aiunit->Unit->Type->Ident);
				counter[aiunit->Unit->Type->Type]--;
				RefsDecrease(aiunit->Unit);
				*prev = aiunit->Next;

				// Move this unit somewhere else...
				AiAssignToForce(aiunit->Unit);
				free(aiunit);

				continue;
			}
			prev = &aiunit->Next;
		}
	}

	DebugLevel3Fn("%d complete %d\n" _C_ force _C_ AiPlayer->Force[force].Completed);
}

/**
**
**  Remove everything in the given force
**
**  @param force  the force to erase
*/
global void AiEraseForce(int force)
{
	AiUnitType* aiut;
	AiUnitType* next;
	AiUnit* aiu;
	AiUnit* next_u;

	aiut = AiPlayer->Force[force].UnitTypes;
	while (aiut) {
		next = aiut->Next;
		free(aiut);
		aiut = next;
	}
	AiPlayer->Force[force].UnitTypes = 0;

	aiu = AiPlayer->Force[force].Units;
	while (aiu) {
		// Decrease usage count
		RefsDecrease(aiu->Unit);

		next_u = aiu->Next;
		free(aiu);
		aiu = next_u;
	}
	AiPlayer->Force[force].Units = 0;

	AiAssignFreeUnitsToForce();
}

/**
**  Cleanup units in forces.
*/
global void AiCleanForces(void)
{
	int force;

	//
	// Release all killed units.
	//
	for (force = 0; force < AI_MAX_FORCES; ++force) {
		AiCleanForce(force);
	}
}

/**
**  Check if the units belongs to the force.
**  If ok, update the completed flag
**
**  @param force  Force to be checked.
**  @param type   Type to check.
**
**  @return       Returns true if it fits & update completed flag, false otherwise.
*/
local int AiCheckBelongsToForce(int force, const UnitType* type)
{
	int counter[UnitTypeMax + 1];
	int missing;
	int realtype;

	//
	// Count units in force.
	//
	AiForceCountUnits(force, counter);

	//
	// Look what should be in the force.
	//
	missing = AiForceSubstractWant(force, counter);
	AiPlayer->Force[force].Completed = (missing == 0);

	realtype = UnitTypeEquivs[type->Type];

	if (counter[realtype] < 0) {
		// Ok we will put this unit in this force !
		// Just one missing ?
		if ((counter[realtype] == -1) && (missing == 1)) {
			AiPlayer->Force[force].Completed = 1;
		}
		return 1;
	}
	return 0;
}

/**
**  Ai assign unit to force.
**
**  @param unit  Unit to assign to force.
*/
global void AiAssignToForce(Unit* unit)
{
	AiUnit* aiunit;
	int force;

	//
	// Check to which force it belongs
	//
	for (force = 0; force < AI_MAX_FORCES; ++force) {
		// care of populate from scratch only.
		if (AiPlayer->Force[force].PopulateMode != AiForcePopulateFromScratch) {
			continue;
		}

		if (AiCheckBelongsToForce(force, unit->Type)) {
			aiunit = malloc(sizeof (*aiunit));
			aiunit->Next = AiPlayer->Force[force].Units;
			AiPlayer->Force[force].Units = aiunit;
			aiunit->Unit = unit;
			RefsIncrease(unit);
			return;
		}
	}

	// Add to the 0 force !
	// ( we overflow the 0 force here, so completed does not need update )
	aiunit = malloc(sizeof (*aiunit));
	aiunit->Next = AiPlayer->Force[0].Units;
	AiPlayer->Force[0].Units = aiunit;
	aiunit->Unit = unit;
	RefsIncrease(unit);
}

/**
**  Try to complete a force, using all available units
**
**  @param force  the force to complete
*/
global void AiForceComplete(int force)
{
	int j;
	int overflowonly;;

	for (j = 0; j < AI_MAX_FORCES; ++j) {
		// Don't complete with self ...
		if (j == force) {
			continue;
		}

		// Complete only with "reusable" forces.
		if (!AiPlayer->Force[j].UnitsReusable) {
			continue;
		}

		// Honor "populate from attack"
		if ((AiPlayer->Force[force].PopulateMode == AiForcePopulateFromAttack) &&
			(!AiPlayer->Force[j].Role == AiForceRoleAttack)) {

			// Use overflow from force 0...
			if (j == 0) {
				overflowonly = 1;
			} else {
					continue;
			}
		} else {
			overflowonly = 0;
		}

		// Complete the force automatically...
		if (!overflowonly) {
			AiForceTransfert(j, force);
		} else {
			AiForceTransfertOverflow(j, force);
		}

		if (AiPlayer->Force[force].Completed) {
			break;
		}
	}
}

/**
**  Enrole a unit in the specific force.
**  Does not take equivalence into account
**
**  @todo FIXME : currently iterate all units (slow)
**        FIXME : should take units which are closer to the hotspot.
**        FIXME : should ensure that units can move to the hotspot.
**
**  @param force  the force to put units on
**  @param ut     the searched unittype
**  @param count  the number of unit to add
**
**  @return       the number of unit still missing (or 0 if successful)
*/
global int AiEnroleSpecificUnitType(int force, UnitType* ut, int count)
{
	AiForce* dstForce;
	int src_force;
	AiUnit* aiUnit;
	AiUnit** prev;

	dstForce = AiPlayer->Force + force;
	for (src_force = 0; src_force < AI_MAX_FORCES; src_force++) {
		if (src_force == force) {
			continue;
		}
		// Only populate with reserve
		if (!AiPlayer->Force[src_force].UnitsReusable) {
			continue;
		}
		// Don't populate attack force with defend reserve.
		if ((AiPlayer->Force[src_force].Role == AiForceRoleDefend) &&
			(AiPlayer->Force[force].PopulateMode == AiForcePopulateFromAttack)) {
			continue;
		}

		aiUnit = AiPlayer->Force[src_force].Units;
		prev = &AiPlayer->Force[src_force].Units;
		while (aiUnit) {
			if (aiUnit->Unit->Type->Type == ut->Type) {
				*prev = aiUnit->Next;

				// Move to dstForce
				AiPlayer->Force[src_force].Completed = 0;
				aiUnit->Next = dstForce->Units;
				dstForce->Units = aiUnit;

				count--;
				if (!count) {
					return 0;
				}
			}
			prev = &aiUnit->Next;
			aiUnit = aiUnit->Next;
		}
	}
	return count;
}

/**
**  Make sure that current force requirement are superior to actual assigned unit count
**
*/
local void AiFinalizeForce(int force)
{
	int i;
	int type;
	int unitcount[UnitTypeMax + 1];
	AiUnitType *aitype;

	AiForceCountUnits(force, unitcount);
	aitype = AiPlayer->Force[force].UnitTypes;
	while (aitype) {
		type = UnitTypeEquivs[aitype->Type->Type];
		if (unitcount[type] > aitype->Want) {
			aitype->Want = unitcount[type];
			unitcount[type] = 0;
		}
		aitype = aitype->Next;
	}

	for (i = 0; i <= UnitTypeMax; i++) {
		if (unitcount[i] > 0) {
			aitype = (AiUnitType *) malloc(sizeof (AiUnitType));
			aitype->Want = unitcount[i];
			aitype->Type = UnitTypes[i];

			// Insert into force.
			aitype->Next = AiPlayer->Force[force].UnitTypes;
			AiPlayer->Force[force].UnitTypes = aitype;
		}
	}
}

/**
**  Create a force full of available units, responding to the powers.
**
**  @param power   Land/Sea/Air power to match
**  @param utypes  array of unittypes to use
**  @param ucount  Size of the utypes array
**
**  @return        -1 if not possible, 0 if force ready.
*/
global int AiCreateSpecificForce(int* power, int* unittypes, int unittypescount)
{
	UnitType *ut;
	int id;
	int maxPower;
	int forceUpdated;
	int curpower[3];
	int maxadd;
	int lefttoadd;
	int unittypeforce;
	int equivalents[UnitTypeMax + 1];
	int equivalentscount;
	int equivalentid;

	curpower[0] = power[0];
	curpower[1] = power[1];
	curpower[2] = power[2];
	AiEraseForce(AiScript->ownForce);



	do {
		forceUpdated = 0;
		maxPower = (curpower[0] > curpower[1] ?
			(curpower[0] > curpower[2] ? 0 : 2) : (curpower[1] > curpower[2] ? 1 : 2));

		for (id = 0; id < unittypescount; id++) {
			// Search in equivalents
			equivalentscount = AiFindAvailableUnitTypeEquiv(UnitTypes[unittypes[id]], equivalents);
			for (equivalentid = 0; equivalentid < equivalentscount; equivalentid++) {
				ut = UnitTypes[equivalents[equivalentid]];
				if (!(ut->CanTarget & (1 << maxPower))) {
					continue;
				}

				unittypeforce = AiUnittypeForce(ut);
				unittypeforce = (unittypeforce ? unittypeforce : 1);

				// Try to respond to the most important power ...
				maxadd = 1 + curpower[maxPower] / unittypeforce;

				lefttoadd = AiEnroleSpecificUnitType(AiScript->ownForce, ut, maxadd);

				// Nothing added, continue.
				if (lefttoadd == maxadd) {
					continue;
				}

				// FIXME : don't always use the right unittype here...
				curpower[maxPower] -= (maxadd - lefttoadd) * unittypeforce;

				forceUpdated = 1;

				maxPower = (curpower[0] > curpower[1] ?
					(curpower[0] > curpower[2] ? 0 : 2) :
					(curpower[1] > curpower[2] ? 1 : 2));
				if (curpower[maxPower] <= 0) {
					AiFinalizeForce(AiScript->ownForce);
					return 0;
				}
			}
		}
	} while (forceUpdated);
	// Sth missing...
	AiFinalizeForce(AiScript->ownForce);
	return -1;
}


/**
**  Assign free units to force.
*/
global void AiAssignFreeUnitsToForce(void)
{
	const AiUnit* aiunit;
	Unit* table[UnitMax];
	Unit* unit;
	int n;
	int f;
	int i;

	AiCleanForces();

	n = AiPlayer->Player->TotalNumUnits;
	memcpy(table, AiPlayer->Player->Units, sizeof(*AiPlayer->Player->Units) * n);

	//
	// Remove all units already in forces.
	//
	for (f = 0; f < AI_MAX_FORCES; ++f) {
		aiunit = AiPlayer->Force[f].Units;
		while (aiunit) {
			unit = aiunit->Unit;
			for (i = 0; i < n; ++i) {
				if (table[i] == unit) {
					table[i] = table[--n];
				}
			}
			aiunit = aiunit->Next;
		}
	}

	//
	// Try to assign the remaining units.
	//
	for (i = 0; i < n; ++i) {
		if (table[i]->Active) {
			AiAssignToForce(table[i]);
		}
	}
}

/**
**  Attack at position with force.
**
**  @param force  Force number to attack with.
**  @param x      X tile map position to be attacked.
**  @param y      Y tile map position to be attacked.
*/
global void AiAttackWithForceAt(int force, int x, int y)
{
	const AiUnit* aiunit;

	AiCleanForce(force);

	if ((aiunit = AiPlayer->Force[force].Units)) {
		AiPlayer->Force[force].Attacking = 1;

		//
		// Send all units in the force to enemy.
		//
		while (aiunit) {
			if (aiunit->Unit->Type->CanAttack) {
				CommandAttack(aiunit->Unit, x, y, NULL, FlushCommands);
			} else {
				CommandMove(aiunit->Unit, x, y, FlushCommands);
			}

			aiunit = aiunit->Next;
		}
	}
}

/**
**  Try to group units in a force. Units are grouped arround the closest units of the hotspot.
**
**  @param force  the force to send home.
*/
global void AiGroupForceNear(int force, int targetX, int targetY)
{
	const AiUnit *aiunit;
	const AiUnit *groupunit;
	int unitdst, groupdst;

	// Step 1 : find the unit closest to the force hotspot
	AiCleanForce(force);

	groupdst = -1;

	groupunit = 0;

	aiunit = AiPlayer->Force[force].Units;

	// Sanity : don't group force with only one unit !
	if ((!aiunit) || (!aiunit->Next)) {
		return;
	}

	while (aiunit) {
		unitdst = abs(aiunit->Unit->X - targetX) + abs(aiunit->Unit->Y - targetY);
		if ((unitdst < groupdst) || (!groupunit)) {
			groupunit = aiunit;
			groupdst = unitdst;
		}

		aiunit = aiunit->Next;
	}

	AiPlayer->Force[force].Attacking = 1;

	// Order units to attack near the "group" unit...
	aiunit = AiPlayer->Force[force].Units;
	while (aiunit) {
		if (aiunit->Unit->Type->CanAttack) {
			CommandAttack(aiunit->Unit, groupunit->Unit->X, groupunit->Unit->Y, NULL,
				FlushCommands);
		} else {
			CommandMove(aiunit->Unit, groupunit->Unit->X, groupunit->Unit->Y,
				FlushCommands);
		}
		aiunit = aiunit->Next;
	}
}

/**
**  Find the closest home batiment.
**  ground is 0 : land, 1 : air, 2 : water ( as UnitType::UnitType )
**
**  @param ground  ground type ( land/air/water )
**  @param x       X start position
**  @param y       Y start position
**  @param rsltx   X destination
**  @param rslyx   Y destination
**
**  @todo  Find in the player unit's the closer to
*/
local void AiFindHome(int ground, int x, int y, int* rsltx, int* rslty)
{
	// Find in the player unit's the closer to
	*rsltx = AiPlayer->Player->StartX;
	*rslty = AiPlayer->Player->StartY;
}

/**
**  Try to send this force home
**
**  @param force  the force to send home.
*/
global void AiSendForceHome(int force)
{
	const AiUnit* aiunit;
	int i;
	int type;
	int x[3];
	int y[3];

	AiCleanForce(force);
	aiunit = AiPlayer->Force[force].Units;

	for (i = 0; i < 3; i++) {
		x[i] = -1;
		y[i] = -1;
	}

	while (aiunit) {
		type = aiunit->Unit->Type->UnitType;

		if (x[type] == -1) {
			AiFindHome(type, aiunit->Unit->X, aiunit->Unit->Y, &x[type], &y[type]);
		}

		CommandMove(aiunit->Unit, x[type], y[type], FlushCommands);

		aiunit = aiunit->Next;
	}
}

/**
**  Ai Force Action when unit/force requires assistance
**
**  @param force     Force Number (FIXME: which?)
**  @param attacker  attacking unit
**  @param defender  defending unit
*/
global void AiForceHelpMe(int force, const Unit* attacker, Unit* defender)
{
	AiForce* aiForce;
	AiUnit* rescue;

	aiForce = AiPlayer->Force + force;

	// Don't handle special cases
	if (aiForce->State > 0) {
		return;
	}

	switch (aiForce->HelpMode) {
		case AiForceDontHelp:
			// Don't react (easy)
			return;

		case AiForceHelpForce:
			// Send all idles units in the attacked force for help
			rescue = aiForce->Units;
			while (rescue) {
				// TODO : check that dead units does appear there
				if (UnitIdle(rescue->Unit)) {
					// This unit attack !
					if (rescue->Unit->Type->CanAttack) {
						CommandAttack(rescue->Unit, attacker->X, attacker->Y, NULL,
							FlushCommands);
					} else {
						CommandMove(rescue->Unit, attacker->X, attacker->Y,
							FlushCommands);
					}
					// Now the force is attacking ( again )
					aiForce->Attacking = 1;
				}
				rescue = rescue->Next;
			}
			break;

		default:
			// the usual way : create a defense force, send it, ...
			AiFindDefendScript(attacker->X, attacker->Y);
			break;
	}
}

/**
**  Entry point of force manager, perodic called.
**
** @todo FIXME : is this really needed anymore
*/
global void AiForceManager(void)
{
	AiAssignFreeUnitsToForce();
}

//@}
