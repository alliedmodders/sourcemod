/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#if defined _sdktools_trace_included
 #endinput
#endif
#define _sdktools_trace_included

#define	CONTENTS_EMPTY			0		/**< No contents. */
#define	CONTENTS_SOLID			0x1		/**< an eye is never valid in a solid . */
#define	CONTENTS_WINDOW			0x2		/**< translucent, but not watery (glass). */
#define	CONTENTS_AUX			0x4
#define	CONTENTS_GRATE			0x8		/**< alpha-tested "grate" textures.  Bullets/sight pass through, but solids don't. */
#define	CONTENTS_SLIME			0x10
#define	CONTENTS_WATER			0x20
#define	CONTENTS_MIST			0x40
#define CONTENTS_OPAQUE			0x80		/**< things that cannot be seen through (may be non-solid though). */
#define	LAST_VISIBLE_CONTENTS	0x80
#define ALL_VISIBLE_CONTENTS 	(LAST_VISIBLE_CONTENTS | (LAST_VISIBLE_CONTENTS-1))
#define CONTENTS_TESTFOGVOLUME	0x100
#define CONTENTS_UNUSED5		0x200
#define CONTENTS_UNUSED6		0x4000
#define CONTENTS_TEAM1			0x800		/**< per team contents used to differentiate collisions. */
#define CONTENTS_TEAM2			0x1000		/**< between players and objects on different teams. */
#define CONTENTS_IGNORE_NODRAW_OPAQUE	0x2000		/**< ignore CONTENTS_OPAQUE on surfaces that have SURF_NODRAW. */
#define CONTENTS_MOVEABLE		0x4000		/**< hits entities which are MOVETYPE_PUSH (doors, plats, etc) */
#define	CONTENTS_AREAPORTAL		0x8000		/**< remaining contents are non-visible, and don't eat brushes. */
#define	CONTENTS_PLAYERCLIP		0x10000
#define	CONTENTS_MONSTERCLIP	0x20000

/**
 * @section currents can be added to any other contents, and may be mixed
 */
#define	CONTENTS_CURRENT_0		0x40000
#define	CONTENTS_CURRENT_90		0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP		0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000

/**
 * @endsection
 */

#define	CONTENTS_ORIGIN			0x1000000	/**< removed before bsping an entity. */
#define	CONTENTS_MONSTER		0x2000000	/**< should never be on a brush, only in game. */
#define	CONTENTS_DEBRIS			0x4000000
#define	CONTENTS_DETAIL			0x8000000	/**< brushes to be added after vis leafs. */
#define	CONTENTS_TRANSLUCENT	0x10000000	/**< auto set if any surface has trans. */
#define	CONTENTS_LADDER			0x20000000
#define CONTENTS_HITBOX			0x40000000	/**< use accurate hitboxes on trace. */

/**
 * @section Trace masks.
 */
#define	MASK_ALL				(0xFFFFFFFF)
#define	MASK_SOLID				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE) 			/**< everything that is normally solid */
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE) 	/**< everything that blocks player movement */
#define	MASK_NPCSOLID			(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE) /**< blocks npc movement */
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_MOVEABLE|CONTENTS_SLIME) 							/**< water physics in these contents */
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_OPAQUE) 							/**< everything that blocks line of sight for AI, lighting, etc */
#define MASK_OPAQUE_AND_NPCS	(MASK_OPAQUE|CONTENTS_MONSTER)										/**< everything that blocks line of sight for AI, lighting, etc, but with monsters added. */
#define	MASK_VISIBLE			(MASK_OPAQUE|CONTENTS_IGNORE_NODRAW_OPAQUE) 								/**< everything that blocks line of sight for players */
#define MASK_VISIBLE_AND_NPCS	(MASK_OPAQUE_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE) 							/**< everything that blocks line of sight for players, but with monsters added. */
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_HITBOX) 	/**< bullets see these as solid */
#define MASK_SHOT_HULL			(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_GRATE) 	/**< non-raycasted weapons see this as solid (includes grates) */
#define MASK_SHOT_PORTAL		(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW) 							/**< hits solids (not grates) and passes through everything else */
#define MASK_SOLID_BRUSHONLY	(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_GRATE) 					/**< everything normally solid, except monsters (world+brush only) */
#define MASK_PLAYERSOLID_BRUSHONLY	(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_PLAYERCLIP|CONTENTS_GRATE) 			/**< everything normally solid for player movement, except monsters (world+brush only) */
#define MASK_NPCSOLID_BRUSHONLY	(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE) 			/**< everything normally solid for npc movement, except monsters (world+brush only) */
#define MASK_NPCWORLDSTATIC		(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE) 					/**< just the world, used for route rebuilding */
#define MASK_SPLITAREAPORTAL	(CONTENTS_WATER|CONTENTS_SLIME) 									/**< These are things that can split areaportals */

/**
 * @endsection
 */

enum RayType
{
	RayType_EndPoint,	/**< The trace ray will go from the start position to the end position. */
	RayType_Infinite	/**< The trace ray will go from the start position to infinity using a direction vector. */
};

funcenum TraceEntityFilter
{
	/**
	 * Called on entity filtering.
	 *
	 * @param entity		Entity index.
 	 * @param contentsMask	Contents Mask.
 	 * @return				True to allow the current entity to be hit, otherwise false.
 	 */
	bool:public(entity, contentsMask),
	
	/**
	 * Called on entity filtering.
	 *
	 * @param entity		Entity index.
 	 * @param contentsMask	Contents Mask.
 	 * @param data			Data value, if used.
 	 * @return				True to allow the current entity to be hit, otherwise false.
 	 */
	bool:public(entity, contentsMask, any:data),
};

/**
 * Get the contents mask and the entity index at the given position.
 *
 * @param pos			World position to test.
 * @param entindex		Entity index found at the given position (by reference).
 * @return				Contents mask.
 */
native TR_GetPointContents(const Float:pos[3], &entindex=-1);

/**
 * Get the point contents testing only the given entity index.
 *
 * @param entindex		Entity index to test.
 * @param pos			World position.
 * @return				Contents mask.
 */
native TR_GetPointContentsEnt(entindex, const Float:pos[3]);

/**
 * Starts up a new trace ray using a global trace result.
 *
 * @param pos			Starting position of the ray.
 * @param vec			Depending on RayType, it will be used as the
 *						ending point, or the direction angle.
 * @param flags			Trace flags.
 * @param rtype			Method to calculate the ray direction.
 * @noreturn
 */
native TR_TraceRay(const Float:pos[3],
				   const Float:vec[3],
				   flags,
				   RayType:rtype);
				   
/**
 * Starts up a new trace hull using a global trace result.
 *
 * @param pos			Starting position of the ray.
 * @param vec			Ending position of the ray.
 * @param mins			Hull minimum size.
 * @param maxs			Hull maximum size.
 * @param flags			Trace flags.
 * @noreturn
 */
native TR_TraceHull(const Float:pos[3],
					const Float:vec[3],
					const Float:mins[3],
					const Float:maxs[3],
					flags);

/**
 * Starts up a new trace ray using a global trace result and a customized 
 * trace ray filter.
 *
 * Calling TR_Trace*Filter or TR_Trace*FilterEx from inside a filter 
 * function is currently not allowed and may not work.
 *
 * @param pos			Starting position of the ray.
 * @param vec			Depending on RayType, it will be used as the ending 
 *						point, or the direction angle.
 * @param flags			Trace flags.
 * @param rtype			Method to calculate the ray direction.
 * @param filter		Function to use as a filter.
 * @param data			Arbitrary data value to pass through to the filter 
 *						function.
 * @noreturn
 */
native TR_TraceRayFilter(const Float:pos[3],
						 const Float:vec[3],
						 flags,
						 RayType:rtype,
						 TraceEntityFilter:filter,
						 any:data=0);
						 
/**
 * Starts up a new trace hull using a global trace result and a customized 
 * trace ray filter.
 *
 * Calling TR_Trace*Filter or TR_Trace*FilterEx from inside a filter 
 * function is currently not allowed and may not work.
 *
 * @param pos			Starting position of the ray.
 * @param vec			Depending on RayType, it will be used as the ending 
 *						point, or the direction angle.
 * @param mins			Hull minimum size.
 * @param maxs			Hull maximum size.
 * @param flags			Trace flags.
 * @param filter		Function to use as a filter.
 * @param data			Arbitrary data value to pass through to the filter 
 *						function.
 * @noreturn
 */
native TR_TraceHullFilter(const Float:pos[3],
						  const Float:vec[3],
						  const Float:mins[3],
						  const Float:maxs[3],
						  flags,
						  TraceEntityFilter:filter,
						  any:data=0);

/**
 * Starts up a new trace ray using a new trace result.
 *
 * @param pos			Starting position of the ray.
 * @param vec			Depending on RayType, it will be used as the ending 
 *						point, or the direction angle.
 * @param flags			Trace flags.
 * @param rtype			Method to calculate the ray direction.
 * @return				Ray trace handle, which must be closed via CloseHandle().
 */
native Handle:TR_TraceRayEx(const Float:pos[3],
							const Float:vec[3],
							flags,
							RayType:rtype);
							
/**
 * Starts up a new trace hull using a new trace result.
 *
 * @param pos			Starting position of the ray.
 * @param vec			Ending position of the ray.
 * @param mins			Hull minimum size.
 * @param maxs			Hull maximum size.
 * @param flags			Trace flags.
 * @return				Ray trace handle, which must be closed via CloseHandle().
 */
native Handle:TR_TraceHullEx(const Float:pos[3],
							 const Float:vec[3],
							 const Float:mins[3],
							 const Float:maxs[3],
							 flags);

/**
 * Starts up a new trace ray using a new trace result and a customized 
 * trace ray filter.
 *
 * Calling TR_Trace*Filter or TR_TraceRay*Ex from inside a filter 
 * function is currently not allowed and may not work.
 *
 * @param pos			Starting position of the ray.
 * @param vec			Depending on RayType, it will be used as the ending 
 *						point, or the direction angle.
 * @param flags			Trace flags.
 * @param rtype			Method to calculate the ray direction.
 * @param filter		Function to use as a filter.
 * @param data			Arbitrary data value to pass through to the filter function.
 * @return				Ray trace handle, which must be closed via CloseHandle().
 */
native Handle:TR_TraceRayFilterEx(const Float:pos[3], 
								  const Float:vec[3],
								  flags, 
								  RayType:rtype, 
								  TraceEntityFilter:filter,
								  any:data=0);
								  
/**
 * Starts up a new trace hull using a new trace result and a customized 
 * trace ray filter.
 *
 * Calling TR_Trace*Filter or TR_Trace*FilterEx from inside a filter 
 * function is currently not allowed and may not work.
 *
 * @param pos			Starting position of the ray.
 * @param vec			Ending position of the ray.
 * @param mins			Hull minimum size.
 * @param maxs			Hull maximum size.
 * @param flags			Trace flags.
 * @param filter		Function to use as a filter.
 * @param data			Arbitrary data value to pass through to the filter function.
 * @return				Ray trace handle, which must be closed via CloseHandle().
 */
native Handle:TR_TraceHullFilterEx(const Float:pos[3], 
								   const Float:vec[3],
								   const Float:mins[3],
								   const Float:maxs[3],
								   flags, 
								   TraceEntityFilter:filter,
								   any:data=0);

/**
 * Returns the time fraction from a trace result (1.0 means no collision).
 *
 * @param hndl			A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @return				Time fraction value of the trace.
 * @error				Invalid Handle.
 */
native Float:TR_GetFraction(Handle:hndl=INVALID_HANDLE);

/**
 * Returns the collision position of a trace result.
 *
 * @param pos			Vector buffer to store data in.
 * @param hndl			A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @noreturn
 * @error				Invalid Handle.
 */
native TR_GetEndPosition(Float:pos[3], Handle:hndl=INVALID_HANDLE);

/**
 * Returns the entity index that collided with the trace.
 *
 * @param hndl			A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @return				Entity index or -1 for no collision.
 * @error				Invalid Handle.
 */
native TR_GetEntityIndex(Handle:hndl=INVALID_HANDLE);

/**
 * Returns if there was any kind of collision along the trace ray.
 *
 * @param hndl			A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @return				True if any collision found, otherwise false.
 * @error				Invalid Handle.
 */
native bool:TR_DidHit(Handle:hndl=INVALID_HANDLE);

/**
 * Returns in which body hit group the trace collided if any.
 *
 * @param hndl			A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @return				Body hit group.
 * @error				Invalid Handle.
 */
native TR_GetHitGroup(Handle:hndl=INVALID_HANDLE);

/**
 * Find the normal vector to the collison plane of a trace.
 *
 * @param hndl		A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @param normal	Vector buffer to store the vector normal to the collision plane
 * @noreturn
 * @error			Invalid Handle
 */
native TR_GetPlaneNormal(Handle:hndl, Float:normal[3]);

/**
 * Tests a point to see if it's outside any playable area
 *
 * @param pos		Vector buffer to store data in.
 * @return		True if outside world, otherwise false.
 */
native TR_PointOutsideWorld(Float:pos[3]);