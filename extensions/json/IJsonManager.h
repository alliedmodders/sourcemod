#ifndef _INCLUDE_SM_JSON_IJSONMANAGER_H_
#define _INCLUDE_SM_JSON_IJSONMANAGER_H_

#include <IHandleSys.h>
#include <variant>

using SourceMod::Handle_t;
using SourceMod::HandleType_t;
using SourceMod::SMInterface;
using SourcePawn::IPluginContext;

// Forward declaration
class JsonValue;
class JsonArrIter;
class JsonObjIter;

#define SMINTERFACE_JSONMANAGER_NAME "IJsonManager"
#define SMINTERFACE_JSONMANAGER_VERSION 1
#define JSON_PACK_ERROR_SIZE 256
#define JSON_ERROR_BUFFER_SIZE 256
#define JSON_INT64_BUFFER_SIZE 32

/**
 * @brief JSON sorting order
 */
enum JSON_SORT_ORDER
{
	JSON_SORT_ASC = 0,      // Ascending order
	JSON_SORT_DESC = 1,     // Descending order
	JSON_SORT_RANDOM = 2    // Random order
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
 * @brief JSON Manager Interface
 *
 * This interface provides complete JSON manipulation capabilities.
 * It's designed to be consumed by other SourceMod C++ extensions
 * without requiring them to link against yyjson library.
 *
 * @usage
 * IJsonManager* g_pJsonManager = nullptr;
 *
 * bool YourExtension::SDK_OnAllLoaded()
 * {
 *     SM_GET_LATE_IFACE(JSONMANAGER, g_pJsonManager);
 * }
 */
class IJsonManager : public SMInterface
{
public:
	virtual ~IJsonManager() = default;

	virtual const char *GetInterfaceName() override {
		return SMINTERFACE_JSONMANAGER_NAME;
	}

	virtual unsigned int GetInterfaceVersion() override {
		return SMINTERFACE_JSONMANAGER_VERSION;
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
	virtual JsonValue* ParseJSON(const char* json_str, bool is_file, bool is_mutable = false,
		uint32_t read_flg = 0, char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Write JSON to string
	 * @param handle JSON value
	 * @param buffer Output buffer
	 * @param buffer_size Buffer size
	 * @param write_flg Write flags (YYJSON_WRITE_FLAG values, default: 0)
	 * @param out_size Pointer to receive actual size written (including null terminator) optional
	 * @return true on success, false if buffer is too small or on error
	 *
	 * @note The out_size parameter returns the size including null terminator
	 * @note Use GetSerializedSize() with the same write_flg to determine buffer size
	 */
	virtual bool WriteToString(JsonValue* handle, char* buffer, size_t buffer_size,
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
	virtual bool WriteToFile(JsonValue* handle, const char* path, uint32_t write_flg = 0,
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Compare two JSON values for equality
	 * @param handle1 First JSON value to compare
	 * @param handle2 Second JSON value to compare
	 * @return true if values are equal, false otherwise
	 * @note Compares structure and content recursively
	 */
	virtual bool Equals(JsonValue* handle1, JsonValue* handle2) = 0;

	/**
	 * Check if JSON value equals a string
	 * @param handle JSON value to compare
	 * @param str String to compare with
	 * @return true if value is a string and equals the given string, false otherwise
	 * @note Returns false if handle is null, str is null, or value is not a string
	 */
	virtual bool EqualsStr(JsonValue* handle, const char* str) = 0;

	/**
	 * Deep copy a JSON value into a target document
	 * @param targetDoc Target document that will own the copied value
	 * @param sourceValue Source value to copy
	 * @return New JSON value (deep copy) or nullptr on failure
	 * @note The returned value is owned by targetDoc's document context
	 */
	virtual JsonValue* DeepCopy(JsonValue* targetDoc, JsonValue* sourceValue) = 0;

	/**
	 * Get human-readable type description string
	 * @param handle JSON value
	 * @return Type description string (e.g., "object", "array", "string", "number", "true", "false", "unknown")
	 */
	virtual const char* GetTypeDesc(JsonValue* handle) = 0;

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
	 *   size_t size = g_pJsonManager->GetSerializedSize(handle, flags);
	 *   char* buffer = new char[size];
	 *   g_pJsonManager->WriteToString(handle, buffer, size, flags);  // Use same flags
	 */
	virtual size_t GetSerializedSize(JsonValue* handle, uint32_t write_flg = 0) = 0;

	/**
	 * Convert immutable document to mutable
	 * @param handle Immutable JSON value
	 * @return New mutable JSON value or nullptr if already mutable or on error
	 * @note Creates a deep copy as a mutable document
	 */
	virtual JsonValue* ToMutable(JsonValue* handle) = 0;

	/**
	 * Convert mutable document to immutable
	 * @param handle Mutable JSON value
	 * @return New immutable JSON value or nullptr if already immutable or on error
	 * @note Creates a deep copy as an immutable document
	 */
	virtual JsonValue* ToImmutable(JsonValue* handle) = 0;

	/**
	 * Get JSON type
	 * @param handle JSON value
	 * @return YYJSON_TYPE value
	 */
	virtual uint8_t GetType(JsonValue* handle) = 0;

	/**
	 * Get JSON subtype
	 * @param handle JSON value
	 * @return YYJSON_SUBTYPE value
	 */
	virtual uint8_t GetSubtype(JsonValue* handle) = 0;

	/**
	 * Check if value is an array
	 * @param handle JSON value
	 * @return true if value is an array
	 */
	virtual bool IsArray(JsonValue* handle) = 0;

	/**
	 * Check if value is an object
	 * @param handle JSON value
	 * @return true if value is an object
	 */
	virtual bool IsObject(JsonValue* handle) = 0;

	/**
	 * Check if value is an integer (signed or unsigned)
	 * @param handle JSON value
	 * @return true if value is an integer
	 */
	virtual bool IsInt(JsonValue* handle) = 0;

	/**
	 * Check if value is an unsigned integer
	 * @param handle JSON value
	 * @return true if value is an unsigned integer
	 */
	virtual bool IsUint(JsonValue* handle) = 0;

	/**
	 * Check if value is a signed integer
	 * @param handle JSON value
	 * @return true if value is a signed integer
	 */
	virtual bool IsSint(JsonValue* handle) = 0;

	/**
	 * Check if value is a number (integer or real)
	 * @param handle JSON value
	 * @return true if value is a number
	 */
	virtual bool IsNum(JsonValue* handle) = 0;

	/**
	 * Check if value is a boolean (true or false)
	 * @param handle JSON value
	 * @return true if value is a boolean
	 */
	virtual bool IsBool(JsonValue* handle) = 0;

	/**
	 * Check if value is boolean true
	 * @param handle JSON value
	 * @return true if value is boolean true
	 */
	virtual bool IsTrue(JsonValue* handle) = 0;

	/**
	 * Check if value is boolean false
	 * @param handle JSON value
	 * @return true if value is boolean false
	 */
	virtual bool IsFalse(JsonValue* handle) = 0;

	/**
	 * Check if value is a floating-point number
	 * @param handle JSON value
	 * @return true if value is a floating-point number
	 */
	virtual bool IsFloat(JsonValue* handle) = 0;

	/**
	 * Check if value is a string
	 * @param handle JSON value
	 * @return true if value is a string
	 */
	virtual bool IsStr(JsonValue* handle) = 0;

	/**
	 * Check if value is null
	 * @param handle JSON value
	 * @return true if value is null
	 */
	virtual bool IsNull(JsonValue* handle) = 0;

	/**
	 * Check if value is a container (object or array)
	 * @param handle JSON value
	 * @return true if value is a container
	 */
	virtual bool IsCtn(JsonValue* handle) = 0;

	/**
	 * Check if document is mutable
	 * @param handle JSON value
	 * @return true if document is mutable
	 */
	virtual bool IsMutable(JsonValue* handle) = 0;

	/**
	 * Check if document is immutable
	 * @param handle JSON value
	 * @return true if document is immutable
	 */
	virtual bool IsImmutable(JsonValue* handle) = 0;

	/**
	 * Get the number of bytes read when parsing this document
	 * @param handle JSON value
	 * @return Number of bytes read during parsing (including null terminator) 0 if not from parsing
	 *
	 * @note This value only applies to documents created from parsing
	 * @note Manually created documents (ObjectInit, CreateBool, etc.) will return 0
	 * @note The returned size includes the null terminator
	 */
	virtual size_t GetReadSize(JsonValue* handle) = 0;

	/**
	 * Create an empty mutable JSON object
	 * @return New mutable JSON object or nullptr on failure
	 */
	virtual JsonValue* ObjectInit() = 0;

	/**
	 * Create a JSON object from key-value string pairs
	 * @param pairs Array of strings [key1, val1, key2, val2, ...]
	 * @param count Number of key-value pairs
	 * @return New JSON object or nullptr on failure
	 */
	virtual JsonValue* ObjectInitWithStrings(const char** pairs, size_t count) = 0;

	/**
	 * Parse a JSON object from string
	 * @param str JSON string to parse
	 * @param read_flg Read flags (YYJSON_READ_FLAG values, default: 0)
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return Parsed JSON object or nullptr on error
	 * @note Returns error if root is not an object
	 */
	virtual JsonValue* ObjectParseString(const char* str, uint32_t read_flg = 0,
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
	virtual JsonValue* ObjectParseFile(const char* path, uint32_t read_flg = 0,
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Get number of key-value pairs in object
	 * @param handle JSON object
	 * @return Number of key-value pairs
	 */
	virtual size_t ObjectGetSize(JsonValue* handle) = 0;

	/**
	 * Get key name at specific index
	 * @param handle JSON object
	 * @param index Index of key-value pair
	 * @param out_key Pointer to receive key string
	 * @return true on success, false if index out of bounds
	 */
	virtual bool ObjectGetKey(JsonValue* handle, size_t index, const char** out_key) = 0;

	/**
	 * Get value at specific index
	 * @param handle JSON object
	 * @param index Index of key-value pair
	 * @return JSON value or nullptr if index out of bounds
	 */
	virtual JsonValue* ObjectGetValueAt(JsonValue* handle, size_t index) = 0;

	/**
	 * Get value by key name
	 * @param handle JSON object
	 * @param key Key name
	 * @return JSON value or nullptr if key not found
	 */
	virtual JsonValue* ObjectGet(JsonValue* handle, const char* key) = 0;

	/**
	 * Get boolean value by key
	 * @param handle JSON object
	 * @param key Key name
	 * @param out_value Pointer to receive boolean value
	 * @return true on success, false if key not found or type mismatch
	 */
	virtual bool ObjectGetBool(JsonValue* handle, const char* key, bool* out_value) = 0;

	/**
	 * Get float value by key
	 * @param handle JSON object
	 * @param key Key name
	 * @param out_value Pointer to receive float value
	 * @return true on success, false if key not found or type mismatch
	 */
	virtual bool ObjectGetFloat(JsonValue* handle, const char* key, double* out_value) = 0;

	/**
	 * Get integer value by key
	 * @param handle JSON object
	 * @param key Key name
	 * @param out_value Pointer to receive integer value
	 * @return true on success, false if key not found or type mismatch
	 */
	virtual bool ObjectGetInt(JsonValue* handle, const char* key, int* out_value) = 0;

	/**
	 * Get 64-bit integer value by key (auto-detects signed/unsigned)
	 * @param handle JSON object
	 * @param key Key name
	 * @param out_value Pointer to receive 64-bit integer value (std::variant<int64_t, uint64_t>)
	 * @return true on success, false if key not found or type mismatch
	 */
	virtual bool ObjectGetInt64(JsonValue* handle, const char* key, std::variant<int64_t, uint64_t>* out_value) = 0;

	/**
	 * Get string value by key
	 * @param handle JSON object
	 * @param key Key name
	 * @param out_str Pointer to receive string pointer
	 * @param out_len Pointer to receive string length
	 * @return true on success, false if key not found or type mismatch
	 */
	virtual bool ObjectGetString(JsonValue* handle, const char* key, const char** out_str, size_t* out_len) = 0;

	/**
	 * Check if value at key is null
	 * @param handle JSON object
	 * @param key Key name
	 * @param out_is_null Pointer to receive result
	 * @return true if key exists, false if key not found
	 */
	virtual bool ObjectIsNull(JsonValue* handle, const char* key, bool* out_is_null) = 0;

	/**
	 * Check if object has a specific key
	 * @param handle JSON object
	 * @param key Key name (or JSON pointer if use_pointer is true)
	 * @param use_pointer If true, treat key as JSON pointer
	 * @return true if key exists
	 */
	virtual bool ObjectHasKey(JsonValue* handle, const char* key, bool use_pointer) = 0;

	/**
	 * Rename a key in the object
	 * @param handle Mutable JSON object
	 * @param old_key Current key name
	 * @param new_key New key name
	 * @param allow_duplicate Allow duplicate key names
	 * @return true on success
	 * @note Only works on mutable objects
	 */
	virtual bool ObjectRenameKey(JsonValue* handle, const char* old_key, const char* new_key, bool allow_duplicate) = 0;

	/**
	 * Set value by key (mutable only)
	 * @param handle Mutable JSON object
	 * @param key Key name
	 * @param value JSON value to set
	 * @return true on success
	 */
	virtual bool ObjectSet(JsonValue* handle, const char* key, JsonValue* value) = 0;

	/**
	 * Set boolean value by key (mutable only)
	 * @param handle Mutable JSON object
	 * @param key Key name
	 * @param value Boolean value
	 * @return true on success
	 */
	virtual bool ObjectSetBool(JsonValue* handle, const char* key, bool value) = 0;

	/**
	 * Set float value by key (mutable only)
	 * @param handle Mutable JSON object
	 * @param key Key name
	 * @param value Float value
	 * @return true on success
	 */
	virtual bool ObjectSetFloat(JsonValue* handle, const char* key, double value) = 0;

	/**
	 * Set integer value by key (mutable only)
	 * @param handle Mutable JSON object
	 * @param key Key name
	 * @param value Integer value
	 * @return true on success
	 */
	virtual bool ObjectSetInt(JsonValue* handle, const char* key, int value) = 0;

	/**
	 * Set 64-bit integer value by key (mutable only, auto-detects signed/unsigned)
	 * @param handle Mutable JSON object
	 * @param key Key name
	 * @param value 64-bit integer value (std::variant<int64_t, uint64_t>)
	 * @return true on success
	 */
	virtual bool ObjectSetInt64(JsonValue* handle, const char* key, std::variant<int64_t, uint64_t> value) = 0;

	/**
	 * Set null value by key (mutable only)
	 * @param handle Mutable JSON object
	 * @param key Key name
	 * @return true on success
	 */
	virtual bool ObjectSetNull(JsonValue* handle, const char* key) = 0;

	/**
	 * Set string value by key (mutable only)
	 * @param handle Mutable JSON object
	 * @param key Key name
	 * @param value String value
	 * @return true on success
	 */
	virtual bool ObjectSetString(JsonValue* handle, const char* key, const char* value) = 0;

	/**
	 * Remove key-value pair by key (mutable only)
	 * @param handle Mutable JSON object
	 * @param key Key name
	 * @return true on success
	 */
	virtual bool ObjectRemove(JsonValue* handle, const char* key) = 0;

	/**
	 * Remove all key-value pairs (mutable only)
	 * @param handle Mutable JSON object
	 * @return true on success
	 */
	virtual bool ObjectClear(JsonValue* handle) = 0;

	/**
	 * Sort object keys
	 * @param handle Mutable JSON object
	 * @param sort_mode Sort order (see JSON_SORT_ORDER enum)
	 * @return true on success
	 * @note Only works on mutable objects
	 */
	virtual bool ObjectSort(JsonValue* handle, JSON_SORT_ORDER sort_mode) = 0;

	/**
	 * Create an empty mutable JSON array
	 * @return New mutable JSON array or nullptr on failure
	 */
	virtual JsonValue* ArrayInit() = 0;

	/**
	 * Create a JSON array from string values
	 * @param strings Array of string values
	 * @param count Number of strings
	 * @return New JSON array or nullptr on failure
	 */
	virtual JsonValue* ArrayInitWithStrings(const char** strings, size_t count) = 0;

	/**
	 * Create a JSON array from 32-bit integer values
	 * @param values Array of int32_t values
	 * @param count Number of values
	 * @return New JSON array or nullptr on failure
	 */
	virtual JsonValue* ArrayInitWithInt32(const int32_t* values, size_t count) = 0;

	/**
	 * Create a JSON array from 64-bit integer string values (auto-detects signed/unsigned)
	 * @param values Array of int64 string values (can be signed or unsigned)
	 * @param count Number of values
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return New JSON array or nullptr on failure
	 * @note Each string value is parsed and auto-detected as signed or unsigned
	 */
	virtual JsonValue* ArrayInitWithInt64(const char** values, size_t count,
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Create a JSON array from boolean values
	 * @param values Array of boolean values
	 * @param count Number of values
	 * @return New JSON array or nullptr on failure
	 */
	virtual JsonValue* ArrayInitWithBool(const bool* values, size_t count) = 0;

	/**
	 * Create a JSON array from float values
	 * @param values Array of float values
	 * @param count Number of values
	 * @return New JSON array or nullptr on failure
	 */
	virtual JsonValue* ArrayInitWithFloat(const double* values, size_t count) = 0;

	/**
	 * Parse a JSON array from string
	 * @param str JSON string to parse
	 * @param read_flg Read flags (YYJSON_READ_FLAG values, default: 0)
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return Parsed JSON array or nullptr on error
	 * @note Returns error if root is not an array
	 */
	virtual JsonValue* ArrayParseString(const char* str, uint32_t read_flg = 0,
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
	virtual JsonValue* ArrayParseFile(const char* path, uint32_t read_flg = 0,
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Get number of elements in array
	 * @param handle JSON array
	 * @return Number of elements
	 */
	virtual size_t ArrayGetSize(JsonValue* handle) = 0;

	/**
	 * Get element at specific index
	 * @param handle JSON array
	 * @param index Element index
	 * @return JSON value or nullptr if index out of bounds
	 */
	virtual JsonValue* ArrayGet(JsonValue* handle, size_t index) = 0;

	/**
	 * Get first element in array
	 * @param handle JSON array
	 * @return First JSON value or nullptr if array is empty
	 */
	virtual JsonValue* ArrayGetFirst(JsonValue* handle) = 0;

	/**
	 * Get last element in array
	 * @param handle JSON array
	 * @return Last JSON value or nullptr if array is empty
	 */
	virtual JsonValue* ArrayGetLast(JsonValue* handle) = 0;

	/**
	 * Get boolean value at index
	 * @param handle JSON array
	 * @param index Element index
	 * @param out_value Pointer to receive boolean value
	 * @return true on success, false if index out of bounds or type mismatch
	 */
	virtual bool ArrayGetBool(JsonValue* handle, size_t index, bool* out_value) = 0;

	/**
	 * Get float value at index
	 * @param handle JSON array
	 * @param index Element index
	 * @param out_value Pointer to receive float value
	 * @return true on success, false if index out of bounds or type mismatch
	 */
	virtual bool ArrayGetFloat(JsonValue* handle, size_t index, double* out_value) = 0;

	/**
	 * Get integer value at index
	 * @param handle JSON array
	 * @param index Element index
	 * @param out_value Pointer to receive integer value
	 * @return true on success, false if index out of bounds or type mismatch
	 */
	virtual bool ArrayGetInt(JsonValue* handle, size_t index, int* out_value) = 0;

	/**
	 * Get 64-bit integer value at index (auto-detects signed/unsigned)
	 * @param handle JSON array
	 * @param index Element index
	 * @param out_value Pointer to receive 64-bit integer value (std::variant<int64_t, uint64_t>)
	 * @return true on success, false if index out of bounds or type mismatch
	 */
	virtual bool ArrayGetInt64(JsonValue* handle, size_t index, std::variant<int64_t, uint64_t>* out_value) = 0;

	/**
	 * Get string value at index
	 * @param handle JSON array
	 * @param index Element index
	 * @param out_str Pointer to receive string pointer
	 * @param out_len Pointer to receive string length
	 * @return true on success, false if index out of bounds or type mismatch
	 */
	virtual bool ArrayGetString(JsonValue* handle, size_t index, const char** out_str, size_t* out_len) = 0;

	/**
	 * Check if element at index is null
	 * @param handle JSON array
	 * @param index Element index
	 * @return true if element is null
	 */
	virtual bool ArrayIsNull(JsonValue* handle, size_t index) = 0;

	/**
	 * Replace element at index with JSON value (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value JSON value to set
	 * @return true on success
	 */
	virtual bool ArrayReplace(JsonValue* handle, size_t index, JsonValue* value) = 0;

	/**
	 * Replace element at index with boolean (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value Boolean value
	 * @return true on success
	 */
	virtual bool ArrayReplaceBool(JsonValue* handle, size_t index, bool value) = 0;

	/**
	 * Replace element at index with float (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value Float value
	 * @return true on success
	 */
	virtual bool ArrayReplaceFloat(JsonValue* handle, size_t index, double value) = 0;

	/**
	 * Replace element at index with integer (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value Integer value
	 * @return true on success
	 */
	virtual bool ArrayReplaceInt(JsonValue* handle, size_t index, int value) = 0;

	/**
	 * Replace element at index with 64-bit integer (mutable only, auto-detects signed/unsigned)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value 64-bit integer value (std::variant<int64_t, uint64_t>)
	 * @return true on success
	 */
	virtual bool ArrayReplaceInt64(JsonValue* handle, size_t index, std::variant<int64_t, uint64_t> value) = 0;

	/**
	 * Replace element at index with null (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @return true on success
	 */
	virtual bool ArrayReplaceNull(JsonValue* handle, size_t index) = 0;

	/**
	 * Replace element at index with string (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value String value
	 * @return true on success
	 */
	virtual bool ArrayReplaceString(JsonValue* handle, size_t index, const char* value) = 0;

	/**
	 * Append JSON value to end of array (mutable only)
	 * @param handle Mutable JSON array
	 * @param value JSON value to append
	 * @return true on success
	 */
	virtual bool ArrayAppend(JsonValue* handle, JsonValue* value) = 0;

	/**
	 * Append boolean to end of array (mutable only)
	 * @param handle Mutable JSON array
	 * @param value Boolean value
	 * @return true on success
	 */
	virtual bool ArrayAppendBool(JsonValue* handle, bool value) = 0;

	/**
	 * Append float to end of array (mutable only)
	 * @param handle Mutable JSON array
	 * @param value Float value
	 * @return true on success
	 */
	virtual bool ArrayAppendFloat(JsonValue* handle, double value) = 0;

	/**
	 * Append integer to end of array (mutable only)
	 * @param handle Mutable JSON array
	 * @param value Integer value
	 * @return true on success
	 */
	virtual bool ArrayAppendInt(JsonValue* handle, int value) = 0;

	/**
	 * Append 64-bit integer to end of array (mutable only, auto-detects signed/unsigned)
	 * @param handle Mutable JSON array
	 * @param value 64-bit integer value (std::variant<int64_t, uint64_t>)
	 * @return true on success
	 */
	virtual bool ArrayAppendInt64(JsonValue* handle, std::variant<int64_t, uint64_t> value) = 0;

	/**
	 * Append null to end of array (mutable only)
	 * @param handle Mutable JSON array
	 * @return true on success
	 */
	virtual bool ArrayAppendNull(JsonValue* handle) = 0;

	/**
	 * Append string to end of array (mutable only)
	 * @param handle Mutable JSON array
	 * @param value String value
	 * @return true on success
	 */
	virtual bool ArrayAppendString(JsonValue* handle, const char* value) = 0;

	/**
	 * Insert JSON value at specific index (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index (0 to size, size means append)
	 * @param value JSON value to insert
	 * @return true on success
	 */
	virtual bool ArrayInsert(JsonValue* handle, size_t index, JsonValue* value) = 0;

	/**
	 * Insert boolean at specific index (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value Boolean value
	 * @return true on success
	 */
	virtual bool ArrayInsertBool(JsonValue* handle, size_t index, bool value) = 0;

	/**
	 * Insert integer at specific index (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value Integer value
	 * @return true on success
	 */
	virtual bool ArrayInsertInt(JsonValue* handle, size_t index, int value) = 0;

	/**
	 * Insert 64-bit integer at specific index (mutable only, auto-detects signed/unsigned)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value 64-bit integer value (std::variant<int64_t, uint64_t>)
	 * @return true on success
	 */
	virtual bool ArrayInsertInt64(JsonValue* handle, size_t index, std::variant<int64_t, uint64_t> value) = 0;

	/**
	 * Insert float at specific index (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value Float value
	 * @return true on success
	 */
	virtual bool ArrayInsertFloat(JsonValue* handle, size_t index, double value) = 0;

	/**
	 * Insert string at specific index (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @param value String value
	 * @return true on success
	 */
	virtual bool ArrayInsertString(JsonValue* handle, size_t index, const char* value) = 0;

	/**
	 * Insert null at specific index (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @return true on success
	 */
	virtual bool ArrayInsertNull(JsonValue* handle, size_t index) = 0;

	/**
	 * Prepend JSON value to beginning of array (mutable only)
	 * @param handle Mutable JSON array
	 * @param value JSON value to prepend
	 * @return true on success
	 */
	virtual bool ArrayPrepend(JsonValue* handle, JsonValue* value) = 0;

	/**
	 * Prepend boolean to beginning of array (mutable only)
	 * @param handle Mutable JSON array
	 * @param value Boolean value
	 * @return true on success
	 */
	virtual bool ArrayPrependBool(JsonValue* handle, bool value) = 0;

	/**
	 * Prepend integer to beginning of array (mutable only)
	 * @param handle Mutable JSON array
	 * @param value Integer value
	 * @return true on success
	 */
	virtual bool ArrayPrependInt(JsonValue* handle, int value) = 0;

	/**
	 * Prepend 64-bit integer to beginning of array (mutable only, auto-detects signed/unsigned)
	 * @param handle Mutable JSON array
	 * @param value 64-bit integer value (std::variant<int64_t, uint64_t>)
	 * @return true on success
	 */
	virtual bool ArrayPrependInt64(JsonValue* handle, std::variant<int64_t, uint64_t> value) = 0;

	/**
	 * Prepend float to beginning of array (mutable only)
	 * @param handle Mutable JSON array
	 * @param value Float value
	 * @return true on success
	 */
	virtual bool ArrayPrependFloat(JsonValue* handle, double value) = 0;

	/**
	 * Prepend string to beginning of array (mutable only)
	 * @param handle Mutable JSON array
	 * @param value String value
	 * @return true on success
	 */
	virtual bool ArrayPrependString(JsonValue* handle, const char* value) = 0;

	/**
	 * Prepend null to beginning of array (mutable only)
	 * @param handle Mutable JSON array
	 * @return true on success
	 */
	virtual bool ArrayPrependNull(JsonValue* handle) = 0;

	/**
	 * Remove element at specific index (mutable only)
	 * @param handle Mutable JSON array
	 * @param index Element index
	 * @return true on success
	 */
	virtual bool ArrayRemove(JsonValue* handle, size_t index) = 0;

	/**
	 * Remove first element (mutable only)
	 * @param handle Mutable JSON array
	 * @return true on success
	 */
	virtual bool ArrayRemoveFirst(JsonValue* handle) = 0;

	/**
	 * Remove last element (mutable only)
	 * @param handle Mutable JSON array
	 * @return true on success
	 */
	virtual bool ArrayRemoveLast(JsonValue* handle) = 0;

	/**
	 * Remove range of elements (mutable only)
	 * @param handle JSON array
	 * @param start_index Start index (inclusive)
	 * @param end_index End index (exclusive)
	 * @return true on success
	 */
	virtual bool ArrayRemoveRange(JsonValue* handle, size_t start_index, size_t end_index) = 0;

	/**
	 * Remove all elements (mutable only)
	 * @param handle Mutable JSON array
	 * @return true on success
	 */
	virtual bool ArrayClear(JsonValue* handle) = 0;

	/**
	 * Find index of boolean value
	 * @param handle JSON array
	 * @param search_value Boolean value to search for
	 * @return Index of first match, or -1 if not found
	 */
	virtual int ArrayIndexOfBool(JsonValue* handle, bool search_value) = 0;

	/**
	 * Find index of string value
	 * @param handle JSON array
	 * @param search_value String value to search for
	 * @return Index of first match, or -1 if not found
	 */
	virtual int ArrayIndexOfString(JsonValue* handle, const char* search_value) = 0;

	/**
	 * Find index of integer value
	 * @param handle JSON array
	 * @param search_value Integer value to search for
	 * @return Index of first match, or -1 if not found
	 */
	virtual int ArrayIndexOfInt(JsonValue* handle, int search_value) = 0;

	/**
	 * Find index of 64-bit integer value
	 * @param handle JSON array
	 * @param search_value 64-bit integer value to search for
	 * @return Index of first match, or -1 if not found
	 */
	virtual int ArrayIndexOfInt64(JsonValue* handle, int64_t search_value) = 0;

	/**
	 * Find index of 64-bit unsigned integer value
	 * @param handle JSON array
	 * @param search_value 64-bit unsigned integer value to search for
	 * @return Index of first match, or -1 if not found
	 */
	virtual int ArrayIndexOfUint64(JsonValue* handle, uint64_t search_value) = 0;

	/**
	 * Find index of float value
	 * @param handle JSON array
	 * @param search_value Float value to search for
	 * @return Index of first match, or -1 if not found
	 */
	virtual int ArrayIndexOfFloat(JsonValue* handle, double search_value) = 0;

	/**
	 * Sort array elements
	 * @param handle Mutable JSON array
	 * @param sort_mode Sort order (see JSON_SORT_ORDER enum)
	 * @return true on success
	 * @note Only works on mutable arrays
	 */
	virtual bool ArraySort(JsonValue* handle, JSON_SORT_ORDER sort_mode) = 0;

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
	virtual JsonValue* Pack(const char* format, IPackParamProvider* param_provider,
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Create a JSON boolean value
	 * @param value Boolean value
	 * @return New JSON boolean or nullptr on failure
	 */
	virtual JsonValue* CreateBool(bool value) = 0;

	/**
	 * Create a JSON float value
	 * @param value Float value
	 * @return New JSON float or nullptr on failure
	 */
	virtual JsonValue* CreateFloat(double value) = 0;

	/**
	 * Create a JSON integer value
	 * @param value Integer value
	 * @return New JSON integer or nullptr on failure
	 */
	virtual JsonValue* CreateInt(int value) = 0;

	/**
	 * Create a JSON 64-bit integer value (auto-detects signed/unsigned)
	 * @param value 64-bit integer value (std::variant<int64_t, uint64_t>)
	 * @return New JSON integer64 or nullptr on failure
	 */
	virtual JsonValue* CreateInt64(std::variant<int64_t, uint64_t> value) = 0;

	/**
	 * Create a JSON null value
	 * @return New JSON null or nullptr on failure
	 */
	virtual JsonValue* CreateNull() = 0;

	/**
	 * Create a JSON string value
	 * @param value String value
	 * @return New JSON string or nullptr on failure
	 */
	virtual JsonValue* CreateString(const char* value) = 0;

	/**
	 * Get boolean value from JSON
	 * @param handle JSON value
	 * @param out_value Pointer to receive boolean value
	 * @return true on success, false on type mismatch
	 */
	virtual bool GetBool(JsonValue* handle, bool* out_value) = 0;

	/**
	 * Get float value from JSON
	 * @param handle JSON value
	 * @param out_value Pointer to receive float value
	 * @return true on success, false on type mismatch
	 */
	virtual bool GetFloat(JsonValue* handle, double* out_value) = 0;

	/**
	 * Get integer value from JSON
	 * @param handle JSON value
	 * @param out_value Pointer to receive integer value
	 * @return true on success, false on type mismatch
	 */
	virtual bool GetInt(JsonValue* handle, int* out_value) = 0;

	/**
	 * Get 64-bit integer value from JSON (auto-detects signed/unsigned)
	 * @param handle JSON value
	 * @param out_value Pointer to receive 64-bit integer value (std::variant<int64_t, uint64_t>)
	 * @return true on success, false on type mismatch
	 */
	virtual bool GetInt64(JsonValue* handle, std::variant<int64_t, uint64_t>* out_value) = 0;

	/**
	 * Get string value from JSON
	 * @param handle JSON value
	 * @param out_str Pointer to receive string pointer
	 * @param out_len Pointer to receive string length
	 * @return true on success, false on type mismatch
	 */
	virtual bool GetString(JsonValue* handle, const char** out_str, size_t* out_len) = 0;

	/**
	 * Get value using JSON Pointer
	 * @param handle JSON value
	 * @param path JSON Pointer path (e.g., "/users/0/name")
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return JSON value or nullptr on error
	 */
	virtual JsonValue* PtrGet(JsonValue* handle, const char* path,
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
	virtual bool PtrGetBool(JsonValue* handle, const char* path, bool* out_value,
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
	virtual bool PtrGetFloat(JsonValue* handle, const char* path, double* out_value,
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
	virtual bool PtrGetInt(JsonValue* handle, const char* path, int* out_value,
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Get 64-bit integer value using JSON Pointer (auto-detects signed/unsigned)
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_value Pointer to receive 64-bit integer value (std::variant<int64_t, uint64_t>)
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrGetInt64(JsonValue* handle, const char* path, std::variant<int64_t, uint64_t>* out_value,
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
	virtual bool PtrGetString(JsonValue* handle, const char* path, const char** out_str,
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
	virtual bool PtrGetIsNull(JsonValue* handle, const char* path, bool* out_is_null,
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
	virtual bool PtrGetLength(JsonValue* handle, const char* path, size_t* out_len,
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
	virtual bool PtrSet(JsonValue* handle, const char* path, JsonValue* value,
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
	virtual bool PtrSetBool(JsonValue* handle, const char* path, bool value,
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
	virtual bool PtrSetFloat(JsonValue* handle, const char* path, double value,
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
	virtual bool PtrSetInt(JsonValue* handle, const char* path, int value,
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Set 64-bit integer value using JSON Pointer (mutable only, auto-detects signed/unsigned)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path
	 * @param value 64-bit integer value (std::variant<int64_t, uint64_t>)
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrSetInt64(JsonValue* handle, const char* path, std::variant<int64_t, uint64_t> value,
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
	virtual bool PtrSetString(JsonValue* handle, const char* path, const char* value,
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Set null value using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrSetNull(JsonValue* handle, const char* path,
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
	virtual bool PtrAdd(JsonValue* handle, const char* path, JsonValue* value,
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
	virtual bool PtrAddBool(JsonValue* handle, const char* path, bool value,
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
	virtual bool PtrAddFloat(JsonValue* handle, const char* path, double value,
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
	virtual bool PtrAddInt(JsonValue* handle, const char* path, int value,
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Add 64-bit integer to array using JSON Pointer (mutable only, auto-detects signed/unsigned)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path to array
	 * @param value 64-bit integer value (std::variant<int64_t, uint64_t>)
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrAddInt64(JsonValue* handle, const char* path, std::variant<int64_t, uint64_t> value,
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
	virtual bool PtrAddString(JsonValue* handle, const char* path, const char* value,
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Add null to array using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path to array
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrAddNull(JsonValue* handle, const char* path,
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Remove value using JSON Pointer (mutable only)
	 * @param handle Mutable JSON value
	 * @param path JSON Pointer path
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on error
	 */
	virtual bool PtrRemove(JsonValue* handle, const char* path,
		char* error = nullptr, size_t error_size = 0) = 0;

	/**
	 * Try to get value using JSON Pointer (no error on failure)
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @return JSON value or nullptr if not found
	 */
	virtual JsonValue* PtrTryGet(JsonValue* handle, const char* path) = 0;

	/**
	 * Try to get boolean value using JSON Pointer (returns false on failure)
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_value Pointer to receive boolean value
	 * @return true on success, false if not found or type mismatch
	 */
	virtual bool PtrTryGetBool(JsonValue* handle, const char* path, bool* out_value) = 0;

	/**
	 * Try to get float value using JSON Pointer (returns false on failure)
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_value Pointer to receive float value
	 * @return true on success, false if not found or type mismatch
	 */
	virtual bool PtrTryGetFloat(JsonValue* handle, const char* path, double* out_value) = 0;

	/**
	 * Try to get integer value using JSON Pointer (returns false on failure)
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_value Pointer to receive integer value
	 * @return true on success, false if not found or type mismatch
	 */
	virtual bool PtrTryGetInt(JsonValue* handle, const char* path, int* out_value) = 0;

	/**
	 * Try to get 64-bit integer value using JSON Pointer (auto-detects signed/unsigned, returns false on failure)
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_value Pointer to receive 64-bit integer value (std::variant<int64_t, uint64_t>)
	 * @return true on success, false if not found or type mismatch
	 */
	virtual bool PtrTryGetInt64(JsonValue* handle, const char* path, std::variant<int64_t, uint64_t>* out_value) = 0;

	/**
	 * Try to get string value using JSON Pointer (returns false on failure)
	 * @param handle JSON value
	 * @param path JSON Pointer path
	 * @param out_str Pointer to receive string pointer
	 * @param out_len Pointer to receive string length
	 * @return true on success, false if not found or type mismatch
	 */
	virtual bool PtrTryGetString(JsonValue* handle, const char* path, const char** out_str, size_t* out_len) = 0;

	// Note: Iterators are stateful and stored in the JsonValue object
	// Call these functions in a loop until they return false

	/**
	 * Get next key-value pair from object iterator
	 * @param handle JSON object
	 * @param out_key Pointer to receive key string
	 * @param out_key_len Pointer to receive key length (can be nullptr)
	 * @param out_value Pointer to receive value (creates new JsonValue)
	 * @return true if iteration continues, false if iteration complete
	 * @note Iterator state is maintained in handle. Returns false when iteration completes.
	 * @deprecated Use JSONObjIter instead for better iterator support
	 */
	virtual bool ObjectForeachNext(JsonValue* handle, const char** out_key,
	                                size_t* out_key_len, JsonValue** out_value) = 0;

	/**
	 * Get next index-value pair from array iterator
	 * @param handle JSON array
	 * @param out_index Pointer to receive current index
	 * @param out_value Pointer to receive value (creates new JsonValue)
	 * @return true if iteration continues, false if iteration complete
	 * @note Iterator state is maintained in handle. Returns false when iteration completes.
	 * @deprecated Use JSONArrIter instead for better iterator support
	 */
	virtual bool ArrayForeachNext(JsonValue* handle, size_t* out_index,
	                               JsonValue** out_value) = 0;

	/**
	 * Get next key from object iterator (key only, no value)
	 * @param handle JSON object
	 * @param out_key Pointer to receive key string
	 * @param out_key_len Pointer to receive key length (can be nullptr)
	 * @return true if iteration continues, false if iteration complete
	 * @note Iterator state is maintained in handle. Returns false when iteration completes.
	 * @deprecated Use JSONObjIter instead for better iterator support
	 */
	virtual bool ObjectForeachKeyNext(JsonValue* handle, const char** out_key,
	                                   size_t* out_key_len) = 0;

	/**
	 * Get next index from array iterator (index only, no value)
	 * @param handle JSON array
	 * @param out_index Pointer to receive current index
	 * @return true if iteration continues, false if iteration complete
	 * @note Iterator state is maintained in handle. Returns false when iteration completes.
	 * @deprecated Use JSONArrIter instead for better iterator support
	 */
	virtual bool ArrayForeachIndexNext(JsonValue* handle, size_t* out_index) = 0;

	/**
	 * Release a JsonValue object
	 * External extensions should use this instead of deleting directly
	 * @param value The JsonValue to release
	 */
	virtual void Release(JsonValue* value) = 0;

	/**
	 * Get the HandleType_t for JSON handles
	 * External extensions MUST use this method to obtain the handle type
	 * @return The HandleType_t for JSON handles
	 */
	virtual HandleType_t GetHandleType() = 0;

	/**
	 * Read JsonValue from a SourceMod handle
	 * @param pContext Plugin context
	 * @param handle Handle to read from
	 * @return JsonValue pointer, or nullptr on error (error will be reported to context)
	 */
	virtual JsonValue* GetFromHandle(IPluginContext* pContext, Handle_t handle) = 0;

	/**
	 * Initialize an array iterator (same as ArrIterWith but returns pointer)
	 * @param handle JSON array value
	 * @return New array iterator or nullptr on error
	 */
	virtual JsonArrIter* ArrIterInit(JsonValue* handle) = 0;

	/**
	 * Create an array iterator with an array
	 * @param handle JSON array value
	 * @return New array iterator or nullptr on error
	 */
	virtual JsonArrIter* ArrIterWith(JsonValue* handle) = 0;

	/**
	 * Get next element from array iterator
	 * @param iter Array iterator
	 * @return JSON value wrapper for next element, or nullptr if iteration complete
	 */
	virtual JsonValue* ArrIterNext(JsonArrIter* iter) = 0;

	/**
	 * Check if array iterator has more elements
	 * @param iter Array iterator
	 * @return true if has next element, false otherwise
	 */
	virtual bool ArrIterHasNext(JsonArrIter* iter) = 0;

	/**
	 * Get current index in array iteration
	 * @param iter Array iterator
	 * @return Current index (0-based), or SIZE_MAX if iterator is not positioned
	 */
	virtual size_t ArrIterGetIndex(JsonArrIter* iter) = 0;

	/**
	 * Remove current element from array (mutable only)
	 * @param iter Mutable array iterator
	 * @return Pointer to removed value, or nullptr on error
	 */
	virtual void* ArrIterRemove(JsonArrIter* iter) = 0;

	/**
	 * Initialize an object iterator (same as ObjIterWith but returns pointer)
	 * @param handle JSON object value
	 * @return New object iterator or nullptr on error
	 */
	virtual JsonObjIter* ObjIterInit(JsonValue* handle) = 0;

	/**
	 * Create an object iterator with an object
	 * @param handle JSON object value
	 * @return New object iterator or nullptr on error
	 */
	virtual JsonObjIter* ObjIterWith(JsonValue* handle) = 0;

	/**
	 * Get next key from object iterator
	 * @param iter Object iterator
	 * @return Key value (yyjson_val* for immutable, yyjson_mut_val* for mutable), or nullptr if iteration complete
	 */
	virtual void* ObjIterNext(JsonObjIter* iter) = 0;

	/**
	 * Check if object iterator has more elements
	 * @param iter Object iterator
	 * @return true if has next element, false otherwise
	 */
	virtual bool ObjIterHasNext(JsonObjIter* iter) = 0;

	/**
	 * Get value by key from object iterator
	 * @param iter Object iterator
	 * @param key Key value (yyjson_val* or yyjson_mut_val*)
	 * @return JSON value wrapper for the value, or nullptr on error
	 */
	virtual JsonValue* ObjIterGetVal(JsonObjIter* iter, void* key) = 0;

	/**
	 * Iterates to a specified key and returns the value
	 * @param iter Object iterator
	 * @param key Key name string
	 * @return JSON value wrapper for the value, or nullptr if key not found
	 * @note This function searches the object using the iterator structure
   * @warning This function takes a linear search time if the key is not nearby.
	 */
	virtual JsonValue* ObjIterGet(JsonObjIter* iter, const char* key) = 0;

	/**
	 * Get current index in object iteration
	 * @param iter Object iterator
	 * @return Current index (0-based), or SIZE_MAX if iterator is not positioned
	 */
	virtual size_t ObjIterGetIndex(JsonObjIter* iter) = 0;

	/**
	 * Remove current key-value pair from object (mutable only)
	 * @param iter Mutable object iterator
	 * @return Pointer to removed key, or nullptr on error
	 */
	virtual void* ObjIterRemove(JsonObjIter* iter) = 0;

	/**
	 * Release an array iterator
	 * @param iter Iterator to release
	 */
	virtual void ReleaseArrIter(JsonArrIter* iter) = 0;

	/**
	 * Release an object iterator
	 * @param iter Iterator to release
	 */
	virtual void ReleaseObjIter(JsonObjIter* iter) = 0;

	/**
	 * Get the HandleType_t for array iterator handles
	 * @return The HandleType_t for array iterator handles
	 */
	virtual HandleType_t GetArrIterHandleType() = 0;

	/**
	 * Get the HandleType_t for object iterator handles
	 * @return The HandleType_t for object iterator handles
	 */
	virtual HandleType_t GetObjIterHandleType() = 0;

	/**
	 * Read JsonArrIter from a SourceMod handle
	 * @param pContext Plugin context
	 * @param handle Handle to read from
	 * @return JsonArrIter pointer, or nullptr on error
	 */
	virtual JsonArrIter* GetArrIterFromHandle(IPluginContext* pContext, Handle_t handle) = 0;

	/**
	 * Read JsonObjIter from a SourceMod handle
	 * @param pContext Plugin context
	 * @param handle Handle to read from
	 * @return JsonObjIter pointer, or nullptr on error
	 */
	virtual JsonObjIter* GetObjIterFromHandle(IPluginContext* pContext, Handle_t handle) = 0;

	/**
	 * Read a JSON number from string
	 * @param dat The JSON data (UTF-8 without BOM), null-terminator is required
	 * @param read_flg Read flags (YYJSON_READ_FLAG values, default: 0)
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @param out_consumed Pointer to receive number of characters consumed (optional)
	 * @return New JSON number value or nullptr on error
	 * @note The returned value is a mutable number value
	 */
	virtual JsonValue* ReadNumber(const char* dat, uint32_t read_flg = 0,
		char* error = nullptr, size_t error_size = 0, size_t* out_consumed = nullptr) = 0;

	/**
	 * Write a JSON number to string buffer
	 * @param handle JSON number value
	 * @param buffer Output buffer (must be at least 40 bytes for floating-point, 21 bytes for integer)
	 * @param buffer_size Buffer size
	 * @param out_written Pointer to receive number of characters written (excluding null terminator) (optional)
	 * @return true on success, false on error
	 * @note The buffer must be large enough to hold the number string
	 */
	virtual bool WriteNumber(JsonValue* handle, char* buffer, size_t buffer_size,
		size_t* out_written = nullptr) = 0;

	/**
	 * Set floating-point number's output format to single-precision
	 * @param handle JSON floating-point number value
	 * @param flt true to use single-precision (float), false to use double-precision (double)
	 * @return true on success, false if handle is not a floating-point number
	 * @note Only works on floating-point numbers (not integers)
	 */
	virtual bool SetFpToFloat(JsonValue* handle, bool flt) = 0;

	/**
	 * Set floating-point number's output format to fixed-point notation
	 * @param handle JSON floating-point number value
	 * @param prec Precision (1-15), similar to ECMAScript Number.prototype.toFixed(prec) but with trailing zeros removed
	 * @return true on success, false if handle is not a floating-point number or prec is out of range
	 * @note Only works on floating-point numbers (not integers)
	 * @note This will produce shorter output but may lose some precision
	 */
	virtual bool SetFpToFixed(JsonValue* handle, int prec) = 0;

	/**
	 * Directly modify a JSON value to boolean type
	 * @param handle JSON value to modify (cannot be object or array)
	 * @param value Boolean value
	 * @return true on success, false if handle is object or array
	 * @warning For immutable documents, this breaks immutability. Use with caution.
	 * @note This modifies the value in-place without creating a new value
	 */
	virtual bool SetBool(JsonValue* handle, bool value) = 0;

	/**
	 * Directly modify a JSON value to integer type
	 * @param handle JSON value to modify (cannot be object or array)
	 * @param value Integer value
	 * @return true on success, false if handle is object or array
	 * @warning For immutable documents, this breaks immutability. Use with caution.
	 * @note This modifies the value in-place without creating a new value
	 */
	virtual bool SetInt(JsonValue* handle, int value) = 0;

	/**
	 * Directly modify a JSON value to 64-bit integer type (auto-detects signed/unsigned)
	 * @param handle JSON value to modify (cannot be object or array)
	 * @param value 64-bit integer value (std::variant<int64_t, uint64_t>)
	 * @return true on success, false if handle is object or array
	 * @warning For immutable documents, this breaks immutability. Use with caution.
	 * @note This modifies the value in-place without creating a new value
	 */
	virtual bool SetInt64(JsonValue* handle, std::variant<int64_t, uint64_t> value) = 0;

	/**
	 * Directly modify a JSON value to floating-point type
	 * @param handle JSON value to modify (cannot be object or array)
	 * @param value Float value
	 * @return true on success, false if handle is object or array
	 * @warning For immutable documents, this breaks immutability. Use with caution.
	 * @note This modifies the value in-place without creating a new value
	 */
	virtual bool SetFloat(JsonValue* handle, double value) = 0;

	/**
	 * Directly modify a JSON value to string type
	 * @param handle JSON value to modify (cannot be object or array)
	 * @param value String value (will be copied for mutable documents)
	 * @return true on success, false if handle is object or array or value is null
	 * @warning For immutable documents, this breaks immutability and does NOT copy the string. Use with caution.
	 * @warning For immutable documents, the string pointer must remain valid for the document's lifetime
	 * @note For mutable documents, the string is copied into the document's memory pool
	 */
	virtual bool SetString(JsonValue* handle, const char* value) = 0;

	/**
	 * Directly modify a JSON value to null type
	 * @param handle JSON value to modify (cannot be object or array)
	 * @return true on success, false if handle is object or array
	 * @warning For immutable documents, this breaks immutability. Use with caution.
	 * @note This modifies the value in-place without creating a new value
	 */
	virtual bool SetNull(JsonValue* handle) = 0;

	/**
	 * Parse an int64 string value into a variant (int64_t or uint64_t)
	 * @param value String representation of the integer
	 * @param out_value Output variant to store the parsed value
	 * @param error Error buffer (optional)
	 * @param error_size Error buffer size
	 * @return true on success, false on parse error
	 * @note Auto-detects whether to use signed or unsigned based on value range
	 * @note Negative values are stored as int64_t, large positive values may be stored as uint64_t
	 */
	virtual bool ParseInt64Variant(const char* value, std::variant<int64_t, uint64_t>* out_value,
		char* error = nullptr, size_t error_size = 0) = 0;
};

#endif // _INCLUDE_SM_JSON_IJSONMANAGER_H_