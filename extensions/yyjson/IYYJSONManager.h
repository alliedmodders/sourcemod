#ifndef _INCLUDE_SM_YYJSON_IYYJSONMANAGER_H_
#define _INCLUDE_SM_YYJSON_IYYJSONMANAGER_H_

#include <IHandleSys.h>

using SourceMod::Handle_t;
using SourceMod::HandleType_t;
using SourceMod::SMInterface;
using SourcePawn::IPluginContext;

// Forward declaration
class YYJSONValue;

#define SMINTERFACE_YYJSONMANAGER_NAME "IYYJSONManager"
#define SMINTERFACE_YYJSONMANAGER_VERSION 1
#define YYJSON_PACK_ERROR_SIZE 256
#define YYJSON_ERROR_BUFFER_SIZE 256

/**
 * @brief JSON sorting order
 */
enum YYJSON_SORT_ORDER
{
	YYJSON_SORT_ASC = 0,      // Ascending order
	YYJSON_SORT_DESC = 1,     // Descending order
	YYJSON_SORT_RANDOM = 2    // Random order
};

/**
 * @brief Parameter provider interface for Pack operation
 * 
 * Allows Pack to retrieve parameters in a platform-independent way.
 */
class IPackParamProvider
{
public:
	virtual ~IPackParamProvider() = default;

	virtual bool GetNextString(const char** out_str) = 0;
	virtual bool GetNextInt(int* out_value) = 0;
	virtual bool GetNextFloat(float* out_value) = 0;
	virtual bool GetNextBool(bool* out_value) = 0;
};

/**
 * @brief YYJSON Manager Interface
 *
 * This interface provides complete JSON manipulation capabilities.
 * It's designed to be consumed by other SourceMod C++ extensions
 * without requiring them to link against yyjson library.
 *
 * @usage
 * IYYJSONManager* g_pYYJSONManager = nullptr;
 *
 * bool YourExtension::SDK_OnAllLoaded()
 * {
 *     SM_GET_LATE_IFACE(YYJSONMANAGER, g_pYYJSONManager);
 * }
 */
class IYYJSONManager : public SMInterface
{
public:
	virtual const char *GetInterfaceName() override {
		return SMINTERFACE_YYJSONMANAGER_NAME;
	}

	virtual unsigned int GetInterfaceVersion() override {
		return SMINTERFACE_YYJSONMANAGER_VERSION;
	}

public:
	/**
	 * Parse JSON from string or file
	 * @param json_str JSON string or file path
	 * @param is_file true if json_str is a file path
	 * @param is_mutable true to create mutable document (default: false)
	 * @param read_flg Read flags (YYJSON_READ_FLAG values, default: 0)
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return JSON value pointer or nullptr on error
	 */
	virtual YYJSONValue* ParseJSON(const char* json_str, bool is_file, bool is_mutable = false, 
		uint32_t read_flg = 0, char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Write JSON to string
	 * @param handle JSON value
	 * @param buffer Output buffer
	 * @param buffer_size Buffer size
	 * @param write_flg Write flags (YYJSON_WRITE_FLAG values, default: 0)
	 * @param out_size Pointer to receive actual size written (including null terminator), optional
	 * @return true on success, false if buffer is too small or on error
	 * 
	 * @note The out_size parameter returns the size including null terminator
	 * @note Use GetSerializedSize() with the same write_flg to determine buffer size
	 */
	virtual bool WriteToString(YYJSONValue* handle, char* buffer, size_t buffer_size, 
		uint32_t write_flg = 0, size_t* out_size = nullptr) = 0;

	/**
	 * Write JSON to file
	 * @param handle JSON value
	 * @param path File path
	 * @param write_flg Write flags (YYJSON_WRITE_FLAG values, default: 0)
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success
	 */
	virtual bool WriteToFile(YYJSONValue* handle, const char* path, uint32_t write_flg = 0,
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Compare two JSON values for equality
	 * @param handle1 First JSON value to compare
	 * @param handle2 Second JSON value to compare
	 * @return true if values are equal, false otherwise
	 * @note Compares structure and content recursively
	 */
	virtual bool Equals(YYJSONValue* handle1, YYJSONValue* handle2) = 0;

	/**
	 * Deep copy a JSON value into a target document
	 * @param targetDoc Target document that will own the copied value
	 * @param sourceValue Source value to copy
	 * @return New JSON value (deep copy) or nullptr on failure
	 * @note The returned value is owned by targetDoc's document context
	 */
	virtual YYJSONValue* DeepCopy(YYJSONValue* targetDoc, YYJSONValue* sourceValue) = 0;

	/**
	 * Get human-readable type description string
	 * @param handle JSON value
	 * @return Type description string (e.g., "object", "array", "string", "number", "true", "false", "unknown")
	 */
	virtual const char* GetTypeDesc(YYJSONValue* handle) = 0;

	/**
	 * Get the size needed to serialize this JSON value
	 * 
	 * @param handle JSON value
	 * @param write_flg Write flags (YYJSON_WRITE_FLAG values, default: 0)
	 * @return Size in bytes (including null terminator)
	 * 
	 * @note The returned size depends on the write_flg parameter.
	 *       You MUST use the same flags when calling both GetSerializedSize() 
	 *       and WriteToString(). Using different flags will return 
	 *       different sizes and may cause buffer overflow.
	 * 
	 * @example
	 *   // Correct usage:
	 *   auto flags = YYJSON_WRITE_PRETTY;
	 *   size_t size = g_pYYJSONManager->GetSerializedSize(handle, flags);
	 *   char* buffer = new char[size];
	 *   g_pYYJSONManager->WriteToString(handle, buffer, size, flags);  // Use same flags
	 */
	virtual size_t GetSerializedSize(YYJSONValue* handle, uint32_t write_flg = 0) = 0;

	/**
	 * Convert immutable document to mutable
	 * @param handle Immutable JSON value
	 * @return New mutable JSON value or nullptr if already mutable or on error
	 * @note Creates a deep copy as a mutable document
	 */
	virtual YYJSONValue* ToMutable(YYJSONValue* handle) = 0;

	/**
	 * Convert mutable document to immutable
	 * @param handle Mutable JSON value
	 * @return New immutable JSON value or nullptr if already immutable or on error
	 * @note Creates a deep copy as an immutable document
	 */
	virtual YYJSONValue* ToImmutable(YYJSONValue* handle) = 0;

	/**
	 * Get JSON type
	 * @param handle JSON value
	 * @return YYJSON_TYPE value
	 */
	virtual uint8_t GetType(YYJSONValue* handle) = 0;

	/**
	 * Get JSON subtype
	 * @param handle JSON value
	 * @return YYJSON_SUBTYPE value
	 */
	virtual uint8_t GetSubtype(YYJSONValue* handle) = 0;

	// Type checking methods

	/**
	 * Check if value is an array
	 * @param handle JSON value
	 * @return true if value is an array
	 */
	virtual bool IsArray(YYJSONValue* handle) = 0;

	/**
	 * Check if value is an object
	 * @param handle JSON value
	 * @return true if value is an object
	 */
	virtual bool IsObject(YYJSONValue* handle) = 0;

	/**
	 * Check if value is an integer (signed or unsigned)
	 * @param handle JSON value
	 * @return true if value is an integer
	 */
	virtual bool IsInt(YYJSONValue* handle) = 0;

	/**
	 * Check if value is an unsigned integer
	 * @param handle JSON value
	 * @return true if value is an unsigned integer
	 */
	virtual bool IsUint(YYJSONValue* handle) = 0;

	/**
	 * Check if value is a signed integer
	 * @param handle JSON value
	 * @return true if value is a signed integer
	 */
	virtual bool IsSint(YYJSONValue* handle) = 0;

	/**
	 * Check if value is a number (integer or real)
	 * @param handle JSON value
	 * @return true if value is a number
	 */
	virtual bool IsNum(YYJSONValue* handle) = 0;

	/**
	 * Check if value is a boolean (true or false)
	 * @param handle JSON value
	 * @return true if value is a boolean
	 */
	virtual bool IsBool(YYJSONValue* handle) = 0;

	/**
	 * Check if value is boolean true
	 * @param handle JSON value
	 * @return true if value is boolean true
	 */
	virtual bool IsTrue(YYJSONValue* handle) = 0;

	/**
	 * Check if value is boolean false
	 * @param handle JSON value
	 * @return true if value is boolean false
	 */
	virtual bool IsFalse(YYJSONValue* handle) = 0;

	/**
	 * Check if value is a floating-point number
	 * @param handle JSON value
	 * @return true if value is a floating-point number
	 */
	virtual bool IsFloat(YYJSONValue* handle) = 0;

	/**
	 * Check if value is a string
	 * @param handle JSON value
	 * @return true if value is a string
	 */
	virtual bool IsStr(YYJSONValue* handle) = 0;

	/**
	 * Check if value is null
	 * @param handle JSON value
	 * @return true if value is null
	 */
	virtual bool IsNull(YYJSONValue* handle) = 0;

	/**
	 * Check if value is a container (object or array)
	 * @param handle JSON value
	 * @return true if value is a container
	 */
	virtual bool IsCtn(YYJSONValue* handle) = 0;

	/**
	 * Check if document is mutable
	 * @param handle JSON value
	 * @return true if document is mutable
	 */
	virtual bool IsMutable(YYJSONValue* handle) = 0;

	/**
	 * Check if document is immutable
	 * @param handle JSON value
	 * @return true if document is immutable
	 */
	virtual bool IsImmutable(YYJSONValue* handle) = 0;

	/**
	 * Get the number of bytes read when parsing this document
	 * @param handle JSON value
	 * @return Number of bytes read during parsing (excluding null terminator), 0 if not from parsing
	 * 
	 * @note This value only applies to documents created from parsing
	 * @note Manually created documents (ObjectInit, CreateBool, etc.) will return 0
	 * @note The returned size does not include the null terminator
	 */
	virtual size_t GetReadSize(YYJSONValue* handle) = 0;

	/**
	 * Create an empty mutable JSON object
	 * @return New mutable JSON object or nullptr on failure
	 */
	virtual YYJSONValue* ObjectInit() = 0;

	/**
	 * Create a JSON object from key-value string pairs
	 * @param pairs Array of strings [key1, val1, key2, val2, ...]
	 * @param count Number of key-value pairs
	 * @return New JSON object or nullptr on failure
	 */
	virtual YYJSONValue* ObjectInitWithStrings(const char** pairs, size_t count) = 0;

	/**
	 * Parse a JSON object from string
	 * @param str JSON string to parse
	 * @param read_flg Read flags (YYJSON_READ_FLAG values, default: 0)
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return Parsed JSON object or nullptr on error
	 * @note Returns error if root is not an object
	 */
	virtual YYJSONValue* ObjectParseString(const char* str, uint32_t read_flg = 0, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Parse a JSON object from file
	 * @param path File path
	 * @param read_flg Read flags (YYJSON_READ_FLAG values, default: 0)
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return Parsed JSON object or nullptr on error
	 * @note Returns error if root is not an object
	 */
	virtual YYJSONValue* ObjectParseFile(const char* path, uint32_t read_flg = 0,
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Get number of key-value pairs in object
	 * @param handle JSON object
	 * @return Number of key-value pairs
	 */
	virtual size_t ObjectGetSize(YYJSONValue* handle) = 0;

	/**
	 * Get key name at specific index
	 * @param handle JSON object
	 * @param index Index of key-value pair
	 * @param out_key Pointer to receive key string
	 * @return true on success, false if index out of bounds
	 */
	virtual bool ObjectGetKey(YYJSONValue* handle, size_t index, const char** out_key) = 0;

	/**
	 * Get value at specific index
	 * @param handle JSON object
	 * @param index Index of key-value pair
	 * @return JSON value or nullptr if index out of bounds
	 */
	virtual YYJSONValue* ObjectGetValueAt(YYJSONValue* handle, size_t index) = 0;

	/**
	 * Get value by key name
	 * @param handle JSON object
	 * @param key Key name
	 * @return JSON value or nullptr if key not found
	 */
	virtual YYJSONValue* ObjectGet(YYJSONValue* handle, const char* key) = 0;

	/**
	 * Get boolean value by key
	 * @param handle JSON object
	 * @param key Key name
	 * @param out_value Pointer to receive boolean value
	 * @return true on success, false if key not found or type mismatch
	 */
	virtual bool ObjectGetBool(YYJSONValue* handle, const char* key, bool* out_value) = 0;

	/**
	 * Get float value by key
	 * @param handle JSON object
	 * @param key Key name
	 * @param out_value Pointer to receive float value
	 * @return true on success, false if key not found or type mismatch
	 */
	virtual bool ObjectGetFloat(YYJSONValue* handle, const char* key, double* out_value) = 0;

	/**
	 * Get integer value by key
	 * @param handle JSON object
	 * @param key Key name
	 * @param out_value Pointer to receive integer value
	 * @return true on success, false if key not found or type mismatch
	 */
	virtual bool ObjectGetInt(YYJSONValue* handle, const char* key, int* out_value) = 0;

	/**
	 * Get 64-bit integer value by key
	 * @param handle JSON object
	 * @param key Key name
	 * @param out_value Pointer to receive 64-bit integer value
	 * @return true on success, false if key not found or type mismatch
	 */
	virtual bool ObjectGetInt64(YYJSONValue* handle, const char* key, int64_t* out_value) = 0;

	/**
	 * Get string value by key
	 * @param handle JSON object
	 * @param key Key name
	 * @param out_str Pointer to receive string pointer
	 * @param out_len Pointer to receive string length
	 * @return true on success, false if key not found or type mismatch
	 */
	virtual bool ObjectGetString(YYJSONValue* handle, const char* key, const char** out_str, size_t* out_len) = 0;

	/**
	 * Check if value at key is null
	 * @param handle JSON object
	 * @param key Key name
	 * @param out_is_null Pointer to receive result
	 * @return true if key exists, false if key not found
	 */
	virtual bool ObjectIsNull(YYJSONValue* handle, const char* key, bool* out_is_null) = 0;

	/**
	 * Check if object has a specific key
	 * @param handle JSON object
	 * @param key Key name (or JSON pointer if use_pointer is true)
	 * @param use_pointer If true, treat key as JSON pointer
	 * @return true if key exists
	 */
	virtual bool ObjectHasKey(YYJSONValue* handle, const char* key, bool use_pointer) = 0;

	/**
	 * Rename a key in the object
	 * @param handle Mutable JSON object
	 * @param old_key Current key name
	 * @param new_key New key name
	 * @param allow_duplicate Allow duplicate key names
	 * @return true on success
	 * @note Only works on mutable objects
	 */
	virtual bool ObjectRenameKey(YYJSONValue* handle, const char* old_key, const char* new_key, bool allow_duplicate) = 0;

	/**
	 * Set value by key (mutable only)
	 * @param handle Mutable JSON object
	 * @param key Key name
	 * @param value JSON value to set
	 * @return true on success
	 */
	virtual bool ObjectSet(YYJSONValue* handle, const char* key, YYJSONValue* value) = 0;

	/**
	 * Set boolean value by key (mutable only)
	 * @param handle Mutable JSON object
	 * @param key Key name
	 * @param value Boolean value
	 * @return true on success
	 */
	virtual bool ObjectSetBool(YYJSONValue* handle, const char* key, bool value) = 0;

	/**
	 * Set float value by key (mutable only)
	 * @param handle Mutable JSON object
	 * @param key Key name
	 * @param value Float value
	 * @return true on success
	 */
	virtual bool ObjectSetFloat(YYJSONValue* handle, const char* key, double value) = 0;

	/**
	 * Set integer value by key (mutable only)
	 * @param handle Mutable JSON object
	 * @param key Key name
	 * @param value Integer value
	 * @return true on success
	 */
	virtual bool ObjectSetInt(YYJSONValue* handle, const char* key, int value) = 0;

	/**
	 * Set 64-bit integer value by key (mutable only)
	 * @param handle Mutable JSON object
	 * @param key Key name
	 * @param value 64-bit integer value
	 * @return true on success
	 */
	virtual bool ObjectSetInt64(YYJSONValue* handle, const char* key, int64_t value) = 0;

	/**
	 * Set null value by key (mutable only)
	 * @param handle Mutable JSON object
	 * @param key Key name
	 * @return true on success
	 */
	virtual bool ObjectSetNull(YYJSONValue* handle, const char* key) = 0;

	/**
	 * Set string value by key (mutable only)
	 * @param handle Mutable JSON object
	 * @param key Key name
	 * @param value String value
	 * @return true on success
	 */
	virtual bool ObjectSetString(YYJSONValue* handle, const char* key, const char* value) = 0;

	/**
	 * Remove key-value pair by key (mutable only)
	 * @param handle Mutable JSON object
	 * @param key Key name
	 * @return true on success
	 */
	virtual bool ObjectRemove(YYJSONValue* handle, const char* key) = 0;

	/**
	 * Remove all key-value pairs (mutable only)
	 * @param handle Mutable JSON object
	 * @return true on success
	 */
	virtual bool ObjectClear(YYJSONValue* handle) = 0;

	/**
	 * Sort object keys
	 * @param handle Mutable JSON object
	 * @param sort_mode Sort order (YYJSON_SORT_ASC, YYJSON_SORT_DESC, or YYJSON_SORT_RANDOM)
	 * @return true on success
	 * @note Only works on mutable objects
	 */
	virtual bool ObjectSort(YYJSONValue* handle, YYJSON_SORT_ORDER sort_mode) = 0;

	/**
	 * Create an empty mutable JSON array
	 * @return New mutable JSON array or nullptr on failure
	 */
	virtual YYJSONValue* ArrayInit() = 0;

	/**
	 * Create a JSON array from string values
	 * @param strings Array of string values
	 * @param count Number of strings
	 * @return New JSON array or nullptr on failure
	 */
	virtual YYJSONValue* ArrayInitWithStrings(const char** strings, size_t count) = 0;

	/**
	 * Parse a JSON array from string
	 * @param str JSON string to parse
	 * @param read_flg Read flags (YYJSON_READ_FLAG values, default: 0)
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return Parsed JSON array or nullptr on error
	 * @note Returns error if root is not an array
	 */
	virtual YYJSONValue* ArrayParseString(const char* str, uint32_t read_flg = 0,
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Parse a JSON array from file
	 * @param path File path
	 * @param read_flg Read flags (YYJSON_READ_FLAG values, default: 0)
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return Parsed JSON array or nullptr on error
	 * @note Returns error if root is not an array
	 */
	virtual YYJSONValue* ArrayParseFile(const char* path, uint32_t read_flg = 0,
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Get number of elements in array
	 * @param handle JSON array
	 * @return Number of elements
	 */
	virtual size_t ArrayGetSize(YYJSONValue* handle) = 0;

	/**
	 * Get element at specific index
	 * @param handle JSON array
	 * @param index Element index
	 * @return JSON value or nullptr if index out of bounds
	 */
	virtual YYJSONValue* ArrayGet(YYJSONValue* handle, size_t index) = 0;

	/**
	 * Get first element in array
	 * @param handle JSON array
	 * @return First JSON value or nullptr if array is empty
	 */
	virtual YYJSONValue* ArrayGetFirst(YYJSONValue* handle) = 0;

	/**
	 * Get last element in array
	 * @param handle JSON array
	 * @return Last JSON value or nullptr if array is empty
	 */
	virtual YYJSONValue* ArrayGetLast(YYJSONValue* handle) = 0;

	/**
	 * Get boolean value at index
	 * @param handle JSON array
	 * @param index Element index
	 * @param out_value Pointer to receive boolean value
	 * @return true on success, false if index out of bounds or type mismatch
	 */
	virtual bool ArrayGetBool(YYJSONValue* handle, size_t index, bool* out_value) = 0;

	/**
	 * Get float value at index
	 * @param handle JSON array
	 * @param index Element index
	 * @param out_value Pointer to receive float value
	 * @return true on success, false if index out of bounds or type mismatch
	 */
	virtual bool ArrayGetFloat(YYJSONValue* handle, size_t index, double* out_value) = 0;

	/**
	 * Get integer value at index
	 * @param handle JSON array
	 * @param index Element index
	 * @param out_value Pointer to receive integer value
	 * @return true on success, false if index out of bounds or type mismatch
	 */
	virtual bool ArrayGetInt(YYJSONValue* handle, size_t index, int* out_value) = 0;

	/**
	 * Get 64-bit integer value at index
	 * @param handle JSON array
	 * @param index Element index
	 * @param out_value Pointer to receive 64-bit integer value
	 * @return true on success, false if index out of bounds or type mismatch
	 */
	virtual bool ArrayGetInt64(YYJSONValue* handle, size_t index, int64_t* out_value) = 0;

	/**
	 * Get string value at index
	 * @param handle JSON array
	 * @param index Element index
	 * @param out_str Pointer to receive string pointer
	 * @param out_len Pointer to receive string length
	 * @return true on success, false if index out of bounds or type mismatch
	 */
	virtual bool ArrayGetString(YYJSONValue* handle, size_t index, const char** out_str, size_t* out_len) = 0;

	/**
	 * Check if element at index is null
	 * @param handle JSON array
	 * @param index Element index
	 * @return true if element is null
	 */
	virtual bool ArrayIsNull(YYJSONValue* handle, size_t index) = 0;

	/**
	 * Replace element at index with JSON value (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value JSON value to set
	 * @return true on success
	 */
	virtual bool ArrayReplace(YYJSONValue* handle, size_t index, YYJSONValue* value) = 0;

	/**
	 * Replace element at index with boolean (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value Boolean value
	 * @return true on success
	 */
	virtual bool ArrayReplaceBool(YYJSONValue* handle, size_t index, bool value) = 0;

	/**
	 * Replace element at index with float (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value Float value
	 * @return true on success
	 */
	virtual bool ArrayReplaceFloat(YYJSONValue* handle, size_t index, double value) = 0;

	/**
	 * Replace element at index with integer (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value Integer value
	 * @return true on success
	 */
	virtual bool ArrayReplaceInt(YYJSONValue* handle, size_t index, int value) = 0;

	/**
	 * Replace element at index with 64-bit integer (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value 64-bit integer value
	 * @return true on success
	 */
	virtual bool ArrayReplaceInt64(YYJSONValue* handle, size_t index, int64_t value) = 0;

	/**
	 * Replace element at index with null (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @return true on success
	 */
	virtual bool ArrayReplaceNull(YYJSONValue* handle, size_t index) = 0;

	/**
	 * Replace element at index with string (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value String value
	 * @return true on success
	 */
	virtual bool ArrayReplaceString(YYJSONValue* handle, size_t index, const char* value) = 0;

	/**
	 * Append JSON value to end of array (mutable only)
	 * @param handle Mutable JSON array
	 * @param value JSON value to append
	 * @return true on success
	 */
	virtual bool ArrayAppend(YYJSONValue* handle, YYJSONValue* value) = 0;

	/**
	 * Append boolean to end of array (mutable only)
	 * @param handle Mutable JSON array
	 * @param value Boolean value
	 * @return true on success
	 */
	virtual bool ArrayAppendBool(YYJSONValue* handle, bool value) = 0;

	/**
	 * Append float to end of array (mutable only)
	 * @param handle Mutable JSON array
	 * @param value Float value
	 * @return true on success
	 */
	virtual bool ArrayAppendFloat(YYJSONValue* handle, double value) = 0;

	/**
	 * Append integer to end of array (mutable only)
	 * @param handle Mutable JSON array
	 * @param value Integer value
	 * @return true on success
	 */
	virtual bool ArrayAppendInt(YYJSONValue* handle, int value) = 0;

	/**
	 * Append 64-bit integer to end of array (mutable only)
	 * @param handle Mutable JSON array
	 * @param value 64-bit integer value
	 * @return true on success
	 */
	virtual bool ArrayAppendInt64(YYJSONValue* handle, int64_t value) = 0;

	/**
	 * Append null to end of array (mutable only)
	 * @param handle Mutable JSON array
	 * @return true on success
	 */
	virtual bool ArrayAppendNull(YYJSONValue* handle) = 0;

	/**
	 * Append string to end of array (mutable only)
	 * @param handle Mutable JSON array
	 * @param value String value
	 * @return true on success
	 */
	virtual bool ArrayAppendString(YYJSONValue* handle, const char* value) = 0;

	/**
	 * Remove element at specific index (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @return true on success
	 */
	virtual bool ArrayRemove(YYJSONValue* handle, size_t index) = 0;

	/**
	 * Remove first element (mutable only)
	 * @param handle Mutable JSON array
	 * @return true on success
	 */
	virtual bool ArrayRemoveFirst(YYJSONValue* handle) = 0;

	/**
	 * Remove last element (mutable only)
	 * @param handle Mutable JSON array
	 * @return true on success
	 */
	virtual bool ArrayRemoveLast(YYJSONValue* handle) = 0;

	/**
	 * Remove range of elements (mutable only)
	 * @param handle JSON array
	 * @param start_index Start index (inclusive)
	 * @param end_index End index (exclusive)
	 * @return true on success
	 */
	virtual bool ArrayRemoveRange(YYJSONValue* handle, size_t start_index, size_t end_index) = 0;

	/**
	 * Remove all elements (mutable only)
	 * @param handle Mutable JSON array
	 * @return true on success
	 */
	virtual bool ArrayClear(YYJSONValue* handle) = 0;

	/**
	 * Find index of boolean value
	 * @param handle JSON array
	 * @param search_value Boolean value to search for
	 * @return Index of first match, or -1 if not found
	 */
	virtual int ArrayIndexOfBool(YYJSONValue* handle, bool search_value) = 0;

	/**
	 * Find index of string value
	 * @param handle JSON array
	 * @param search_value String value to search for
	 * @return Index of first match, or -1 if not found
	 */
	virtual int ArrayIndexOfString(YYJSONValue* handle, const char* search_value) = 0;

	/**
	 * Find index of integer value
	 * @param handle JSON array
	 * @param search_value Integer value to search for
	 * @return Index of first match, or -1 if not found
	 */
	virtual int ArrayIndexOfInt(YYJSONValue* handle, int search_value) = 0;

	/**
	 * Find index of 64-bit integer value
	 * @param handle JSON array
	 * @param search_value 64-bit integer value to search for
	 * @return Index of first match, or -1 if not found
	 */
	virtual int ArrayIndexOfInt64(YYJSONValue* handle, int64_t search_value) = 0;

	/**
	 * Find index of float value
	 * @param handle JSON array
	 * @param search_value Float value to search for
	 * @return Index of first match, or -1 if not found
	 */
	virtual int ArrayIndexOfFloat(YYJSONValue* handle, double search_value) = 0;

	/**
	 * Sort array elements
	 * @param handle Mutable JSON array
	 * @param sort_mode Sort order (YYJSON_SORT_ASC, YYJSON_SORT_DESC, or YYJSON_SORT_RANDOM)
	 * @return true on success
	 * @note Only works on mutable arrays
	 */
	virtual bool ArraySort(YYJSONValue* handle, YYJSON_SORT_ORDER sort_mode) = 0;

	/**
	 * Create JSON value from format string and parameters
	 * @param format Format string (e.g., "{s:i,s:s}", "[i,s,b]")
	 *               Format specifiers:
	 *               - 's': string
	 *               - 'i': integer
	 *               - 'f': float
	 *               - 'b': boolean
	 *               - 'n': null
	 *               - '{': object start, '}': object end
	 *               - '[': array start, ']': array end
	 * @param param_provider Parameter provider implementation
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return New JSON value or nullptr on error
	 * @note Example: format="{s:i,s:s}" with params ["age", 25, "name", "John"] creates {"age":25,"name":"John"}
	 */
	virtual YYJSONValue* Pack(const char* format, IPackParamProvider* param_provider, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Create a JSON boolean value
	 * @param value Boolean value
	 * @return New JSON boolean or nullptr on failure
	 */
	virtual YYJSONValue* CreateBool(bool value) = 0;

	/**
	 * Create a JSON float value
	 * @param value Float value
	 * @return New JSON float or nullptr on failure
	 */
	virtual YYJSONValue* CreateFloat(double value) = 0;

	/**
	 * Create a JSON integer value
	 * @param value Integer value
	 * @return New JSON integer or nullptr on failure
	 */
	virtual YYJSONValue* CreateInt(int value) = 0;

	/**
	 * Create a JSON 64-bit integer value
	 * @param value 64-bit integer value
	 * @return New JSON integer64 or nullptr on failure
	 */
	virtual YYJSONValue* CreateInt64(int64_t value) = 0;

	/**
	 * Create a JSON null value
	 * @return New JSON null or nullptr on failure
	 */
	virtual YYJSONValue* CreateNull() = 0;

	/**
	 * Create a JSON string value
	 * @param value String value
	 * @return New JSON string or nullptr on failure
	 */
	virtual YYJSONValue* CreateString(const char* value) = 0;

	/**
	 * Get boolean value from JSON
	 * @param handle JSON value
	 * @param out_value Pointer to receive boolean value
	 * @return true on success, false on type mismatch
	 */
	virtual bool GetBool(YYJSONValue* handle, bool* out_value) = 0;

	/**
	 * Get float value from JSON
	 * @param handle JSON value
	 * @param out_value Pointer to receive float value
	 * @return true on success, false on type mismatch
	 */
	virtual bool GetFloat(YYJSONValue* handle, double* out_value) = 0;

	/**
	 * Get integer value from JSON
	 * @param handle JSON value
	 * @param out_value Pointer to receive integer value
	 * @return true on success, false on type mismatch
	 */
	virtual bool GetInt(YYJSONValue* handle, int* out_value) = 0;

	/**
	 * Get 64-bit integer value from JSON
	 * @param handle JSON value
	 * @param out_value Pointer to receive 64-bit integer value
	 * @return true on success, false on type mismatch
	 */
	virtual bool GetInt64(YYJSONValue* handle, int64_t* out_value) = 0;

	/**
	 * Get string value from JSON
	 * @param handle JSON value
	 * @param out_str Pointer to receive string pointer
	 * @param out_len Pointer to receive string length
	 * @return true on success, false on type mismatch
	 */
	virtual bool GetString(YYJSONValue* handle, const char** out_str, size_t* out_len) = 0;

	/**
	 * Get value using JSON Pointer
	 * @param handle JSON value
	 * @param path JSON Pointer path (e.g., "/users/0/name")
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return JSON value or nullptr on error
	 */
	virtual YYJSONValue* PtrGet(YYJSONValue* handle, const char* path, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Get boolean value using JSON Pointer
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_value Pointer to receive boolean value
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrGetBool(YYJSONValue* handle, const char* path, bool* out_value, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Get float value using JSON Pointer
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_value Pointer to receive float value
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrGetFloat(YYJSONValue* handle, const char* path, double* out_value, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Get integer value using JSON Pointer
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_value Pointer to receive integer value
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrGetInt(YYJSONValue* handle, const char* path, int* out_value, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Get 64-bit integer value using JSON Pointer
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_value Pointer to receive 64-bit integer value
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrGetInt64(YYJSONValue* handle, const char* path, int64_t* out_value, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Get string value using JSON Pointer
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_str Pointer to receive string pointer
	 * @param out_len Pointer to receive string length
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrGetString(YYJSONValue* handle, const char* path, const char** out_str, 
		size_t* out_len, char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Check if value is null using JSON Pointer
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_is_null Pointer to receive result
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrGetIsNull(YYJSONValue* handle, const char* path, bool* out_is_null, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Get length of container (array/object) using JSON Pointer
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_len Pointer to receive length
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrGetLength(YYJSONValue* handle, const char* path, size_t* out_len, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Set value using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path
	 * @param value JSON value to set
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrSet(YYJSONValue* handle, const char* path, YYJSONValue* value, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Set boolean value using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path
	 * @param value Boolean value
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrSetBool(YYJSONValue* handle, const char* path, bool value, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Set float value using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path
	 * @param value Float value
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrSetFloat(YYJSONValue* handle, const char* path, double value, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Set integer value using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path
	 * @param value Integer value
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrSetInt(YYJSONValue* handle, const char* path, int value, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Set 64-bit integer value using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path
	 * @param value 64-bit integer value
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrSetInt64(YYJSONValue* handle, const char* path, int64_t value, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Set string value using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path
	 * @param value String value
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrSetString(YYJSONValue* handle, const char* path, const char* value, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Set null value using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrSetNull(YYJSONValue* handle, const char* path, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Add value to array using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path to array
	 * @param value JSON value to add
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrAdd(YYJSONValue* handle, const char* path, YYJSONValue* value, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Add boolean to array using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path to array
	 * @param value Boolean value
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrAddBool(YYJSONValue* handle, const char* path, bool value, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Add float to array using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path to array
	 * @param value Float value
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrAddFloat(YYJSONValue* handle, const char* path, double value, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Add integer to array using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path to array
	 * @param value Integer value
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrAddInt(YYJSONValue* handle, const char* path, int value, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Add 64-bit integer to array using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path to array
	 * @param value 64-bit integer value
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrAddInt64(YYJSONValue* handle, const char* path, int64_t value, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Add string to array using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path to array
	 * @param value String value
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrAddString(YYJSONValue* handle, const char* path, const char* value, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Add null to array using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path to array
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrAddNull(YYJSONValue* handle, const char* path, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Remove value using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrRemove(YYJSONValue* handle, const char* path, 
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Try to get value using JSON Pointer (no error on failure)
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @return JSON value or nullptr if not found
	 */
	virtual YYJSONValue* PtrTryGet(YYJSONValue* handle, const char* path) = 0;

	/**
	 * Try to get boolean value using JSON Pointer (returns false on failure)
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_value Pointer to receive boolean value
	 * @return true on success, false if not found or type mismatch
	 */
	virtual bool PtrTryGetBool(YYJSONValue* handle, const char* path, bool* out_value) = 0;

	/**
	 * Try to get float value using JSON Pointer (returns false on failure)
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_value Pointer to receive float value
	 * @return true on success, false if not found or type mismatch
	 */
	virtual bool PtrTryGetFloat(YYJSONValue* handle, const char* path, double* out_value) = 0;

	/**
	 * Try to get integer value using JSON Pointer (returns false on failure)
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_value Pointer to receive integer value
	 * @return true on success, false if not found or type mismatch
	 */
	virtual bool PtrTryGetInt(YYJSONValue* handle, const char* path, int* out_value) = 0;

	/**
	 * Try to get 64-bit integer value using JSON Pointer (returns false on failure)
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_value Pointer to receive 64-bit integer value
	 * @return true on success, false if not found or type mismatch
	 */
	virtual bool PtrTryGetInt64(YYJSONValue* handle, const char* path, int64_t* out_value) = 0;

	/**
	 * Try to get string value using JSON Pointer (returns false on failure)
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_str Pointer to receive string pointer
	 * @param out_len Pointer to receive string length
	 * @return true on success, false if not found or type mismatch
	 */
	virtual bool PtrTryGetString(YYJSONValue* handle, const char* path, const char** out_str, size_t* out_len) = 0;

	// Note: Iterators are stateful and stored in the YYJSONValue object
	// Call these functions in a loop until they return false

	/**
	 * Get next key-value pair from object iterator
	 * @param handle JSON object
	 * @param out_key Pointer to receive key string
	 * @param out_key_len Pointer to receive key length (can be nullptr)
	 * @param out_value Pointer to receive value (creates new YYJSONValue)
	 * @return true if iteration continues, false if iteration complete
	 * @note Iterator state is maintained in handle. Returns false when iteration completes.
	 */
	virtual bool ObjectForeachNext(YYJSONValue* handle, const char** out_key, 
	                                size_t* out_key_len, YYJSONValue** out_value) = 0;

	/**
	 * Get next index-value pair from array iterator
	 * @param handle JSON array
	 * @param out_index Pointer to receive current index
	 * @param out_value Pointer to receive value (creates new YYJSONValue)
	 * @return true if iteration continues, false if iteration complete
	 * @note Iterator state is maintained in handle. Returns false when iteration completes.
	 */
	virtual bool ArrayForeachNext(YYJSONValue* handle, size_t* out_index, 
	                               YYJSONValue** out_value) = 0;

	/**
	 * Get next key from object iterator (key only, no value)
	 * @param handle JSON object
	 * @param out_key Pointer to receive key string
	 * @param out_key_len Pointer to receive key length (can be nullptr)
	 * @return true if iteration continues, false if iteration complete
	 * @note Iterator state is maintained in handle. Returns false when iteration completes.
	 */
	virtual bool ObjectForeachKeyNext(YYJSONValue* handle, const char** out_key, 
	                                   size_t* out_key_len) = 0;

	/**
	 * Get next index from array iterator (index only, no value)
	 * @param handle JSON array
	 * @param out_index Pointer to receive current index
	 * @return true if iteration continues, false if iteration complete
	 * @note Iterator state is maintained in handle. Returns false when iteration completes.
	 */
	virtual bool ArrayForeachIndexNext(YYJSONValue* handle, size_t* out_index) = 0;

	/**
	 * Release a YYJSONValue object
	 * External extensions should use this instead of deleting directly
	 * @param value The YYJSONValue to release
	 */
	virtual void Release(YYJSONValue* value) = 0;

	/**
	 * Get the HandleType_t for YYJSON handles
	 * External extensions MUST use this method to obtain the handle type
	 * @return The HandleType_t for YYJSON handles
	 */
	virtual HandleType_t GetHandleType() = 0;

	/**
	 * Read YYJSONValue from a SourceMod handle
	 * @param pContext Plugin context
	 * @param handle Handle to read from
	 * @return YYJSONValue pointer, or nullptr on error (error will be reported to context)
	 */
	virtual YYJSONValue* GetFromHandle(IPluginContext* pContext, Handle_t handle) = 0;
};

#endif // _INCLUDE_SM_YYJSON_IYYJSONMANAGER_H_