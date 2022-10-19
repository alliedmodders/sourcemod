#pragma semicolon 1
#include <sourcemod>

#include <sdktools>
#include <entitylump>

#pragma newdecls required

#define PLUGIN_VERSION "0.0.0"
public Plugin myinfo = {
	name = "Entity Lump Core Native Test",
	author = "nosoop",
	description = "A port of the Level KeyValues entity test to the Entity Lump implementation in core SourceMod",
}

#define OUTPUT_NAME "OnCapTeam2"

public void OnMapInit() {
	// set every area_time_to_cap value to 30
	for (int i, n = EntityLump.Length(); i < n; i++) {
		EntityLumpEntry entry = EntityLump.Get(i);
		
		int ttc = entry.FindKey("area_time_to_cap");
		if (ttc != -1) {
			entry.Update(ttc, NULL_STRING, "30");
			PrintToServer("Set time to cap for item %d to 30", i);
		}
		
		delete entry;
	}
}

public void OnMapStart() {
	int captureArea = FindEntityByClassname(-1, "trigger_capture_area");
	
	if (!IsValidEntity(captureArea)) {
		LogMessage("---- %s", "No capture area");
		return;
	}
	
	int hammerid = GetEntProp(captureArea, Prop_Data, "m_iHammerID");
	EntityLumpEntry entry = FindEntityLumpEntryByHammerID(hammerid);
	
	if (!entry) {
		return;
	}
	
	LogMessage("---- %s", "Found a trigger_capture_area with keys:");
	
	for (int i, n = entry.Length; i < n; i++) {
		char keyBuffer[128], valueBuffer[128];
		entry.Get(i, keyBuffer, sizeof(keyBuffer), valueBuffer, sizeof(valueBuffer));
		
		LogMessage("%s -> %s", keyBuffer, valueBuffer);
	}
	
	LogMessage("---- %s", "List of " ... OUTPUT_NAME ... " outputs:");
	char outputString[256];
	for (int k = -1; (k = entry.GetNextKey(OUTPUT_NAME, outputString, sizeof(outputString), k)) != -1;) {
		char targetName[32], inputName[64], variantValue[32];
		float delay;
		int nFireCount;
		
		ParseEntityOutputString(outputString, targetName, sizeof(targetName),
				inputName, sizeof(inputName), variantValue, sizeof(variantValue),
				delay, nFireCount);
		
		LogMessage("target %s -> input %s (value %s, delay %.2f, refire %d)",
				targetName, inputName, variantValue, delay, nFireCount);
	}
	delete entry;
}

/**
 * Returns the first EntityLumpEntry with a matching hammerid.
 */
EntityLumpEntry FindEntityLumpEntryByHammerID(int hammerid) {
	for (int i, n = EntityLump.Length(); i < n; i++) {
		EntityLumpEntry entry = EntityLump.Get(i);
		
		char value[32];
		if (entry.GetNextKey("hammerid", value, sizeof(value)) != -1
				&& StringToInt(value) == hammerid) {
			return entry;
		}
		delete entry;
	}
	return null;
}

/**
 * Parses an entity's output value (as formatted in the entity string).
 * Refer to https://developer.valvesoftware.com/wiki/AddOutput for the format.
 * 
 * @return True if the output string was successfully parsed, false if not.
 */
stock bool ParseEntityOutputString(const char[] output, char[] targetName, int targetNameLength,
		char[] inputName, int inputNameLength, char[] variantValue, int variantValueLength,
		float &delay, int &nFireCount) {
	int delimiter;
	char buffer[32];
	
	{
		// validate that we have something resembling an output string (four commas)
		int i, c, nDelim;
		while ((c = FindCharInString(output[i], ',')) != -1) {
			nDelim++;
			i += c + 1;
		}
		if (nDelim < 4) {
			return false;
		}
	}
	
	delimiter = SplitString(output, ",", targetName, targetNameLength);
	delimiter += SplitString(output[delimiter], ",", inputName, inputNameLength);
	delimiter += SplitString(output[delimiter], ",", variantValue, variantValueLength);
	
	delimiter += SplitString(output[delimiter], ",", buffer, sizeof(buffer));
	delay = StringToFloat(buffer);
	
	nFireCount = StringToInt(output[delimiter]);
	
	return true;
}
