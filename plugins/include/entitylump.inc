#if defined _entitylump_included
	#endinput
#endif

#define _entitylump_included

/**
 * A handle to a weakref of an ordered list of key / value pairs; any write operations on this handle will affect the resulting map entity string.
 * If the entry in the EntityLump is removed, the handle will error on all operations.
 * (The handle will remain valid on the scripting side, and will still need to be deleted.)
 */
methodmap EntityLumpEntry < Handle {
	/**
	 * Copies the key / value at the given index into buffers.
	 *
	 * @error Index is out of bounds.
	 */
	public native void Get(int index, char[] keybuf = "", int keylen = 0, char[] valbuf = "", int vallen = 0);
	
	/**
	 * Updates the key / value pair at the given index.
	 * NULL_STRING may be used to preserve the existing value.
	 *
	 * @error Index is out of bounds or entity lump is read-only.
	 */
	public native void Update(int index, const char[] key = NULL_STRING, const char[] value = NULL_STRING);
	
	/**
	 * Inserts a new key / value pair at the given index, shifting the pair at that index and beyond up.
	 * If EntityLumpEntry.Length is passed in, this is an append operation.
	 *
	 * @error Index is out of bounds or entity lump is read-only.
	 */
	public native void Insert(int index, const char[] key, const char[] value);
	
	/**
	 * Removes the key / value pair at the given index, shifting all entries past it down.
	 *
	 * @error Index is out of bounds or entity lump is read-only.
	 */
	public native void Erase(int index);
	
	/**
	 * Inserts a new key / value pair at the end.
	 *
	 * @error Index is out of bounds or entity lump is read-only.
	 */
	public native void Append(const char[] key, const char[] value);
	
	/**
	 * Returns the first index after `start` with the matching key, or -1 if not found.
	 */
	public native int FindKey(const char[] key, int start = -1);
	
	/**
	 * Returns the first index after `start` with the matching key, or -1 if not found.
	 * This also copies the value from that index into the given buffer.
	 *
	 * @error `start` is out of bounds and not -1.
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
	 * Returns the number of key / value pairs in the list.
	 */
	property int Length {
		public native get();
	}
};

/**
 * A group of natives for a singleton entity lump.
 * EntityLumpEntry instances are only available for writing during OnMapInit().
 */
methodmap EntityLump {
	/**
	 * Returns the EntityLumpEntry at the given index.
	 * This handle should be freed by the calling plugin.
	 *
	 * @error Index is out of bounds.
	 */
	public static native EntityLumpEntry Get(int index);
	
	/**
	 * Erases an EntityLumpEntry at the given index, shifting all entries past it down.
	 * Any handles referencing the erased EntityLumpEntry will throw on any operations aside from delete.
	 *
	 * @error Index is out of bounds or entity lump is read-only.
	 */
	public static native void Erase(int index);
	
	/**
	 * Inserts an empty EntityLumpEntry at the given index, shifting the existing entry and ones past it up.
	 *
	 * @error Index is out of bounds or entity lump is read-only.
	 */
	public static native void Insert(int index);
	
	/**
	 * Creates an empty EntityLumpEntry, returning its index.
	 *
	 * @error Entity lump is read-only.
	 */
	public static native int Append();
	
	/**
	 * Returns the number of entities currently in the lump.
	 */
	public static native int Length();
};
