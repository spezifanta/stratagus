--            ____            
--           / __ )____  _____
--          / __  / __ \/ ___/
--         / /_/ / /_/ (__  ) 
--        /_____/\____/____/  
--
--  Invasion - Battle of Survival                  
--   A GPL'd futuristic RTS game
--
--	anim.lua	-	The unit animation definitions.
--
--	(c) Copyright 2000-2005 by Josh Cogliati, Lutz Sammer, Crestez Leonard
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
--      Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
--
--        $Id$

 

---------------------------------------
-- Misc.

DefineAnimations("animations-building", {
    Still = {"frame 0", "wait 4", "frame 0", "wait 1"},
    })

---------------------------------------
--        Dead Body

DefineAnimations("animations-dead-body", {
    Death = {"unbreakable begin", "frame 5", "wait 200", "frame 10", "wait 200",
        "frame 15", "wait 200", "frame 20", "wait 200", "frame 25", "wait 200",
        "frame 25", "unbreakable end", "wait 1", },
    })

---------------------------------------
--        Destroyed *x* Place

DefineAnimations("animations-destroyed-place", {
    Death = {"unbreakable begin", "frame 0", "wait 200", "frame 1", "wait 200",
        "frame 1", "unbreakable end", "wait 1", },
    })

