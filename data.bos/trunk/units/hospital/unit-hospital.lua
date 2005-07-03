--       _________ __                 __
--      /   _____//  |_1____________ _/  |______     ____  __ __  ______
--      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
--      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ \ 
--     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
--             \/                  \/          \//_____/            \/ 
--  ______________________                           ______________________
--			  T H E   W A R   B E G I N S
--	   Stratagus - A free fantasy real time strategy game engine
--
--	unit-hospital.lua	-	Define the hospital building
--
--	(c) Copyright 2001 - 2005 by Fran�ois Beerten, Lutz Sammer and Crestez Leonard
--
--      This program is free software; you can redistribute it and/or modify
--      it under the terms of the GNU General Public License as published by
--      the Free Software Foundation; either version 2 of the License, or
--      (at your option) any later version.
--  
--      This program is distributed in the hope that it will be useful,
--      but WITHOUT ANY WARRANTY; without even the implied warranty of
--      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--      GNU General Public License for more details.
--  
--      You should have received a copy of the GNU General Public License
--      along with this program; if not, write to the Free Software
--      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
--
--	$Id$

DefineAnimations("animations-hosp", {
    Still = {"frame 0", "wait 2", "frame 1", "wait 2", "frame 2", "wait 2", 
        "frame 3", "wait 2", },
    })
DefineIcon({
	Name = "icon-hosp",
	Size = {46, 38},
	Frame = 0,
	File = GetCurrentLuaPath().."/hospital_i.png"})

DefineUnitType("unit-hosp", {
	Name = "Hospital",
	Image = {"file", GetCurrentLuaPath().."/hospital.png", "size", {128, 96}},
	Shadow = {"file", GetCurrentLuaPath().."/hospital_s.png", "size", {128, 96}},
	Animations = "animations-hosp", Icon = "icon-hosp",
	Costs = {"time", 125, "titanium", 500, "crystal", 100},
	RepairHp = 2, RepairCosts = {"titanium", 2}, Construction = "construction-hosp",
	Speed = 0, HitPoints = 350, DrawLevel = 25, TileSize = {4, 3},
	BoxSize = {124, 92}, SightRange = 2, Armor = 30, BasicDamage = 0,
	PiercingDamage = 0, Missile = "missile-none", Priority = 35,
	AnnoyComputerFactor = 45, Demand = 200, Points = 200,
	ExplodeWhenKilled = "missile-160x128-explosion", Corpse = {"build-dead-body5", 0},
	Type = "land", Building = true, BuilderOutside = true, VisibleUnderFog = true,
	Sounds = {
		"selected", "hosp-selected",
		"ready", "hosp-ready",
		"help", "hosp-help",
		"dead", "hosp-dead"}
	})
DefineUnitType("build-dead-body5", {
	Name = "HospCrater",
	Image = {"file", GetCurrentLuaPath().."/hospital.png", "size", {128, 96}},
	Animations = "animations-elitebuild5", Icon = "icon-cancel",
	Speed = 0, HitPoints = 999, DrawLevel = 10, TileSize = {4, 3},
	BoxSize = {124, 92}, SightRange = 1, BasicDamage = 0,
	PiercingDamage = 0, Missile = "missile-none",
	Priority = 0, Type = "land", Building = true, Vanishes = true
	})
