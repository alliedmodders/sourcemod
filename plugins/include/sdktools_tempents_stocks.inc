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

#if defined _te_stocks_included
 #endinput
#endif
#define _te_stocks_included

/**
 * @section TE Explosion flags.
 */
#define TE_EXPLFLAG_NONE		0x0	/**< all flags clear makes default Half-Life explosion */
#define TE_EXPLFLAG_NOADDITIVE		0x1	/**< sprite will be drawn opaque (ensure that the sprite you send is a non-additive sprite) */
#define TE_EXPLFLAG_NODLIGHTS		0x2	/**< do not render dynamic lights */
#define TE_EXPLFLAG_NOSOUND		0x4	/**< do not play client explosion sound */
#define TE_EXPLFLAG_NOPARTICLES		0x8	/**< do not draw particles */
#define TE_EXPLFLAG_DRAWALPHA		0x10	/**< sprite will be drawn alpha */
#define TE_EXPLFLAG_ROTATE		0x20	/**< rotate the sprite randomly */
#define TE_EXPLFLAG_NOFIREBALL		0x40	/**< do not draw a fireball */
#define TE_EXPLFLAG_NOFIREBALLSMOKE	0x80	/**< do not draw smoke with the fireball */

/**
 * @endsection
 */

/**
 * @section TE Beam flags.
 */
#define FBEAM_STARTENTITY 	0x00000001
#define	FBEAM_ENDENTITY		0x00000002
#define	FBEAM_FADEIN		0x00000004
#define	FBEAM_FADEOUT		0x00000008
#define	FBEAM_SINENOISE		0x00000010
#define	FBEAM_SOLID		0x00000020
#define	FBEAM_SHADEIN		0x00000040
#define	FBEAM_SHADEOUT		0x00000080
#define	FBEAM_ONLYNOISEONCE	0x00000100 	/**< Only calculate our noise once */
#define	FBEAM_NOTILE		0x00000200
#define	FBEAM_USE_HITBOXES	0x00000400	/**< Attachment indices represent hitbox indices instead when this is set. */
#define	FBEAM_STARTVISIBLE	0x00000800	/**< Has this client actually seen this beam's start entity yet? */
#define	FBEAM_ENDVISIBLE	0x00001000	/**< Has this client actually seen this beam's end entity yet? */
#define	FBEAM_ISACTIVE		0x00002000
#define	FBEAM_FOREVER		0x00004000
#define	FBEAM_HALOBEAM		0x00008000	/**< When drawing a beam with a halo, don't ignore the segments and endwidth */

/**
 * @endsection
 */

/**
 * Sets up a sparks effect.
 *
 * @param pos			Position of the sparks.
 * @param dir			Direction of the sparks.
 * @param Magnitude		Sparks size.
 * @param TrailLength		Trail lenght of the sparks.
 * @noreturn
 */
stock TE_SetupSparks(const Float:pos[3], const Float:dir[3], Magnitude, TrailLength)
{
	TE_Start("Sparks");
	TE_WriteVector("m_vecOrigin[0]", pos);
	TE_WriteVector("m_vecDir", dir);
	TE_WriteNum("m_nMagnitude", Magnitude);
	TE_WriteNum("m_nTrailLength", TrailLength);
}

/**
 * Sets up a smoke effect.
 *
 * @param pos			Position of the smoke.
 * @param Model			Precached model index.
 * @param Scale			Scale of the smoke.
 * @param Framerate		Frame rate of the smoke.
 * @noreturn
 */
stock TE_SetupSmoke(const Float:pos[3], Model, Float:Scale, FrameRate)
{
	TE_Start("Smoke");
	TE_WriteVector("m_vecOrigin", pos);
	TE_WriteNum("m_nModelIndex", Model);
	TE_WriteFloat("m_fScale", Scale);
	TE_WriteNum("m_nFrameRate", FrameRate);
}

/**
 * Sets up a dust cloud effect.
 *
 * @param pos			Position of the dust.
 * @param dir			Direction of the dust.
 * @param Size			Dust cloud size.
 * @param Speed			Dust cloud speed.
 * @noreturn
 */
stock TE_SetupDust(const Float:pos[3], const Float:dir[3], Float:Size, Float:Speed)
{
	TE_Start("Dust");
	TE_WriteVector("m_vecOrigin[0]", pos);
	TE_WriteVector("m_vecDirection", dir);
	TE_WriteFloat("m_flSize", Size);
	TE_WriteFloat("m_flSpeed", Speed);
}

/**
 * Sets up a muzzle flash effect.
 *
 * @param pos			Position of the muzzle flash.
 * @param angles		Rotation angles of the muzzle flash.
 * @param Scale			Scale of the muzzle flash.
 * @param Type			Muzzle flash type to render (Mod specific).
 * @noreturn
 */
stock TE_SetupMuzzleFlash(const Float:pos[3], const Float:angles[3], Float:Scale, Type)
{
	TE_Start("MuzzleFlash");
	TE_WriteVector("m_vecOrigin", pos);
	TE_WriteVector("m_vecAngles", angles);
	TE_WriteFloat("m_flScale", Scale);
	TE_WriteNum("m_nType", Type);
}

/**
 * Sets up a metal sparks effect.
 *
 * @param pos			Position of the metal sparks.
 * @param dir			Direction of the metal sparks.
 * @noreturn
 */
stock TE_SetupMetalSparks(const Float:pos[3], const Float:dir[3])
{
	TE_Start("Metal Sparks");
	TE_WriteVector("m_vecPos", pos);
	TE_WriteVector("m_vecDir", dir);
}

/**
 * Sets up an energy splash effect.
 *
 * @param pos			Position of the energy splash.
 * @param dir			Direction of the energy splash.
 * @param Explosive		Makes the effect explosive.
 * @noreturn
 */
stock TE_SetupEnergySplash(const Float:pos[3], const Float:dir[3], bool:Explosive)
{
	TE_Start("Energy Splash");
	TE_WriteVector("m_vecPos", pos);
	TE_WriteVector("m_vecDir", dir);
	TE_WriteNum("m_bExplosive", Explosive);
}

/**
 * Sets up an armor ricochet effect.
 *
 * @param pos			Position of the armor ricochet.
 * @param dir			Directon of the armor ricochet.
 * @noreturn
 */
stock TE_SetupArmorRicochet(const Float:pos[3], const Float:dir[3])
{
	TE_Start("Armor Ricochet");
	TE_WriteVector("m_vecPos", pos);
	TE_WriteVector("m_vecDir", dir);
}

/**
 * Sets up a glowing sprite effect.
 *
 * @param pos			Position of the sprite.
 * @param Model			Precached model index.
 * @param Life			Time duration of the sprite.
 * @param Size			Sprite size.
 * @param Brightness		Sprite brightness.
 * @noreturn
 */
stock TE_SetupGlowSprite(const Float:pos[3], Model, Float:Life, Float:Size, Brightness)
{
	TE_Start("GlowSprite");
	TE_WriteVector("m_vecOrigin", pos);
	TE_WriteNum("m_nModelIndex", Model);
	TE_WriteFloat("m_fScale", Size);
	TE_WriteFloat("m_fLife", Life);
	TE_WriteNum("m_nBrightness", Brightness);
}

/**
 * Sets up a explosion effect.
 *
 * @param pos			Explosion position.
 * @param Model			Precached model index.
 * @param Scale			Explosion scale.
 * @param Framerate		Explosion frame rate.
 * @param Flags			Explosion flags.
 * @param Radius		Explosion radius.
 * @param Magnitude		Explosion size.
 * @param normal		Normal vector to the explosion.
 * @param MaterialType		Exploded material type.
 * @noreturn
 */
stock TE_SetupExplosion(const Float:pos[3], Model, Float:Scale, Framerate, Flags, Radius, Magnitude, const Float:normal[3]={0.0, 0.0, 1.0}, MaterialType='C')
{
	TE_Start("Explosion");
	TE_WriteVector("m_vecOrigin[0]", pos);
	TE_WriteVector("m_vecNormal", normal);
	TE_WriteNum("m_nModelIndex", Model);
	TE_WriteFloat("m_fScale", Scale);
	TE_WriteNum("m_nFrameRate", Framerate);
	TE_WriteNum("m_nFlags", Flags);
	TE_WriteNum("m_nRadius", Radius);
	TE_WriteNum("m_nMagnitude", Magnitude);
	TE_WriteNum("m_chMaterialType", MaterialType);
}

/**
 * Sets up a blood sprite effect.
 *
 * @param pos			Position of the sprite.
 * @param dir			Sprite direction.
 * @param color			Color array (r, g, b, a).
 * @param Size			Sprite size.
 * @param SprayModel		Precached model index.
 * @param BloodDropModel	Precached model index.
 * @noreturn
 */
stock TE_SetupBloodSprite(const Float:pos[3], const Float:dir[3], const color[4], Size, SprayModel, BloodDropModel)
{
	TE_Start("Blood Sprite");
	TE_WriteVector("m_vecOrigin", pos);
	TE_WriteVector("m_vecDirection", dir);
	TE_WriteNum("r", color[0]);
	TE_WriteNum("g", color[1]);
	TE_WriteNum("b", color[2]);
	TE_WriteNum("a", color[3]);
	TE_WriteNum("m_nSize", Size);
	TE_WriteNum("m_nSprayModel", SprayModel);
	TE_WriteNum("m_nDropModel", BloodDropModel);
}

/**
 * Sets up a beam ring point effect.
 *
 * @param center		Center position of the ring.
 * @param Start_Radius		Initial ring radius.
 * @param End_Radius		Final ring radius.
 * @param ModelIndex		Precached model index.
 * @param HaloIndex		Precached model index.
 * @param StartFrame		Initital frame to render.
 * @param FrameRate		Ring frame rate.
 * @param Life			Time duration of the ring.
 * @param Width			Beam width.
 * @param Amplitude		Beam amplitude.
 * @param color			Color array (r, g, b, a).
 * @param Speed			Speed of the beam.
 * @param Flags			Beam flags.
 * @noreturn
 */
stock TE_SetupBeamRingPoint(const Float:center[3], Float:Start_Radius, Float:End_Radius, ModelIndex, HaloIndex, StartFrame, 
				FrameRate, Float:Life, Float:Width, Float:Amplitude, const Color[4], Speed, Flags)
{
	TE_Start("BeamRingPoint");
	TE_WriteVector("m_vecCenter", center);
	TE_WriteFloat("m_flStartRadius", Start_Radius);
	TE_WriteFloat("m_flEndRadius", End_Radius);
	TE_WriteNum("m_nModelIndex", ModelIndex);
	TE_WriteNum("m_nHaloIndex", HaloIndex);
	TE_WriteNum("m_nStartFrame", StartFrame);
	TE_WriteNum("m_nFrameRate", FrameRate);
	TE_WriteFloat("m_fLife", Life);
	TE_WriteFloat("m_fWidth", Width);
	TE_WriteFloat("m_fEndWidth", Width);
	TE_WriteFloat("m_fAmplitude", Amplitude);
	TE_WriteNum("r", Color[0]);
	TE_WriteNum("g", Color[1]);
	TE_WriteNum("b", Color[2]);
	TE_WriteNum("a", Color[3]);
	TE_WriteNum("m_nSpeed", Speed);
	TE_WriteNum("m_nFlags", Flags);
	TE_WriteNum("m_nFadeLength", 0);
}

/**
 * Sets up a point to point beam effect.
 *
 * @param start			Start position of the beam.
 * @param end			End position of the beam.
 * @param ModelIndex		Precached model index.
 * @param HaloIndex		Precached model index.
 * @param StartFrame		Initital frame to render.
 * @param FrameRate		Beam frame rate.
 * @param Life			Time duration of the beam.
 * @param Width			Initial beam width.
 * @param EndWidth		Final beam width.
 * @param FadeLength		Beam fade time duration.
 * @param Amplitude		Beam amplitude.
 * @param color			Color array (r, g, b, a).
 * @param Speed			Speed of the beam.
 * @noreturn
 */
stock TE_SetupBeamPoints(const Float:start[3], const Float:end[3], ModelIndex, HaloIndex, StartFrame, FrameRate, Float:Life, 
				Float:Width, Float:EndWidth, FadeLength, Float:Amplitude, const Color[4], Speed)
{
	TE_Start("BeamPoints");
	TE_WriteVector("m_vecStartPoint", start);
	TE_WriteVector("m_vecEndPoint", end);
	TE_WriteNum("m_nModelIndex", ModelIndex);
	TE_WriteNum("m_nHaloIndex", HaloIndex);
	TE_WriteNum("m_nStartFrame", StartFrame);
	TE_WriteNum("m_nFrameRate", FrameRate);
	TE_WriteFloat("m_fLife", Life);
	TE_WriteFloat("m_fWidth", Width);
	TE_WriteFloat("m_fEndWidth", EndWidth);
	TE_WriteFloat("m_fAmplitude", Amplitude);
	TE_WriteNum("r", Color[0]);
	TE_WriteNum("g", Color[1]);
	TE_WriteNum("b", Color[2]);
	TE_WriteNum("a", Color[3]);
	TE_WriteNum("m_nSpeed", Speed);
	TE_WriteNum("m_nFadeLength", FadeLength);
}

/**
 * Sets up an entity to entity laser effect.
 *
 * @param StartEntity		Entity index from where the beam starts.
 * @param EndEntity		Entity index from where the beam ends.
 * @param ModelIndex		Precached model index.
 * @param HaloIndex		Precached model index.
 * @param StartFrame		Initital frame to render.
 * @param FrameRate		Beam frame rate.
 * @param Life			Time duration of the beam.
 * @param Width			Initial beam width.
 * @param EndWidth		Final beam width.
 * @param FadeLength		Beam fade time duration.
 * @param Amplitude		Beam amplitude.
 * @param color			Color array (r, g, b, a).
 * @param Speed			Speed of the beam.
 * @noreturn
 */
stock TE_SetupBeamLaser(StartEntity, EndEntity, ModelIndex, HaloIndex, StartFrame, FrameRate, Float:Life, 
				Float:Width, Float:EndWidth, FadeLength, Float:Amplitude, const Color[4], Speed)
{
	TE_Start("BeamLaser");
	TE_WriteEncodedEnt("m_nStartEntity", StartEntity);
	TE_WriteEncodedEnt("m_nEndEntity", EndEntity);
	TE_WriteNum("m_nModelIndex", ModelIndex);
	TE_WriteNum("m_nHaloIndex", HaloIndex);
	TE_WriteNum("m_nStartFrame", StartFrame);
	TE_WriteNum("m_nFrameRate", FrameRate);
	TE_WriteFloat("m_fLife", Life);
	TE_WriteFloat("m_fWidth", Width);
	TE_WriteFloat("m_fEndWidth", EndWidth);
	TE_WriteFloat("m_fAmplitude", Amplitude);
	TE_WriteNum("r", Color[0]);
	TE_WriteNum("g", Color[1]);
	TE_WriteNum("b", Color[2]);
	TE_WriteNum("a", Color[3]);
	TE_WriteNum("m_nSpeed", Speed);
	TE_WriteNum("m_nFadeLength", FadeLength);
}

/**
 * Sets up a beam ring effect.
 *
 * @param StartEntity		Entity index from where the ring starts.
 * @param EndEntity		Entity index from where the ring ends.
 * @param ModelIndex		Precached model index.
 * @param HaloIndex		Precached model index.
 * @param StartFrame		Initital frame to render.
 * @param FrameRate		Ring frame rate.
 * @param Life			Time duration of the ring.
 * @param Width			Beam width.
 * @param Amplitude		Beam amplitude.
 * @param color			Color array (r, g, b, a).
 * @param Speed			Speed of the beam.
 * @param Flags			Beam flags.
 * @noreturn
 */
stock TE_SetupBeamRing(StartEntity, EndEntity, ModelIndex, HaloIndex, StartFrame, FrameRate, Float:Life, Float:Width, Float:Amplitude, const Color[4], Speed, Flags)
{
	TE_Start("BeamRing");
	TE_WriteEncodedEnt("m_nStartEntity", StartEntity);
	TE_WriteEncodedEnt("m_nEndEntity", EndEntity);
	TE_WriteNum("m_nModelIndex", ModelIndex);
	TE_WriteNum("m_nHaloIndex", HaloIndex);
	TE_WriteNum("m_nStartFrame", StartFrame);
	TE_WriteNum("m_nFrameRate", FrameRate);
	TE_WriteFloat("m_fLife", Life);
	TE_WriteFloat("m_fWidth", Width);
	TE_WriteFloat("m_fEndWidth", Width);
	TE_WriteFloat("m_fAmplitude", Amplitude);
	TE_WriteNum("r", Color[0]);
	TE_WriteNum("g", Color[1]);
	TE_WriteNum("b", Color[2]);
	TE_WriteNum("a", Color[3]);
	TE_WriteNum("m_nSpeed", Speed);
	TE_WriteNum("m_nFadeLength", 0);
	TE_WriteNum("m_nFlags", Flags);
}

/**
 * Sets up a follow beam effect.
 *
 * @param EntIndex		Entity index from where the beam starts.
 * @param ModelIndex		Precached model index.
 * @param HaloIndex		Precached model index.
 * @param Life			Time duration of the beam.
 * @param Width			Initial beam width.
 * @param EndWidth		Final beam width.
 * @param FadeLength		Beam fade time duration.
 * @param color			Color array (r, g, b, a).
 * @noreturn
 */
stock TE_SetupBeamFollow(EntIndex, ModelIndex, HaloIndex, Float:Life, Float:Width, Float:EndWidth, FadeLength, const Color[4])
{
	TE_Start("BeamFollow");
	TE_WriteEncodedEnt("m_iEntIndex", EntIndex);
	TE_WriteNum("m_nModelIndex", ModelIndex);
	TE_WriteNum("m_nHaloIndex", HaloIndex);
	TE_WriteNum("m_nStartFrame", 0);
	TE_WriteNum("m_nFrameRate", 0);
	TE_WriteFloat("m_fLife", Life);
	TE_WriteFloat("m_fWidth", Width);
	TE_WriteFloat("m_fEndWidth", EndWidth);
	TE_WriteNum("m_nFadeLength", FadeLength);
	TE_WriteNum("r", Color[0]);
	TE_WriteNum("g", Color[1]);
	TE_WriteNum("b", Color[2]);
	TE_WriteNum("a", Color[3]);
}
