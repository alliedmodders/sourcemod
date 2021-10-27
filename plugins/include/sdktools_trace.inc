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

#define CONTENTS_EMPTY                   0           /**< No contents. */
#define CONTENTS_SOLID                   0x1         /**< an eye is never valid in a solid . */
#define CONTENTS_WINDOW                  0x2         /**< translucent, but not watery (glass). */
#define CONTENTS_AUX                     0x4
#define CONTENTS_GRATE                   0x8         /**< alpha-tested "grate" textures.  Bullets/sight pass through, but solids don't. */
#define CONTENTS_SLIME                   0x10
#define CONTENTS_WATER                   0x20
#define CONTENTS_MIST                    0x40
#define CONTENTS_OPAQUE                  0x80        /**< things that cannot be seen through (may be non-solid though). */
#define LAST_VISIBLE_CONTENTS            0x80
#define ALL_VISIBLE_CONTENTS             (LAST_VISIBLE_CONTENTS | (LAST_VISIBLE_CONTENTS-1))
#define CONTENTS_TESTFOGVOLUME           0x100
#define CONTENTS_UNUSED5                 0x200
#define CONTENTS_UNUSED6                 0x4000
#define CONTENTS_TEAM1                   0x800       /**< per team contents used to differentiate collisions. */
#define CONTENTS_TEAM2                   0x1000      /**< between players and objects on different teams. */
#define CONTENTS_IGNORE_NODRAW_OPAQUE    0x2000      /**< ignore CONTENTS_OPAQUE on surfaces that have SURF_NODRAW. */
#define CONTENTS_MOVEABLE                0x4000      /**< hits entities which are MOVETYPE_PUSH (doors, plats, etc) */
#define CONTENTS_AREAPORTAL              0x8000      /**< remaining contents are non-visible, and don't eat brushes. */
#define CONTENTS_PLAYERCLIP              0x10000
#define CONTENTS_MONSTERCLIP             0x20000

/**
 * @section currents can be added to any other contents, and may be mixed
 */
#define CONTENTS_CURRENT_0      0x40000
#define CONTENTS_CURRENT_90     0x80000
#define CONTENTS_CURRENT_180    0x100000
#define CONTENTS_CURRENT_270    0x200000
#define CONTENTS_CURRENT_UP     0x400000
#define CONTENTS_CURRENT_DOWN   0x800000

/**
 * @endsection
 */

#define CONTENTS_ORIGIN         0x1000000   /**< removed before bsp-ing an entity. */
#define CONTENTS_MONSTER        0x2000000   /**< should never be on a brush, only in game. */
#define CONTENTS_DEBRIS         0x4000000
#define CONTENTS_DETAIL         0x8000000   /**< brushes to be added after vis leafs. */
#define CONTENTS_TRANSLUCENT    0x10000000  /**< auto set if any surface has trans. */
#define CONTENTS_LADDER         0x20000000
#define CONTENTS_HITBOX         0x40000000  /**< use accurate hitboxes on trace. */

/**
 * @section Trace masks.
 */
#define MASK_ALL                    (0xFFFFFFFF)
#define MASK_SOLID                  (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)                      /**< everything that is normally solid */
#define MASK_PLAYERSOLID            (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)  /**< everything that blocks player movement */
#define MASK_NPCSOLID               (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE) /**< blocks npc movement */
#define MASK_WATER                  (CONTENTS_WATER|CONTENTS_MOVEABLE|CONTENTS_SLIME)                                                       /**< water physics in these contents */
#define MASK_OPAQUE                 (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_OPAQUE)                                                      /**< everything that blocks line of sight for AI, lighting, etc */
#define MASK_OPAQUE_AND_NPCS        (MASK_OPAQUE|CONTENTS_MONSTER)                                                                          /**< everything that blocks line of sight for AI, lighting, etc, but with monsters added. */
#define MASK_VISIBLE                (MASK_OPAQUE|CONTENTS_IGNORE_NODRAW_OPAQUE)                                                             /**< everything that blocks line of sight for players */
#define MASK_VISIBLE_AND_NPCS       (MASK_OPAQUE_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE)                                                    /**< everything that blocks line of sight for players, but with monsters added. */
#define MASK_SHOT                   (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_HITBOX)     /**< bullets see these as solid */
#define MASK_SHOT_HULL              (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_GRATE)      /**< non-raycasted weapons see this as solid (includes grates) */
#define MASK_SHOT_PORTAL            (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW)                                                      /**< hits solids (not grates) and passes through everything else */
#define MASK_SOLID_BRUSHONLY        (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_GRATE)                                       /**< everything normally solid, except monsters (world+brush only) */
#define MASK_PLAYERSOLID_BRUSHONLY  (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_PLAYERCLIP|CONTENTS_GRATE)                   /**< everything normally solid for player movement, except monsters (world+brush only) */
#define MASK_NPCSOLID_BRUSHONLY     (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)                  /**< everything normally solid for npc movement, except monsters (world+brush only) */
#define MASK_NPCWORLDSTATIC         (CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)                                    /**< just the world, used for route rebuilding */
#define MASK_SPLITAREAPORTAL        (CONTENTS_WATER|CONTENTS_SLIME)                                                                         /**< These are things that can split areaportals */

/**
 * @endsection
 */

/**
 * @section Surface flags.
 */

#define SURF_LIGHT       0x0001      /**< value will hold the light strength */
#define SURF_SKY2D       0x0002      /**< don't draw, indicates we should skylight + draw 2d sky but not draw the 3D skybox */
#define SURF_SKY         0x0004      /**< don't draw, but add to skybox */
#define SURF_WARP        0x0008      /**< turbulent water warp */
#define SURF_TRANS       0x0010
#define SURF_NOPORTAL    0x0020      /**< the surface can not have a portal placed on it */
#define SURF_TRIGGER     0x0040      /**< This is an xbox hack to work around elimination of trigger surfaces, which breaks occluders */
#define SURF_NODRAW      0x0080      /**< don't bother referencing the texture */

#define SURF_HINT        0x0100      /**< make a primary bsp splitter */

#define SURF_SKIP        0x0200      /**< completely ignore, allowing non-closed brushes */
#define SURF_NOLIGHT     0x0400      /**< Don't calculate light */
#define SURF_BUMPLIGHT   0x0800      /**< calculate three lightmaps for the surface for bumpmapping */
#define SURF_NOSHADOWS   0x1000      /**< Don't receive shadows */
#define SURF_NODECALS    0x2000      /**< Don't receive decals */
#define SURF_NOCHOP      0x4000      /**< Don't subdivide patches on this surface */
#define SURF_HITBOX      0x8000      /**< surface is part of a hitbox */

/**
 * @endsection
 */

/**
 * @section Partition masks.
 */

#define PARTITION_SOLID_EDICTS        (1 << 1) /**< every edict_t that isn't SOLID_TRIGGER or SOLID_NOT (and static props) */
#define PARTITION_TRIGGER_EDICTS      (1 << 2) /**< every edict_t that IS SOLID_TRIGGER */
#define PARTITION_NON_STATIC_EDICTS   (1 << 5) /**< everything in solid & trigger except the static props, includes SOLID_NOTs */
#define PARTITION_STATIC_PROPS        (1 << 7)

/**
 * @endsection
 */

/**
 * @section Displacement flags.
 */

#define DISPSURF_FLAG_SURFACE         (1<<0)
#define DISPSURF_FLAG_WALKABLE        (1<<1)
#define DISPSURF_FLAG_BUILDABLE       (1<<2)
#define DISPSURF_FLAG_SURFPROP1       (1<<3)
#define DISPSURF_FLAG_SURFPROP2       (1<<4)

/**
 * @endsection
 */

enum RayType
{
	RayType_EndPoint,   /**< The trace ray will go from the start position to the end position. */
	RayType_Infinite    /**< The trace ray will go from the start position to infinity using a direction vector. */
};

typeset TraceEntityFilter
{
	/**
	 * Called on entity filtering.
	 *
	 * @param entity        Entity index.
	 * @param contentsMask  Contents Mask.
	 * @return              True to allow the current entity to be hit, otherwise false.
	 */
	function bool (int entity, int contentsMask);

	/**
	 * Called on entity filtering.
	 *
	 * @param entity        Entity index.
	 * @param contentsMask  Contents Mask.
	 * @param data          Data value, if used.
	 * @return              True to allow the current entity to be hit, otherwise false.
	 */
	function bool (int entity, int contentsMask, any data);
};

typeset TraceEntityEnumerator
{
	/**
	 * Called for each entity enumerated with EnumerateEntities*.
	 *
	 * @param entity        Entity index.
	 * @return              True to continue enumerating, otherwise false.
	 */
	function bool (int entity);

	/**
	 * Called for each entity enumerated with EnumerateEntities*.
	 *
	 * @param entity        Entity index.
	 * @param data          Data value, if used.
	 * @return              True to continue enumerating, otherwise false. */
	function bool (int entity, any data);
}

/**
 * Get the contents mask and the entity index at the given position.
 *
 * @param pos           World position to test.
 * @param entindex      Entity index found at the given position (by reference).
 * @return              Contents mask.
 */
native int TR_GetPointContents(const float pos[3], int &entindex=-1);

/**
 * Get the point contents testing only the given entity index.
 *
 * @param entindex      Entity index to test.
 * @param pos           World position.
 * @return              Contents mask.
 * @error               Invalid entity.
 */
native int TR_GetPointContentsEnt(int entindex, const float pos[3]);

/**
 * Starts up a new trace ray using a global trace result.
 *
 * @param pos           Starting position of the ray.
 * @param vec           Depending on RayType, it will be used as the
 *                      ending point, or the direction angle.
 * @param flags         Trace flags.
 * @param rtype         Method to calculate the ray direction.
 */
native void TR_TraceRay(const float pos[3],
                        const float vec[3],
                        int flags,
                        RayType rtype);

/**
 * Starts up a new trace hull using a global trace result.
 *
 * @param pos           Starting position of the ray.
 * @param vec           Ending position of the ray.
 * @param mins          Hull minimum size.
 * @param maxs          Hull maximum size.
 * @param flags         Trace flags.
 */
native void TR_TraceHull(const float pos[3],
                         const float vec[3],
                         const float mins[3],
                         const float maxs[3],
                         int flags);

/**
 * Enumerates over entities along a ray. This may find entities that are
 * close to the ray but do not actually intersect it. Use TR_Clip*RayToEntity
 * with TR_DidHit to check if the ray actually intersects the entity.
 *
 * @param pos           Starting position of the ray.
 * @param vec           Depending on RayType, it will be used as the ending
 *                      point, or the direction angle.
 * @param mask          Mask to use for the trace. See PARTITION_* flags.
 * @param rtype         Method to calculate the ray direction.
 * @param enumerator    Function to use as enumerator. For each entity found
 *                      along the ray, this function is called.
 * @param data          Arbitrary data value to pass through to the enumerator.
 */
native void TR_EnumerateEntities(const float pos[3],
                                 const float vec[3],
                                 int mask,
                                 RayType rtype,
                                 TraceEntityEnumerator enumerator,
                                 any data=0);

/**
 * Enumerates over entities along a ray hull. This may find entities that are
 * close to the ray but do not actually intersect it. Use TR_Clip*RayToEntity
 * with TR_DidHit to check if the ray actually intersects the entity.
 *
 * @param pos           Starting position of the ray.
 * @param vec           Ending position of the ray.
 * @param mins          Hull minimum size.
 * @param maxs          Hull maximum size.
 * @param mask          Mask to use for the trace. See PARTITION_* flags.
 * @param enumerator    Function to use as enumerator. For each entity found
 *                      along the ray, this function is called.
 * @param data          Arbitrary data value to pass through to the enumerator.
 */
native void TR_EnumerateEntitiesHull(const float pos[3],
                                    const float vec[3],
                                    const float mins[3],
                                    const float maxs[3],
                                    int mask,
                                    TraceEntityEnumerator enumerator,
                                    any data=0);

/**
 * Enumerates over entities in a sphere.
 *
 * @param pos           Starting position of the ray.
 * @param radius        Radius of the ray.
 * @param mask          Mask to use for the trace. See PARTITION_* flags.
 * @param enumerator    Function to use as enumerator. For each entity found
 *                      along the ray, this function is called.
 * @param data          Arbitrary data value to pass through to the enumerator.
 */
native void TR_EnumerateEntitiesSphere(const float pos[3],
                                    float radius,
                                    int mask,
                                    TraceEntityEnumerator enumerator,
                                    any data=0);
                                    
/**
 * Enumerates over entities in a box.
 *
 * @param mins          Box minimum size.
 * @param maxs          Box maximum size.
 * @param mask          Mask to use for the trace. See PARTITION_* flags.
 * @param enumerator    Function to use as enumerator. For each entity found
 *                      along the box, this function is called.
 * @param data          Arbitrary data value to pass through to the enumerator.
 */
native void TR_EnumerateEntitiesBox(const float mins[3],
                                    const float maxs[3],
                                    int mask,
                                    TraceEntityEnumerator enumerator,
                                    any data=0);
        
/**
 * Enumerates over entities at point.
 *
 * @param pos           Position of the point.
 * @param mask          Mask to use for the trace. See PARTITION_* flags.
 * @param enumerator    Function to use as enumerator. For each entity found
 *                      along the point, this function is called.
 * @param data          Arbitrary data value to pass through to the enumerator.
 */
native void TR_EnumerateEntitiesPoint(const float pos[3],
                                    int mask,
                                    TraceEntityEnumerator enumerator,
                                    any data=0);
        
/**
 * Starts up a new trace ray using a global trace result and a customized
 * trace ray filter.
 *
 * Calling TR_Trace*Filter or TR_Trace*FilterEx from inside a filter
 * function is currently not allowed and may not work.
 *
 * @param pos           Starting position of the ray.
 * @param vec           Depending on RayType, it will be used as the ending
 *                      point, or the direction angle.
 * @param flags         Trace flags.
 * @param rtype         Method to calculate the ray direction.
 * @param filter        Function to use as a filter.
 * @param data          Arbitrary data value to pass through to the filter
 *                      function.
 */
native void TR_TraceRayFilter(const float pos[3],
                              const float vec[3],
                              int flags,
                              RayType rtype,
                              TraceEntityFilter filter,
                              any data=0);

/**
 * Starts up a new trace hull using a global trace result and a customized
 * trace ray filter.
 *
 * Calling TR_Trace*Filter or TR_Trace*FilterEx from inside a filter
 * function is currently not allowed and may not work.
 *
 * @param pos           Starting position of the ray.
 * @param vec           Ending position of the ray.
 * @param mins          Hull minimum size.
 * @param maxs          Hull maximum size.
 * @param flags         Trace flags.
 * @param filter        Function to use as a filter.
 * @param data          Arbitrary data value to pass through to the filter
 *                      function.
 */
native void TR_TraceHullFilter(const float pos[3],
                               const float vec[3],
                               const float mins[3],
                               const float maxs[3],
                               int flags,
                               TraceEntityFilter filter,
                               any data=0);

/**
 * Clips a ray to a particular entity.
 *
 * @param pos           Starting position of the ray.
 * @param vec           Depending on RayType, it will be used as the ending
 *                      point, or the direction angle.
 * @param flags         Trace flags.
 * @param rtype         Method to calculate the ray direction.
 * @param entity        Entity to clip to.
 * @error               Invalid entity.
 */
native void TR_ClipRayToEntity(const float pos[3],
                               const float vec[3],
                               int flags,
                               RayType rtype,
                               int entity);

/**
 * Clips a ray hull to a particular entity.
 *
 * @param pos           Starting position of the ray.
 * @param vec           Ending position of the ray.
 * @param mins          Hull minimum size.
 * @param maxs          Hull maximum size.
 * @param flags         Trace flags.
 * @param entity        Entity to clip to.
 * @error               Invalid entity.
 */
native void TR_ClipRayHullToEntity(const float pos[3],
                                   const float vec[3],
                                   const float mins[3],
                                   const float maxs[3],
                                   int flags,
                                   int entity);

/**
 * Clips the current global ray (or hull) to a particular entity.
 *
 * @param flags         Trace flags.
 * @param entity        Entity to clip to.
 * @error               Invalid entity.
 */
native void TR_ClipCurrentRayToEntity(int flags, int entity);

/**
 * Starts up a new trace ray using a new trace result.
 *
 * @param pos           Starting position of the ray.
 * @param vec           Depending on RayType, it will be used as the ending
 *                      point, or the direction angle.
 * @param flags         Trace flags.
 * @param rtype         Method to calculate the ray direction.
 * @return              Ray trace handle, which must be closed via CloseHandle().
 */
native Handle TR_TraceRayEx(const float pos[3],
                            const float vec[3],
                            int flags,
                            RayType rtype);

/**
 * Starts up a new trace hull using a new trace result.
 *
 * @param pos           Starting position of the ray.
 * @param vec           Ending position of the ray.
 * @param mins          Hull minimum size.
 * @param maxs          Hull maximum size.
 * @param flags         Trace flags.
 * @return              Ray trace handle, which must be closed via CloseHandle().
 */
native Handle TR_TraceHullEx(const float pos[3],
                             const float vec[3],
                             const float mins[3],
                             const float maxs[3],
                             int flags);

/**
 * Starts up a new trace ray using a new trace result and a customized
 * trace ray filter.
 *
 * Calling TR_Trace*Filter or TR_TraceRay*Ex from inside a filter
 * function is currently not allowed and may not work.
 *
 * @param pos           Starting position of the ray.
 * @param vec           Depending on RayType, it will be used as the ending
 *                      point, or the direction angle.
 * @param flags         Trace flags.
 * @param rtype         Method to calculate the ray direction.
 * @param filter        Function to use as a filter.
 * @param data          Arbitrary data value to pass through to the filter function.
 * @return              Ray trace handle, which must be closed via CloseHandle().
 */
native Handle TR_TraceRayFilterEx(const float pos[3],
                                  const float vec[3],
                                  int flags,
                                  RayType rtype,
                                  TraceEntityFilter filter,
                                  any data=0);

/**
 * Starts up a new trace hull using a new trace result and a customized
 * trace ray filter.
 *
 * Calling TR_Trace*Filter or TR_Trace*FilterEx from inside a filter
 * function is currently not allowed and may not work.
 *
 * @param pos           Starting position of the ray.
 * @param vec           Ending position of the ray.
 * @param mins          Hull minimum size.
 * @param maxs          Hull maximum size.
 * @param flags         Trace flags.
 * @param filter        Function to use as a filter.
 * @param data          Arbitrary data value to pass through to the filter function.
 * @return              Ray trace handle, which must be closed via CloseHandle().
 */
native Handle TR_TraceHullFilterEx(const float pos[3],
                                   const float vec[3],
                                   const float mins[3],
                                   const float maxs[3],
                                   int flags,
                                   TraceEntityFilter filter,
                                   any data=0);

/**
 * Clips a ray to a particular entity.
 *
 * @param pos           Starting position of the ray.
 * @param vec           Depending on RayType, it will be used as the ending
 *                      point, or the direction angle.
 * @param flags         Trace flags.
 * @param rtype         Method to calculate the ray direction.
 * @param entity        Entity to clip to.
 * @return              Ray trace handle, which must be closed via CloseHandle().
 * @error               Invalid entity.
 */
native Handle TR_ClipRayToEntityEx(const float pos[3],
                                   const float vec[3],
                                   int flags,
                                   RayType rtype,
                                   int entity);

/**
 * Clips a ray hull to a particular entity.
 *
 * @param pos           Starting position of the ray.
 * @param vec           Ending position of the ray.
 * @param mins          Hull minimum size.
 * @param maxs          Hull maximum size.
 * @param flags         Trace flags.
 * @param entity        Entity to clip to.
 * @return              Ray trace handle, which must be closed via CloseHandle().
 * @error               Invalid entity.
 */
native Handle TR_ClipRayHullToEntityEx(const float pos[3],
                                       const float vec[3],
                                       const float mins[3],
                                       const float maxs[3],
                                       int flags,
                                       int entity);

/**
 * Clips the current global ray (or hull) to a particular entity.
 *
 * @param flags         Trace flags.
 * @param entity        Entity to clip to.
 * @return              Ray trace handle, which must be closed via CloseHandle().
 * @error               Invalid entity.
 */
native Handle TR_ClipCurrentRayToEntityEx(int flags, int entity);

/**
 * Returns the time fraction from a trace result (1.0 means no collision).
 *
 * @param hndl          A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @return              Time fraction value of the trace.
 * @error               Invalid Handle.
 */
native float TR_GetFraction(Handle hndl=INVALID_HANDLE);

/**
 * Returns the time fraction from a trace result when it left a solid.
 * Only valid if trace started in solid
 *
 * @param hndl          A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @return              Time fraction left solid value of the trace.
 * @error               Invalid Handle.
 */
native float TR_GetFractionLeftSolid(Handle hndl=INVALID_HANDLE);

/**
 * Returns the starting position of a trace.
 *
 * @param hndl          A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @param pos           Vector buffer to store data in.
 * @error               Invalid Handle.
 */
native void TR_GetStartPosition(Handle hndl, float pos[3]);

/**
 * Returns the collision position of a trace result.
 *
 * @param pos           Vector buffer to store data in.
 * @param hndl          A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @error               Invalid Handle.
 */
native void TR_GetEndPosition(float pos[3], Handle hndl=INVALID_HANDLE);

/**
 * Returns the entity index that collided with the trace.
 *
 * @param hndl          A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @return              Entity index or -1 for no collision.
 * @error               Invalid Handle.
 */
native int TR_GetEntityIndex(Handle hndl=INVALID_HANDLE);

/**
 * Returns the displacement flags for the surface that was hit. See DISPSURF_FLAG_*.
 *
 * @param hndl          A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @return              Displacement flags.
 * @error               Invalid Handle.
 */
native int TR_GetDisplacementFlags(Handle hndl=INVALID_HANDLE);

/**
 * Returns the name of the surface that was hit.
 *
 * @param hndl          A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @param buffer        Buffer to store surface name in
 * @param maxlen        Maximum length of output buffer
 * @error               Invalid Handle.
 */
native void TR_GetSurfaceName(Handle hndl, char[] buffer, int maxlen);

/**
 * Returns the surface properties index of the surface that was hit.
 *
 * @param hndl          A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @return              Surface props.
 * @error               Invalid Handle.
 */
native int TR_GetSurfaceProps(Handle hndl=INVALID_HANDLE);

/**
 * Returns the surface flags. See SURF_*.
 *
 * @param hndl          A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @return              Surface flags.
 * @error               Invalid Handle.
 */
native int TR_GetSurfaceFlags(Handle hndl=INVALID_HANDLE);

/**
 * Returns the index of the physics bone that was hit.
 *
 * @param hndl          A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @return              Physics bone index.
 * @error               Invalid Handle.
 */
native int TR_GetPhysicsBone(Handle hndl=INVALID_HANDLE);

/**
 * Returns whether the entire trace was in a solid area.
 *
 * @param hndl          A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @return              True if entire trace was in a solid area, otherwise false.
 * @error               Invalid Handle.
 */
native bool TR_AllSolid(Handle hndl=INVALID_HANDLE);

/**
 * Returns whether the initial point was in a solid area.
 *
 * @param hndl          A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @return              True if initial point was in a solid area, otherwise false.
 * @error               Invalid Handle.
 */
native bool TR_StartSolid(Handle hndl=INVALID_HANDLE);

/**
 * Returns if there was any kind of collision along the trace ray.
 *
 * @param hndl          A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @return              True if any collision found, otherwise false.
 * @error               Invalid Handle.
 */
native bool TR_DidHit(Handle hndl=INVALID_HANDLE);

/**
 * Returns in which body hit group the trace collided if any.
 *
 * @param hndl          A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @return              Body hit group.
 * @error               Invalid Handle.
 */
native int TR_GetHitGroup(Handle hndl=INVALID_HANDLE);

/**
 * Returns in which hitbox the trace collided if any. 
 *
 * Note: if the entity that collided with the trace is the world entity, 
 * then this function doesn't return an hitbox index but a static prop index.
 *
 * @param hndl          A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @return              Hitbox index (Or static prop index).
 * @error               Invalid Handle.
 */
native int TR_GetHitBoxIndex(Handle hndl=INVALID_HANDLE);

/**
 * Find the normal vector to the collision plane of a trace.
 *
 * @param hndl          A trace Handle, or INVALID_HANDLE to use a global trace result.
 * @param normal        Vector buffer to store the vector normal to the collision plane
 * @error               Invalid Handle
 */
native void TR_GetPlaneNormal(Handle hndl, float normal[3]);

/**
 * Tests a point to see if it's outside any playable area
 *
 * @param pos           Vector buffer to store data in.
 * @return              True if outside world, otherwise false.
 */
native bool TR_PointOutsideWorld(float pos[3]);
