#if defined _entitylump_included
	#endinput
#endif

#define _entitylump_included

/**
 * An ordered list of key / value pairs for a map entity.
 * If the entry in the EntityLump is removed, the handle will error on all operations.
 * (The handle will remain valid on the scripting side, and will still need to be deleted.)
 *
 * Write operations (update, insert, erase, append) are only allowed during OnMapInit.
 */
methodmap EntityLumpEntry < Handle {
	/**
	 * Copies the key / value at the given index into buffers.
	 *
	 * @param index     Position, starting from 0.
	 * @param keybuf    Key name buffer.
	 * @param keylen    Maximum length of the key name buffer.
	 * @param valbuf    Value buffer.
	 * @param vallen    Maximum length of the value buffer.
	 * @error           Index is out of bounds.
	 */
	public native void Get(int index, char[] keybuf = "", int keylen = 0, char[] valbuf = "", int vallen = 0);
	
	/**
	 * Updates the key / value pair at the given index.
	 *
	 * @param index    Position, starting from 0.
	 * @param key      New key name, or NULL_STRING to preserve the existing key name.
	 * @param value    New value, or NULL_STRING to preserve the existing value.
	 * @error          Index is out of bounds or entity lump is read-only.
	 */
	public native void Update(int index, const char[] key = NULL_STRING, const char[] value = NULL_STRING);
	
	/**
	 * Inserts a new key / value pair at the given index, shifting the pair at that index and beyond up.
	 * If EntityLumpEntry.Length is passed in, this is an append operation.
	 *
	 * @param index    Position, starting from 0.
	 * @param key      New key name.
	 * @param value    New value.
	 * @error          Index is out of bounds or entity lump is read-only.
	 */
	public native void Insert(int index, const char[] key, const char[] value);
	
	/**
	 * Removes the key / value pair at the given index, shifting all entries past it down.
	 *
	 * @param index    Position, starting from 0.
	 * @error          Index is out of bounds or entity lump is read-only.
	 */
	public native void Erase(int index);
	
	/**
	 * Inserts a new key / value pair at the end of the entry's list.
	 *
	 * @param key      New key name.
	 * @param value    New value.
	 * @error          Index is out of bounds or entity lump is read-only.
	 */
	public native void Append(const char[] key, const char[] value);
	
	/**
	 * Searches the entry list for an index matching a key starting from a position.
	 *
	 * @param key      Key name to search.
	 * @param start    A position after which to begin searching from.  Use -1 to start from the
	 *                 first entry.
	 * @return         Position after start with an entry matching the given key, or -1 if no
	 *                 match was found.
	 * @error          Invalid start position; must be a value between -1 and one less than the
	 *                 length of the entry.
	 */
	public native int FindKey(const char[] key, int start = -1);
	
	/**
	 * Searches the entry list for an index matching a key starting from a position.
	 * This also copies the value from that index into the given buffer.
	 *
	 * This can be used to find the first / only value matching a key, or to iterate over all
	 * the values that match said key.
	 *
	 * @param key       Key name to search.
	 * @param buffer    Value buffer.  This will contain the result of the next match, or empty
	 *                  if no match was found.
	 * @param maxlen    Maximum length of the value buffer.
	 * @param start     An index after which to begin searching from.  Use -1 to start from the
	 *                  first entry.
	 * @return          Position after start with an entry matching the given key, or -1 if no
	 *                  match was found.
	 * @error           Invalid start position; must be a value between -1 and one less than the
	 *                  length of the entry.
	 */
	public int GetNextKey(const char[] key, char[] buffer, int maxlen, int start = -1) {
		int result = this.FindKey(key, start);
		if (result != -1) {
			this.Get(result, .valbuf = buffer, .vallen = maxlen);
		} else {
			buffer[0] = '\0';
		}
		return result;
	}
	
	/**
	 * Retrieves the number of key / value pairs in the entry.
	 */
	property int Length {
		public native get();
	}
};

/**
 * A group of natives for a singleton entity lump, representing all the entities defined in the map.
 *
 * Write operations (insert, erase, append) are only allowed during OnMapInit.
 */
methodmap EntityLump {
	/**
	 * Returns the EntityLumpEntry at the given index.
	 * This handle should be freed by the calling plugin.
	 *
	 * @param index    Position, starting from 0.
	 * @error          Index is out of bounds.
	 */
	public static native EntityLumpEntry Get(int index);
	
	/**
	 * Erases an EntityLumpEntry at the given index, shifting all entries past it down.
	 * Any handles referencing the erased EntityLumpEntry will throw on any operations aside from delete.
	 *
	 * @param index    Position, starting from 0.
	 * @error          Index is out of bounds or entity lump is read-only.
	 */
	public static native void Erase(int index);
	
	/**
	 * Inserts an empty EntityLumpEntry at the given index, shifting the existing entry and ones past it up.
	 *
	 * @param index    Position, starting from 0.
	 * @error          Index is out of bounds or entity lump is read-only.
	 */
	public static native void Insert(int index);
	
	/**
	 * Creates an empty EntityLumpEntry, returning its index.
	 *
	 * @error    Entity lump is read-only.
	 */
	public static native int Append();
	
	/**
	 * Returns the number of entities currently in the lump.
	 */
	public static native int Length();
};
