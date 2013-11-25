/*
 *  Combat.c
 *  Brogue
 *
 *  Created by Brian Walker on 6/11/09.
 *  Copyright 2009. All rights reserved.
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
#include "rogue.h"
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
 * 0-10 with CF 4 would be 2d3 + 2d2. By playing with the numbers, one can roughly approximate a gaussian
 * distribution of any mean and standard deviation.
 *
 * Player combatants take their base defense value of their actual armor. Their accuracy is a combination of weapon, armor
 * and strength. Each weapon and armor has a strength factor, and each point short of that number means an marginal -15
 * accuracy for the player.
 * 
 * Players have a base accuracy value of 50. This goes up by 10 with each experience level gained past 1.
 */

boolean attackHit(creature *attacker, creature *defender) {
	short accuracy = attacker->info.accuracy;
	short defense = defender->info.defense;
	short hitProbability;
	
	// any per-attack modifications to either accuracy or defense go here
	if (attacker == &player) {
		if (rogue.weapon) {
			accuracy -= 15 * max(0, (rogue.weapon->strengthRequired - rogue.currentStrength));
			accuracy += 10 * rogue.weapon->enchant1;
			if (rogue.weapon->flags & ITEM_VORPALIZED && defender->info.monsterID == rogue.weapon->vorpalEnemy) {
				accuracy += 4 * 10; // +4 to hit vorpal enemy
			}
		}
		if (rogue.armor) {
			accuracy -= 15 * max(0, (rogue.armor->strengthRequired - rogue.currentStrength));
		}
	}
	
	// automatically hit if the monster is sleeping or captive or stuck in a web
	if (defender->status.stuck || defender->bookkeepingFlags & MONST_CAPTIVE) {
		return true;
	}
	
	hitProbability = accuracy * pow(0.986, defense);
	return rand_percent(hitProbability);
}

void monsterShoots(creature *attacker, short targetLoc[2], uchar projChar, color *projColor) {
	short listOfCoordinates[DCOLS][2], originLoc[2];
	short i, x, y, numCells;
	creature *monst;
	uchar displayChar;
	color foreColor, backColor, multColor;
	boolean fastForward = false;
	
	originLoc[0] = attacker->xLoc;
	originLoc[1] = attacker->yLoc;
	
	numCells = getLineCoordinates(listOfCoordinates, originLoc, targetLoc);
	
	attacker->ticksUntilTurn = attacker->attackSpeed;
	
	updateDisplay();
	
	for (i=0; i<numCells; i++) {
		
		x = listOfCoordinates[i][0];
		y = listOfCoordinates[i][1];
		
		if (cellHasTerrainFlag(x, y, (OBSTRUCTS_PASSABILITY | OBSTRUCTS_VISION))) {
			break;
		}
		
		if (pmap[x][y].flags & (HAS_MONSTER | HAS_PLAYER)) {
			monst = monsterAtLoc(x, y);
			attack(attacker, monst);
			break;
		}
		
		if (pmap[x][y].flags & (VISIBLE | CLAIRVOYANT_VISIBLE)) {
			getCellAppearance(x, y, &displayChar, &foreColor, &backColor);
			foreColor = gray;
			colorMultiplierFromDungeonLight(x, y, &multColor);
			applyColorMultiplier(&foreColor, &multColor);
			plotCharWithColor(projChar, x + STAT_BAR_WIDTH, y + 1, *projColor, backColor);
			
			updateCoordinate(x + STAT_BAR_WIDTH, y + 1);
			
			if (!fastForward) {
				fastForward = pauseForMilliseconds(1);
			}
			
			refreshDungeonCell(x, y);
			updateCoordinate(x + STAT_BAR_WIDTH, y + 1);
		}
	}
}

void shootWeb(creature *breather, short targetLoc[2], short kindOfWeb) {
	short listOfCoordinates[DCOLS][2], originLoc[2];
	short i, x, y, numCells;
	char buf[COLS], buf2[COLS];
	boolean fastForward = false;
	
	originLoc[0] = breather->xLoc;
	originLoc[1] = breather->yLoc;
	
	spawnSurfaceEffect(originLoc[0], originLoc[1], SURFACE, kindOfWeb, 10, 10, 0, true);
	
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
			spawnSurfaceEffect(x, y, SURFACE, kindOfWeb, 80, 20, 0, true);
			break;
		} else if (!cellHasTerrainFlag(x, y, EXTINGUISHES_FIRE)) {
			spawnSurfaceEffect(x, y, SURFACE, kindOfWeb, 15, 12, 0, true);
		}
		updateDisplay();
		if (!fastForward) {
			fastForward = pauseForMilliseconds(1);
		}
	}
	if (!player.status.stuck && cellHasTerrainFlag(player.xLoc, player.yLoc, ENTANGLES)) {
		applyTileEffectToCreature(&player);
	}
}

void specialHit(creature *attacker, creature *defender) {
	short itemCandidates, randItemIndex;
	item *theItem, *previousItem, *itemFromTopOfStack;
	char buf[COLS], buf2[COLS], buf3[COLS];
	
	if (!(attacker->info.abilityFlags & SPECIAL_HIT)) {
		return;
	}
	
	// special hits that can affect only the player:
	if (defender == &player) {
		if (attacker->info.abilityFlags & MA_HIT_DEGRADE_ARMOR && defender == &player && rogue.armor && !(rogue.armor->flags & ITEM_PROTECTED)) {
			rogue.armor->enchant1--;
			equipItem(rogue.armor, true);
			nameOfItem(rogue.armor, buf2, false, false);
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
		
		if (attacker->info.abilityFlags & MA_HIT_SAP_STRENGTH && rogue.currentStrength > 0 && attackHit(attacker, defender)
			&& (!rogue.easyMode || rand_percent(50))) {
			rogue.currentStrength--;
			// updateStatBar(STR_CURRENT_STAT);
			message("you feel weaker.", true, false);
			strengthCheck(rogue.weapon);
			strengthCheck(rogue.armor);
		}
		
		if (attacker->info.abilityFlags & MA_HIT_STEAL_FLEE && !(attacker->carriedItem) && (packItems->nextItem)
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
				nameOfItem(theItem, buf3, false, true);
				sprintf(buf, "%s stole %s!", buf2, buf3);
				message(buf, true, false);
			}
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

// returns whether the attack hit
boolean attack(creature *attacker, creature *defender) {
	short damage, transferenceAmount;
	char buf[COLS], buf2[COLS], attackerName[COLS], defenderName[COLS], verb[DCOLS], explicationClause[DCOLS] = "";
	boolean sneakAttack, defenderWasAsleep, degradesAttackerWeapon = (defender->info.flags & MONST_DEFEND_DEGRADE_WEAPON ? true : false);
	
	if (defender->status.levitating && attacker->info.flags & MONST_RESTRICTED_TO_LIQUID) {
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
		
		if (attacker == &player && rogue.weapon && rogue.weapon->flags & ITEM_VORPALIZED
			&& rogue.weapon->vorpalEnemy == defender->info.monsterID) {
			damage = randClump(attacker->info.damage); // can hurt revenants with right vorpal
			damage += max(4, damage * 0.6); // vorpal weapons get (+4, +4) against their enemy
		}
		
		if (sneakAttack || defenderWasAsleep) {
			// won't do anything this turn because he's still flat-footed, so he doesn't hit back this turn
			defender->ticksUntilTurn += max(defender->movementSpeed, defender->attackSpeed);
			defender->creatureState = MONSTER_TRACKING_SCENT; // wake up
			damage *= 3; // treble damage when attacking a sleeping monster
		}
		
		if (defender == &player && rogue.depthLevel == 1) {
			damage = max(damage/2, 1); // player takes half damage on level 1
		}
		
		damage = min(damage, defender->currentHP);
		
		if (attacker == &player && rogue.transference && !(defender->info.flags & MONST_INANIMATE)) {
			transferenceAmount = damage * rogue.transference / 10;
			if (transferenceAmount == 0) {
				transferenceAmount = ((rogue.transference > 0) ? 1 : -1);
			}
			player.currentHP += min((player.info.maxHP - player.currentHP), transferenceAmount);
			
			if (player.currentHP <= 0) {
				gameOver("drained by a cursed ring", true);
				return false;
			}
		}
		
		if (damage == 0) {
			sprintf(explicationClause, " but %s no damage", (attacker == &player ? "do" : "does"));
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
		
		if (inflictDamage(attacker, defender, damage)) { // if the attack killed the defender
			if (defenderWasAsleep || sneakAttack) {
				sprintf(buf, "%s dispatched %s%s", attackerName, defenderName, explicationClause);
			} else {
				sprintf(buf, "%s defeated %s%s", attackerName, defenderName, explicationClause);
			}
			combatMessage(buf);
			if (&player == attacker && defender->info.expForKilling > 0 && rogue.weapon
				&& !(rogue.weapon->flags & ITEM_IDENTIFIED) && !--rogue.weapon->charges) {
				rogue.weapon->flags |= ITEM_IDENTIFIED;
				message("you are now familiar enough with your weapon to identify it.", true, false);
				nameOfItem(rogue.weapon, buf2, true, true);
				sprintf(buf, "%s %s.", (rogue.weapon->quantity > 1 ? "they are" : "it is"), buf2);
				message(buf, true, false);
			}
			if (&player != defender) {
				free(defender); // free the memory
				defender = NULL;
			} else {
				gameOver(monsterText[attacker->info.monsterID].name, false);
				return true;
			}
		} else { // if the defender survived
			if (!rogue.blockCombatText && (canSeeMonster(attacker) || canSeeMonster(defender))) {
				attackVerb(verb, attacker, max(damage - attacker->info.damage.lowerBound, 0) * 100
						   / max(0, attacker->info.damage.upperBound - attacker->info.damage.lowerBound));
				sprintf(buf, "%s %s %s%s", attackerName, verb, defenderName, explicationClause);
				combatMessage(buf);
			}
			if (attacker->info.abilityFlags & SPECIAL_HIT) {
				specialHit(attacker, defender);
			}
		}
		
		if (degradesAttackerWeapon && attacker == &player && rogue.weapon && !(rogue.weapon->flags & ITEM_PROTECTED)) {
			if (rand_percent(50)) {
				rogue.weapon->enchant1--;
			} else {
				rogue.weapon->enchant2--;
			}
			nameOfItem(rogue.weapon, buf2, false, false);
			sprintf(buf, "your %s weakens!", buf2);
			message(buf, true, false);
		}
		
		return true;
	} else { // if the attack missed
		if (!rogue.blockCombatText) {
			sprintf(buf, "%s missed %s", attackerName, defenderName);
			combatMessage(buf);
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
			if (rogue.experienceLevel % 2 == 1) {
				rogue.maxStrength++;
				rogue.currentStrength++;
			}
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
		strcpy(combatText, "");
		message(buf, true, false);
	}
}

// returns true if this was a killing stroke; does NOT free the pointer, but DOES remove it from the monster chain
boolean inflictDamage(creature *attacker, creature *defender, short damage) {
	dungeonFeature theBlood;
	
	if (damage == 0) {
		return false;
	}
	
	// bleed all over the place, proportionately to damage inflicted:
	if (defender->info.bloodType) {
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
		damage = max(1, damage/2);
	}
	
	if (defender->currentHP <= damage) {
		if (defender->carriedItem) {
			makeMonsterDropItem(defender);
		}
		killCreature(defender);
		addExperience(defender->info.expForKilling);
		return true;
	}
	defender->currentHP -= damage;
	if (defender != &player && defender->creatureState != MONSTER_ALLY
		&& defender->info.flags & MONST_FLEES_NEAR_DEATH
		&& defender->info.maxHP / 4 >= defender->currentHP) {
		defender->creatureState = MONSTER_FLEEING;
		chooseNewWanderDestination(defender);
	}
	return false;
}

// removes the decedent from the screen and from the monster chain; does NOT free the memory.
void killCreature(creature *decedent) {
	short x, y;
	lightSource *theLight;
	creature *previousCreature;
	
	x = decedent->xLoc;
	y = decedent->yLoc;
	pmap[x][y].flags &= ~HAS_MONSTER;
	if (decedent == &player) { // the player died
		player.currentHP = 0;
		// game over handled elsewhere
	} else {
		for (previousCreature = monsters;
			 previousCreature->nextCreature != decedent;
			 previousCreature = previousCreature->nextCreature);
		previousCreature->nextCreature = decedent->nextCreature;
		refreshDungeonCell(x, y);
		// updateDisplay();
		for (theLight = lights->nextLight; theLight != NULL; theLight = theLight->nextLight) {
			if (theLight->followsCreature == decedent) {
				deleteLight(theLight);
				theLight = lights;
			}
		}
	}
	if (decedent->bookkeepingFlags & MONST_LEADER) {
		demoteMonsterFromLeadership(decedent);
	}
}