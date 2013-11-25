/*
 *  Combat.c
 *  Brogue
 *
 *  Created by Brian Walker on 6/11/09.
 *  Copyright 2010. All rights reserved.
 *  
 *  This file is part of Brogue.
 *
 *  Brogue is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Brogue is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Brogue.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include "Rogue.h"
#include "IncludeGlobals.h"


/* Combat rules:
 * Each combatant has an accuracy rating. This is the percentage of their attacks that will ordinarily hit;
 * higher numbers are better for them. Numbers over 100 are permitted.
 *
 * Each combatant also has a defense rating. The "hit probability" is calculated as given by this formula:
 * 
 * 							hit probability = (accuracy) * 0.986 ^ (defense)
 * 
 * when hit determinations are made. Negative numbers and numbers over 100 are permitted.
 * The hit is then randomly determined according to this final percentage.
 *
 * Some environmental factors can modify these numbers. An unaware, sleeping or stuck enemy is always hit.
 *
 * If the hit lands, damage is calculated in the range provided. However, the clumping factor affects the
 * probability distribution. If the range is 0-10 with a clumping factor of 1, it's a uniform distribution.
 * With a clumping factor of 2, it's calculated as 2d5 (with d5 meaing a die numbered from 0 through 5).
 * With 3, it's 3d3, and so on. Note that a range not divisible by the clumping factor is defective,
 * as it will never be resolved in the top few numbers of the range. In fact, the top
 * (rangeWidth % clumpingFactor) will never succeed. Thus we increment the maximum of the first
 * (rangeWidth % clumpingFactor) dice by 1, so that in fact 0-10 with a CF of 3 would be 1d4 + 2d3. Similarly,
 * 0-10 with CF 4 would be 2d3 + 2d2. By playing with the numbers, one can approximate a gaussian
 * distribution of any mean and standard deviation.
 *
 * Player combatants take their base defense value of their actual armor. Their accuracy is a combination of weapon, armor
 * and strength. Each weapon and armor has a strength factor, and each point short of that number means an marginal -15
 * accuracy for the player.
 * 
 * Players have a base accuracy value of 75. This goes up by 10 with each experience level gained past 1.
 */

float strengthModifier(item *theItem) {
	short difference = rogue.strength - theItem->strengthRequired;
	if (difference > 0) {
		return (float) 0.25 * difference;
	} else {
		return (float) 2.5 * difference;
	}
}

float netEnchant(item *theItem) {
	if (rogue.armor == theItem || rogue.weapon == theItem) {
		return theItem->enchant1 + strengthModifier(theItem);
	} else {
		return theItem->enchant1;
	}
}

// does NOT account for auto-hit from sleeping or unaware defenders; does account for auto-hit from
// stuck or captive defenders.
short hitProbability(creature *attacker, creature *defender) {
	short accuracy = attacker->info.accuracy;
	short defense = defender->info.defense;
	short hitProbability;
	
	if (defender->status.stuck || (defender->bookkeepingFlags & MONST_CAPTIVE)) {
		return 100;
	}
	
	if (attacker == &player && rogue.weapon) {
		accuracy += 5 * netEnchant(rogue.weapon);
	}
	
	if (defender == &player && rogue.armor) {
		defense += 10 * strengthModifier(rogue.armor);
	}
	
	hitProbability = accuracy * pow(DEFENSE_RADICAND, defense);
	
	if (hitProbability > 100) {
		hitProbability = 100;
	} else if (hitProbability < 0) {
		hitProbability = 0;
	}
	
	return hitProbability;
}

boolean attackHit(creature *attacker, creature *defender) {
	
	// automatically hit if the monster is sleeping or captive or stuck in a web
	if (defender->status.stuck || defender->bookkeepingFlags & MONST_CAPTIVE) {
		return true;
	}
	
	return rand_percent(hitProbability(attacker, defender));
}

void monsterShoots(creature *attacker, short targetLoc[2], uchar projChar, color *projColor) {
	short listOfCoordinates[MAX_BOLT_LENGTH][2], originLoc[2];
	short i, x, y, numCells;
	creature *monst;
	uchar displayChar;
	color foreColor, backColor, multColor;
	boolean fastForward = false;
	char monstName[DCOLS], buf[DCOLS];
	
	originLoc[0] = attacker->xLoc;
	originLoc[1] = attacker->yLoc;
	
	numCells = getLineCoordinates(listOfCoordinates, originLoc, targetLoc);
	
	attacker->ticksUntilTurn = attacker->attackSpeed;
	
	for (i=0; i<numCells; i++) {
		
		x = listOfCoordinates[i][0];
		y = listOfCoordinates[i][1];
		
		if (cellHasTerrainFlag(x, y, (OBSTRUCTS_PASSABILITY | OBSTRUCTS_VISION))) {
			break;
		}
		
		if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
			monst = monsterAtLoc(x, y);
			
			if (monst && projectileReflects(attacker, monst) && i < DCOLS*2) {
				if (projectileReflects(attacker, monst)) { // if it scores another reflection roll, reflect at caster
					numCells = reflectBolt(originLoc[0], originLoc[1], listOfCoordinates, i, true);
				} else {
					numCells = reflectBolt(-1, -1, listOfCoordinates, i, false); // otherwise reflect randomly
				}
				
				monsterName(monstName, monst, true);
				sprintf(buf, "%s deflect%s the arrow", monstName, (monst == &player ? "" : "s"));
				combatMessage(buf);
				continue;
			}
			
			attack(attacker, monst);
			break;
		}
		
		if (playerCanSee(x, y)) {
			getCellAppearance(x, y, &displayChar, &foreColor, &backColor);
			foreColor = gray;
			colorMultiplierFromDungeonLight(x, y, &multColor);
			applyColorMultiplier(&foreColor, &multColor);
			plotCharWithColor(projChar, x + STAT_BAR_WIDTH, y + MESSAGE_LINES, *projColor, backColor);
			
			if (!fastForward) {
				fastForward = pauseBrogue(50);
			}
			
			refreshDungeonCell(x, y);
		}
	}
}

void shootWeb(creature *breather, short targetLoc[2], short kindOfWeb) {
	short listOfCoordinates[DCOLS][2], originLoc[2];
	short i, x, y, numCells;
	char buf[COLS], buf2[COLS];
	boolean fastForward = false;
	dungeonFeature smallWeb = {kindOfWeb, SURFACE, {15,15,1}, {12,12,1}, false};
	dungeonFeature largeWeb = {kindOfWeb, SURFACE, {100,100,1}, {39,39,1}, false};
	originLoc[0] = breather->xLoc;
	originLoc[1] = breather->yLoc;
	
	spawnDungeonFeature(originLoc[0], originLoc[1], &smallWeb, true);
	
	numCells = getLineCoordinates(listOfCoordinates, originLoc, targetLoc);
	
	if (pmap[originLoc[0]][originLoc[1]].flags & IN_FIELD_OF_VIEW) {
		monsterName(buf2, breather, true);
		sprintf(buf, "%s shoots a sticky web.", buf2);
		message(buf, true, false);
	}
	
	for (i=0; i<numCells; i++) {
		
		x = listOfCoordinates[i][0];
		y = listOfCoordinates[i][1];
		
		if (cellHasTerrainFlag(x, y, (OBSTRUCTS_PASSABILITY | OBSTRUCTS_VISION | IS_FIRE))) {
			break;
		}
		
		if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
			spawnDungeonFeature(x, y, &largeWeb, true);
			break;
		} else {
			spawnDungeonFeature(x, y, &smallWeb, true);
		}
		if (!fastForward) {
			fastForward = pauseBrogue(50);
		}
	}
}

void specialHit(creature *attacker, creature *defender, short damage) {
	short itemCandidates, randItemIndex;
	item *theItem, *previousItem, *itemFromTopOfStack;
	char buf[COLS], buf2[COLS], buf3[COLS];
	
	if (!(attacker->info.abilityFlags & SPECIAL_HIT)) {
		return;
	}
	
	// special hits that can affect only the player:
	if (defender == &player) {
		if (attacker->info.abilityFlags & MA_HIT_DEGRADE_ARMOR
			&& defender == &player
			&& rogue.armor
			&& !(rogue.armor->flags & ITEM_PROTECTED)) {
			
			rogue.armor->enchant1--;
			equipItem(rogue.armor, true);
			itemName(rogue.armor, buf2, false, false);
			sprintf(buf, "your %s weakens!", buf2);
			message(buf, true, false);
		}
		if (attacker->info.abilityFlags & MA_HIT_HALLUCINATE) {
			if (!player.status.hallucinating) {
				combatMessage("you begin to hallucinate");
			}
			if (!player.status.hallucinating) {
				player.maxStatus.hallucinating = 0;
			}
			player.status.hallucinating += 20;
			player.maxStatus.hallucinating = max(player.maxStatus.hallucinating, player.status.hallucinating);
		}
		
		if (attacker->info.abilityFlags & MA_HIT_STEAL_FLEE
			&& !(attacker->carriedItem)
			&& (packItems->nextItem)
			&& attacker->currentHP > 0
			&& attackHit(attacker, defender)) {
			
			itemCandidates = numberOfMatchingPackItems(ALL_ITEMS & ~AMULET, 0, (ITEM_EQUIPPED | ITEM_CURSED), false);
			if (itemCandidates) {
				randItemIndex = rand_range(1, itemCandidates);
				for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
					if (!(theItem->flags & (ITEM_CURSED | ITEM_EQUIPPED))) {
						if (randItemIndex == 1) {
							break;
						} else {
							randItemIndex--;
						}
					}
				}
				
				if (theItem->quantity > 1 && !(theItem->category & WEAPON)) { // peel off the top item and drop it
					itemFromTopOfStack = generateItem(-1, -1);
					*itemFromTopOfStack = *theItem; // clone the item
					theItem->quantity--;
					itemFromTopOfStack->quantity = 1;
					theItem = itemFromTopOfStack; // redirect pointer
				} else {
					for (previousItem = packItems; previousItem->nextItem != theItem; previousItem = previousItem->nextItem);
					previousItem->nextItem = theItem->nextItem;
				}
				attacker->carriedItem = theItem;
				attacker->creatureMode = MODE_PERM_FLEEING;
				attacker->creatureState = MONSTER_FLEEING;
				chooseNewWanderDestination(attacker);
				monsterName(buf2, attacker, true);
				itemName(theItem, buf3, false, true);
				sprintf(buf, "%s stole %s!", buf2, buf3);
				message(buf, true, false);
			}
		}
	}
	if (attacker->info.abilityFlags & MA_POISONS && damage > 0) {
		if (defender == &player && !player.status.poisoned) {
			combatMessage("scalding poison fills your veins");
		}
		if (!defender->status.poisoned) {
			defender->maxStatus.poisoned = 0;
		}
		defender->status.poisoned = max(defender->status.poisoned, damage);
		defender->maxStatus.poisoned = max(defender->maxStatus.poisoned, defender->status.poisoned);
	}
}

short runicWeaponChance(item *theItem, boolean customEnchantLevel, float enchantLevel) {
	
	// W_POISON, W_QUIETUS, W_PARALYSIS, W_NEGATION, W_SLOWING, W_CONFUSION, W_SLAYING, W_MERCY, W_PLENTY
	
	const short effectChances[NUMBER_WEAPON_RUNIC_KINDS][2] = {
		{25, 130},
		{5, 60},
		{7, 65},
		{20, 180},
		{10, 75},
		{7, 70},
		{0, 0},
		{15, 15},
		{15, 15}};
	short runicType = theItem->enchant2;
	short chance;
	
	if (customEnchantLevel) {
		enchantLevel = netEnchant(theItem);
	}
	chance = max(effectChances[runicType][0],
			   effectChances[runicType][0] +
			   (effectChances[runicType][1] - effectChances[runicType][0]) * enchantLevel / 10);
	
	// innately high-damage weapon types have much less likelihood of runic effects actuating
	chance = chance * (100 - 100 * ((theItem->damage.lowerBound + theItem->damage.upperBound) / 2 - 3) / (20 - 3)) / 100;
	
	return chance;
}

void magicWeaponHit(creature *defender, item *theItem, boolean backstabbed) {
	char buf[DCOLS*3], monstName[DCOLS];
	// lower is chance at weapon level 0, upper is chance at weapon level 10, to be interpolated/extrapolated as necessary
	color *effectColors[NUMBER_WEAPON_RUNIC_KINDS] = {&darkGreen, &black,
		&yellow, &pink, &green, &confusionGasColor, &white, &darkRed, &rainbow};
	//	W_POISON, W_QUIETUS, W_PARALYSIS, W_NEGATION, W_SLOWING, W_CONFUSION, W_SLAYING, W_MERCY, W_PLENTY
	short chance;
	float enchant;
	enum weaponEnchants enchantType = theItem->enchant2;
	creature *newMonst;
	
	enchant = netEnchant(theItem);
	
	if (theItem->enchant2 == W_SLAYING) {
		chance = (theItem->vorpalEnemy == defender->info.monsterID ? 100 : 0);
	} else {
		chance = runicWeaponChance(theItem, false, 0);
		if (backstabbed) {
			chance *= 2;
		}
	}
	if (!(defender->info.flags & MONST_INANIMATE) && rand_percent(chance)) {
		//defender->bookkeepingFlags &= ~MONST_WILL_FLASH; // no need to flash red too
		//flash(effectColors[enchantType], 15, defender->xLoc, defender->yLoc);
		if (!(defender->bookkeepingFlags & MONST_SUBMERGED)) {
			flashMonster(defender, effectColors[enchantType], 100);
			theItem->flags |= (ITEM_RUNIC_IDENTIFIED | ITEM_RUNIC_HINTED);
		}
		rogue.disturbed = true;
		monsterName(monstName, defender, true);
		itemName(theItem, buf, false, false);
		
		switch (enchantType) {
			case W_POISON:
				defender->status.poisoned = max(defender->status.poisoned, max(5, (int) 10 * pow(1.53, enchant - 2)));
				defender->maxStatus.poisoned = defender->status.poisoned;
				sprintf(buf, "%s is poisoned", monstName);
				buf[DCOLS] = '\0';
				combatMessage(buf);
				break;
			case W_SLAYING:
			case W_QUIETUS:
				inflictDamage(&player, defender, defender->currentHP, 0);
				sprintf(buf, "%s suddenly dies", monstName);
				buf[DCOLS] = '\0';
				combatMessage(buf);
				break;
			case W_PARALYSIS:
				defender->status.paralyzed = max(defender->status.paralyzed, max(2, 2 + enchant));
				defender->maxStatus.paralyzed = defender->status.paralyzed;
				sprintf(buf, "%s is frozen in place", monstName);
				buf[DCOLS] = '\0';
				combatMessage(buf);
				break;
			case W_NEGATION:
				sprintf(buf, "%s is swept by cancellation magic", monstName);
				buf[DCOLS] = '\0';
				combatMessage(buf);
				cancelMagic(defender);
				break;
			case W_SLOWING:
				slow(defender, enchant * 3);
				sprintf(buf, "%s slows down", monstName);
				buf[DCOLS] = '\0';
				combatMessage(buf);
				break;
			case W_CONFUSION:
				defender->status.confused = max(defender->status.confused, max(3, 2 * enchant));
				defender->maxStatus.confused = defender->status.confused;
				sprintf(buf, "%s looks very confused", monstName);
				buf[DCOLS] = '\0';
				combatMessage(buf);
				break;
			case W_MERCY:
				heal(defender, 50);
				break;
			case W_PLENTY:
				newMonst = cloneMonster(defender);
				if (newMonst) {
					flashMonster(newMonst, effectColors[enchantType], 100);
				}
				break;
			default:
				break;
		}
	}
}

void attackVerb(char returnString[DCOLS], creature *attacker, short hitPercentile) {
	short verbCount, increment;
	
	if (attacker != &player && (player.status.hallucinating || !canSeeMonster(attacker))) {
		strcpy(returnString, "hits");
		return;
	}
	
	for (verbCount = 0; verbCount < 4 && monsterText[attacker->info.monsterID].attack[verbCount + 1][0] != '\0'; verbCount++);
	increment = (100 / (verbCount + 1));
	hitPercentile = max(0, min(hitPercentile, increment * (verbCount + 1) - 1));
	strcpy(returnString, monsterText[attacker->info.monsterID].attack[hitPercentile / increment]);
}

void applyArmorRunicEffect(char returnString[DCOLS], creature *attacker, short *damage, boolean melee) {
	char armorName[DCOLS], attackerName[DCOLS];
	boolean runicKnown;
	boolean runicDiscovered;
	short newDamage;
	float enchant;
	
	returnString[0] = '\0';
	
	if (!(rogue.armor && rogue.armor->flags & ITEM_RUNIC)) {
		return; // just in case
	}
	
	enchant = netEnchant(rogue.armor);
	
	runicKnown = rogue.armor->flags & ITEM_RUNIC_IDENTIFIED;
	runicDiscovered = false;
	
	itemName(rogue.armor, armorName, false, false);
	
	switch (rogue.armor->enchant2) {
		case A_ABSORPTION:
			*damage -= randClumpedRange(0, (short) enchant, (short) enchant / 2);
			if (*damage <= 0) {
				*damage = 0;
				if (!runicKnown) {
					sprintf(returnString, "your %s pulses and absorbs the blow!", armorName);
				}
				runicDiscovered = true;
			}
			break;
		case A_REPRISAL:
			if (melee && !(attacker->info.flags & MONST_INANIMATE)) {
				monsterName(attackerName, attacker, true);
				newDamage = max(1, enchant * (*damage) / 10); // 10% reprisal per armor level
				if (inflictDamage(&player, attacker, newDamage, &blue)) {
					sprintf(returnString, "your %s pulses and %s drops dead!", armorName, attackerName);
				} else if (!runicKnown) {
					sprintf(returnString, "your %s pulses and %s shudders in pain!", armorName, attackerName);
				}
				runicDiscovered = true;
			}
			break;
		case A_IMMUNITY:
			if (rogue.armor->vorpalEnemy == attacker->info.monsterID) {
				*damage = 0;
				runicDiscovered = true;
			}
			break;
		case A_BURDEN:
			if (rand_percent(10)) {
				rogue.armor->strengthRequired++;
				sprintf(returnString, "your %s suddenly feels heavier!", armorName);
				equipItem(rogue.armor, true);
				runicDiscovered = true;
			}
			break;
		case A_VULNERABILITY:
			*damage *= 2;
			if (!runicKnown) {
				sprintf(returnString, "your %s pulses and you are wracked with pain!", armorName);
				runicDiscovered = true;
			}
			break;
		default:
			break;
	}
	
	if (runicDiscovered && !runicKnown) {
		rogue.armor->flags |= (ITEM_RUNIC_IDENTIFIED | ITEM_RUNIC_HINTED);
		if (rogue.armor->flags & ITEM_IDENTIFIED) {
			rogue.armor->flags &= ~ITEM_CAN_BE_IDENTIFIED;
		}
	}
}

// returns whether the attack hit
boolean attack(creature *attacker, creature *defender) {
	short damage, transferenceAmount, poisonDamage;
	char buf[COLS], buf2[COLS], attackerName[COLS], defenderName[COLS], verb[DCOLS], explicationClause[DCOLS] = "", armorRunicString[DCOLS];
	boolean sneakAttack, defenderWasAsleep, degradesAttackerWeapon, sightUnseen;
	
	if (attacker->info.abilityFlags & MA_KAMIKAZE) {
		addExperience(attacker->info.expForKilling);
		killCreature(attacker);
		return true;
	}
	
	armorRunicString[0] = '\0';
	
	poisonDamage = 0;
	
	degradesAttackerWeapon = (defender->info.flags & MONST_DEFEND_DEGRADE_WEAPON ? true : false);
	
	sightUnseen = !(canSeeMonster(attacker) || canSeeMonster(defender));
	
	if (defender->status.levitating && (attacker->info.flags & MONST_RESTRICTED_TO_LIQUID)) {
		return false; // aquatic or other liquid-bound monsters cannot attack flying opponents
	}
	
	if ((attacker == &player || defender == &player) && !rogue.blockCombatText) {
		rogue.disturbed = true;
	}
	
	defender->status.entranced = 0;
	if (defender->status.magicalFear) {
		defender->status.magicalFear = 1;
	}
	
	if (attacker != &player && defender == &player && attacker->creatureState == MONSTER_WANDERING) {
		attacker->creatureState = MONSTER_TRACKING_SCENT;
	}
	
	sneakAttack = (defender != &player && attacker == &player && (defender->creatureState == MONSTER_WANDERING) ? true : false);
	defenderWasAsleep = (defender != &player && (defender->creatureState == MONSTER_SLEEPING) ? true : false);
	
	monsterName(attackerName, attacker, true);
	monsterName(defenderName, defender, true);
	
	if (attacker->info.abilityFlags & MA_SEIZES && (!(attacker->bookkeepingFlags & MONST_SEIZING) || !(defender->bookkeepingFlags & MONST_SEIZED))) {
		attacker->bookkeepingFlags |= MONST_SEIZING;
		defender->bookkeepingFlags |= MONST_SEIZED;
		if (canSeeMonster(attacker) || canSeeMonster(defender)) {
			sprintf(buf, "%s seizes %s!", attackerName, (defender == &player ? "your legs" : defenderName));
			message(buf, true, false);
		}
		return false;
	}
	
	if (sneakAttack || defenderWasAsleep || attackHit(attacker, defender)) {
		
		damage = (defender->info.flags & MONST_IMMUNE_TO_WEAPONS ? 0 : randClump(attacker->info.damage));
		
		if (sneakAttack || defenderWasAsleep) {
			// won't do anything this turn because he's still flat-footed, so he doesn't hit back this turn
			defender->ticksUntilTurn += max(defender->movementSpeed, defender->attackSpeed);
			defender->creatureState = MONSTER_TRACKING_SCENT; // wake up
			damage *= 3; // treble damage when attacking a sleeping monster
		}
		
		if (defender == &player && rogue.depthLevel == 1) {
			damage = max(damage/2, 1); // player takes half damage on depth 1
		}
		
		damage = min(damage, defender->currentHP);
		
		if (attacker == &player && rogue.transference && !(defender->info.flags & MONST_INANIMATE)) {
			transferenceAmount = damage * rogue.transference / 10;
			if (transferenceAmount == 0) {
				transferenceAmount = ((rogue.transference > 0) ? 1 : -1);
			}
			player.currentHP += min((player.info.maxHP - player.currentHP), transferenceAmount);
			
			if (player.currentHP <= 0) {
				gameOver("Drained by a cursed ring", true);
				return false;
			}
		}
		
		if (defender == &player && rogue.armor && (rogue.armor->flags & ITEM_RUNIC)) {
			applyArmorRunicEffect(armorRunicString, attacker, &damage, true);
		}
		
		if (damage == 0) {
			sprintf(explicationClause, " but %s no damage", (attacker == &player ? "do" : "does"));
			if (attacker == &player) {
				rogue.disturbed = true;
			}
		} else if (defenderWasAsleep) {
			sprintf(explicationClause, " while %s sleep%s", monsterText[defender->info.monsterID].pronoun,
					(defender == &player ? "" : "s"));
		} else if (sneakAttack) {
			strcpy(explicationClause, " from the shadows");
		} else if (defender->status.stuck || defender->bookkeepingFlags & MONST_CAPTIVE) {
			sprintf(explicationClause, " while %s dangle%s helplessly",
					(canSeeMonster(defender) ? monsterText[defender->info.monsterID].pronoun : "it"),
					(defender == &player ? "" : "s"));
		}
		
		if ((attacker->info.abilityFlags & MA_POISONS) && damage > 0) {
			poisonDamage = damage;
			damage = 1;
		}
		
		if (inflictDamage(attacker, defender, damage, &red)) { // if the attack killed the defender
			if (defenderWasAsleep || sneakAttack) {
				sprintf(buf, "%s dispatched %s%s", attackerName, defenderName, explicationClause);
			} else {
				sprintf(buf, "%s defeated %s%s", attackerName, defenderName, explicationClause);
			}
			if (sightUnseen) {
				combatMessage("you hear something die in combat");
			} else {
				combatMessage(buf);
			}
			if (&player == defender) {
				gameOver(attacker->info.monsterName, false);
				return true;
			}
			if (&player == attacker && defender->info.expForKilling > 0 && rogue.weapon
				&& !(rogue.weapon->flags & ITEM_IDENTIFIED) && !--rogue.weapon->charges) {
				rogue.weapon->flags |= ITEM_IDENTIFIED;
				if (!(rogue.weapon->flags & ITEM_RUNIC) || (rogue.weapon->flags & ITEM_RUNIC_IDENTIFIED)) {
					rogue.weapon->flags &= ~ITEM_CAN_BE_IDENTIFIED;
				}
				message("you are now familiar enough with your weapon to identify it.", true, false);
				itemName(rogue.weapon, buf2, true, true);
				sprintf(buf, "%s %s.", (rogue.weapon->quantity > 1 ? "they are" : "it is"), buf2);
				message(buf, true, false);
			}
		} else { // if the defender survived
			if (!rogue.blockCombatText && (canSeeMonster(attacker) || canSeeMonster(defender))) {
				attackVerb(verb, attacker, max(damage - attacker->info.damage.lowerBound, 0) * 100
						   / max(1, attacker->info.damage.upperBound - attacker->info.damage.lowerBound));
				sprintf(buf, "%s %s %s%s", attackerName, verb, defenderName, explicationClause);
				if (sightUnseen) {
					if (!rogue.heardCombatThisTurn) {
						rogue.heardCombatThisTurn = true;
						combatMessage("you hear combat in the distance");
					}
				} else {
					combatMessage(buf);
				}
			}
			if (attacker->info.abilityFlags & SPECIAL_HIT) {
				specialHit(attacker, defender, (attacker->info.abilityFlags & MA_POISONS) ? poisonDamage : damage);
			}
			if (attacker == &player && rogue.weapon && (rogue.weapon->flags & ITEM_RUNIC)) {
				magicWeaponHit(defender, rogue.weapon, sneakAttack || defenderWasAsleep);
			}
			if (armorRunicString[0]) {
				message(armorRunicString, true, false);
				if (rogue.armor && (rogue.armor->flags & ITEM_RUNIC) && rogue.armor->enchant2 == A_BURDEN) {
					strengthCheck(rogue.armor);
				}
			}
			if (defender->currentHP > 0 && defender->info.abilityFlags & MA_CLONE_SELF_ON_DEFEND) {
				fadeInMonster(cloneMonster(defender));
				if (canSeeMonster(defender)) {
					sprintf(buf, "%s splits in two!", defenderName);
					message(buf, true, false);
				}
			}
		}
		
		if (degradesAttackerWeapon && attacker == &player && rogue.weapon && !(rogue.weapon->flags & ITEM_PROTECTED)) {
			rogue.weapon->enchant1--;
			equipItem(rogue.weapon, true);
			itemName(rogue.weapon, buf2, false, false);
			sprintf(buf, "your %s weakens!", buf2);
			message(buf, true, false);
		}
		
		return true;
	} else { // if the attack missed
		if (!rogue.blockCombatText) {
			if (sightUnseen) {
				if (!rogue.heardCombatThisTurn) {
					rogue.heardCombatThisTurn = true;
					combatMessage("you hear combat in the distance");
				}
			} else {
				sprintf(buf, "%s missed %s", attackerName, defenderName);
				combatMessage(buf);
			}
		}
		return false;
	}
}

void addExperience(unsigned long exp) {	
	rogue.experience += exp;
	if (rogue.experience >= levelPoints[rogue.experienceLevel - 1] && rogue.experienceLevel < MAX_EXP_LEVEL) {
		while (rogue.experience >= levelPoints[rogue.experienceLevel - 1] && rogue.experienceLevel < MAX_EXP_LEVEL) {
			rogue.experienceLevel++;
			player.info.maxHP += 5;
			player.currentHP += (5 * player.currentHP / (player.info.maxHP - 5));
			updatePlayerRegenerationDelay();
			player.info.accuracy += 10;
		}
		rogue.gainedLevel = true;
	}
}

void combatMessage(char *theMsg) {
	if (strlen(combatText) + strlen(theMsg) + 3 > DCOLS) {
		displayCombatText();
	}
	
	if (strlen(combatText) > 0) {
		sprintf(combatText, "%s; %s", combatText, theMsg);
	} else {
		strcpy(combatText, theMsg);
	}
}

void displayCombatText() {
	char buf[COLS];
	
	if (strlen(combatText) > 0) {
		sprintf(buf, "%s.", combatText);
		combatText[0] = '\0';
		message(buf, true, rogue.cautiousMode);
		rogue.cautiousMode = false;
	}
}

void flashMonster(creature *monst, color *theColor, short strength) {
	if (!(monst->bookkeepingFlags & MONST_WILL_FLASH) || monst->flashStrength < strength) {
		monst->bookkeepingFlags |= MONST_WILL_FLASH;
		monst->flashStrength = strength;
		monst->flashColor = *theColor;
		rogue.creaturesWillFlashThisTurn = true;
	}
}

boolean canAbsorb(creature *ally, creature *prey, short **grid) {
	return (ally->creatureState == MONSTER_ALLY
			&& ally->absorbsAllowed >= 2000
			&& (ally->targetCorpseLoc[0] <= 0)
			&& monstersAreEnemies(ally, prey)
			&& ((~(ally->info.abilityFlags) & prey->info.abilityFlags & LEARNABLE_ABILITIES)
				|| (~(ally->info.flags) & prey->info.flags & LEARNABLE_BEHAVIORS))
			&& !((ally->info.flags | prey->info.flags) & (MONST_INANIMATE | MONST_IMMOBILE))
			&& !monsterAvoids(ally, prey->xLoc, prey->yLoc)
			&& grid[ally->xLoc][ally->yLoc] <= 10);
}

boolean anyoneWantABite(creature *decedent) {
	short candidates, randIndex, i;
	short **grid;
	creature *ally;
	
	candidates = 0;
	if ((!(decedent->info.abilityFlags & LEARNABLE_ABILITIES)
		 && !(decedent->info.flags & LEARNABLE_BEHAVIORS))
		|| (cellHasTerrainFlag(decedent->xLoc, decedent->yLoc, PATHING_BLOCKER))
		|| (decedent->info.flags & (MONST_INANIMATE | MONST_IMMOBILE))) {
		return false;
	}
	grid = allocDynamicGrid();
	calculateDistances(grid, decedent->xLoc, decedent->yLoc, PATHING_BLOCKER, 0);
	for (ally = monsters->nextCreature; ally != NULL; ally = ally->nextCreature) {
		if (canAbsorb(ally, decedent, grid)) {
			candidates++;
		}
	}
	if (candidates > 0) {
		randIndex = rand_range(1, candidates);
		for (ally = monsters->nextCreature; ally != NULL; ally = ally->nextCreature) {
			if (canAbsorb(ally, decedent, grid) && !--randIndex) {
				break;
			}
		}
		if (ally) {
			ally->targetCorpseLoc[0] = decedent->xLoc;
			ally->targetCorpseLoc[1] = decedent->yLoc;
			strcpy(ally->targetCorpseName, decedent->info.monsterName);
			ally->corpseAbsorptionCounter = 20; // 20 turns to get there and start eating before he loses interest
			
			// choose a superpower
			candidates = 0;
			for (i=0; i<32; i++) {
				if (Fl(i) & ~(ally->info.abilityFlags) & decedent->info.abilityFlags & LEARNABLE_ABILITIES) {
					candidates++;
				}
			}
			for (i=0; i<32; i++) {
				if (Fl(i) & ~(ally->info.flags) & decedent->info.flags & LEARNABLE_BEHAVIORS) {
					candidates++;
				}
			}
			if (candidates > 0) {
				randIndex = rand_range(1, candidates);
				for (i=0; i<32; i++) {
					if ((Fl(i) & ~(ally->info.abilityFlags) & decedent->info.abilityFlags & LEARNABLE_ABILITIES)
						&& !--randIndex) {
						ally->absorptionFlags = Fl(i);
						ally->absorbBehavior = false;
						freeDynamicGrid(grid);
						return true;
					}
				}
				for (i=0; i<32; i++) {
					if ((Fl(i) & ~(ally->info.flags) & decedent->info.flags & LEARNABLE_BEHAVIORS)
						&& !--randIndex) {
						ally->absorptionFlags = Fl(i);
						ally->absorbBehavior = true;
						freeDynamicGrid(grid);
						return true;
					}
				}
			}
		}
	}
	freeDynamicGrid(grid);
	return false;
}

#define MIN_FLASH_STRENGTH	30

// returns true if this was a killing stroke; does NOT free the pointer, but DOES remove it from the monster chain
// flashColor indicates the color that the damage will cause the creature to flash
boolean inflictDamage(creature *attacker, creature *defender, short damage, color *flashColor) {
	dungeonFeature theBlood;
	
	if (damage == 0) {
		return false;
	}
	
	// bleed all over the place, proportionately to damage inflicted:
	if (damage > 0 && defender->info.bloodType) {
		theBlood = dungeonFeatureCatalog[defender->info.bloodType];
		theBlood.startProbability.lowerBound = (theBlood.startProbability.lowerBound * (15 + damage * 3 / 2) / 100);
		theBlood.startProbability.upperBound = (theBlood.startProbability.upperBound * (15 + damage * 3 / 2) / 100);
		if (theBlood.layer == GAS) {
			theBlood.startProbability.lowerBound *= 100;
			theBlood.startProbability.upperBound *= 100;
		}
		spawnDungeonFeature(defender->xLoc, defender->yLoc, &theBlood, true);
	}
	
	if (defender != &player && defender->creatureState == MONSTER_SLEEPING) {
		wakeUp(defender);
		chooseNewWanderDestination(defender);
	} else if (defender != &player && attacker == &player && defender->creatureState == MONSTER_ALLY) {
		// totally not your friend anymore
		unAlly(defender);
	}
	
	if (defender == &player && rogue.easyMode) {
		damage = max(1, damage/5);
	}
	
	if (defender->currentHP <= damage) { // killed
		if (defender->carriedItem) {
			makeMonsterDropItem(defender);
		}
		anyoneWantABite(defender);
		killCreature(defender);
		addExperience(defender->info.expForKilling);
		return true;
	}
	
	if (damage < 0 && defender->currentHP - damage > defender->info.maxHP) {
		defender->currentHP = max(defender->currentHP, defender->info.maxHP);
	} else {
		defender->currentHP -= damage; // inflict the damage!
	}
	
	if (defender != &player && defender->creatureState != MONSTER_ALLY
		&& defender->info.flags & MONST_FLEES_NEAR_DEATH
		&& defender->info.maxHP / 4 >= defender->currentHP) {
		defender->creatureState = MONSTER_FLEEING;
		chooseNewWanderDestination(defender);
	}
	if (flashColor && damage > 0) {
		flashMonster(defender, flashColor, MIN_FLASH_STRENGTH + (100 - MIN_FLASH_STRENGTH) * damage / defender->info.maxHP);
	}
	return false;
}

// removes the decedent from the screen and from the monster chain; does NOT free the memory.
void killCreature(creature *decedent) {
	short x, y;
	lightSource *theLight;
	creature *previousCreature;
	char monstName[DCOLS], buf[DCOLS];
	
	if (decedent->info.abilityFlags & MA_DF_ON_DEATH) {
		spawnDungeonFeature(decedent->xLoc, decedent->yLoc, &dungeonFeatureCatalog[decedent->info.DFType], true);
		
		if (monsterText[decedent->info.monsterID].DFMessage && canSeeMonster(decedent)) {
			monsterName(monstName, decedent, true);
			sprintf(buf, "%s %s", monstName, monsterText[decedent->info.monsterID].DFMessage);
			message(buf, true, false);
		}
	}
	
	if (decedent == &player) { // the player died
		// game over handled elsewhere
	} else {
		if (decedent->creatureState == MONSTER_ALLY && !canSeeMonster(decedent) && !(decedent->info.flags & MONST_INANIMATE)) {
			message("you feel a sense of loss.", true, false);
		}
		x = decedent->xLoc;
		y = decedent->yLoc;
		pmap[x][y].flags &= ~HAS_MONSTER;
		for (previousCreature = monsters;
			 previousCreature->nextCreature != decedent;
			 previousCreature = previousCreature->nextCreature);
		previousCreature->nextCreature = decedent->nextCreature;
		decedent->nextCreature = graveyard->nextCreature;
		graveyard->nextCreature = decedent;
		refreshDungeonCell(x, y);
		for (theLight = lights->nextLight; theLight != NULL; theLight = theLight->nextLight) {
			if (theLight->followsCreature == decedent) {
				deleteLight(theLight);
				theLight = lights;
			}
		}
	}
	decedent->currentHP = 0;
	if (decedent->bookkeepingFlags & MONST_LEADER) {
		demoteMonsterFromLeadership(decedent);
	}
}

// basically runs a simplified deterministic combat simulation against a hypothetical
// monster with infinite HP (the dummy) and returns the amount of damage the tested
// monster deals before succumbing.
short monsterPower(creature *theMonst) {
	short damageDealt[2] = {0, 0};
	short turnsParalyzed[2] = {theMonst->status.paralyzed, 0};
	short turnsPoisoned[2] = {theMonst->status.poisoned, 0};
	short k;
	double damagePerHit[2];
	double hitChance[2];
	
	damagePerHit[0] = (theMonst->info.damage.lowerBound + theMonst->info.damage.upperBound) / 2 * 100 / (theMonst->info.attackSpeed);
	damagePerHit[1] = 10;
	hitChance[0] = theMonst->info.accuracy * pow(DEFENSE_RADICAND, 100); // assumes the dummy has 100 armor
	hitChance[1] = theMonst->info.accuracy * pow(DEFENSE_RADICAND, theMonst->info.defense);
	turnsParalyzed[1] += theMonst->status.hasted / 2;

	
	while (damageDealt[1] < theMonst->currentHP) {
		for (k=0; k<=1; k++) { // k is whose turn it is
			if (turnsPoisoned[!k]) {
				damageDealt[k]++;
				turnsPoisoned[!k]--;
			}
			
			if (turnsParalyzed[k]) {
				turnsParalyzed[k]--;
			} else {
				damageDealt[k] += damagePerHit[k] * hitChance[k];
			}
			
		}
	}
	
	return damageDealt[0];
}
