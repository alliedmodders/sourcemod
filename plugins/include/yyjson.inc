#if defined _yyjson_included
  #endinput
#endif
#define _yyjson_included

// JSON value types
enum YYJSON_TYPE
{
  YYJSON_TYPE_NONE = 0, // Invalid type
  YYJSON_TYPE_RAW  = 1, // Raw string (stored as is, used for number/literal)
  YYJSON_TYPE_NULL = 2, // null
  YYJSON_TYPE_BOOL = 3, // true/false
  YYJSON_TYPE_NUM  = 4, // Number (integer/real)
  YYJSON_TYPE_STR  = 5, // String
  YYJSON_TYPE_ARR  = 6, // Array
  YYJSON_TYPE_OBJ  = 7  // Object
}

// JSON value subtypes
enum YYJSON_SUBTYPE
{
  YYJSON_SUBTYPE_NONE  = 0 << 3, // Invalid subtype
  YYJSON_SUBTYPE_FALSE = 0 << 3, // Boolean false
  YYJSON_SUBTYPE_TRUE  = 1 << 3, // Boolean true
  YYJSON_SUBTYPE_UINT  = 0 << 3, // Unsigned integer
  YYJSON_SUBTYPE_SINT  = 1 << 3, // Signed integer
  YYJSON_SUBTYPE_REAL  = 2 << 3, // Real number (float/double)
  YYJSON_SUBTYPE_NOESC = 1 << 3  // String without escape character
}

// JSON reader flags for parsing behavior
enum YYJSON_READ_FLAG
{
  YYJSON_READ_NOFLAG                  = 0 << 0,  // Default behavior
  YYJSON_READ_INSITU                  = 1 << 0,  // Read JSON data in-situ (modify input string)
  YYJSON_READ_STOP_WHEN_DONE          = 1 << 1,  // Stop when done instead of issuing an error if there's additional content
  YYJSON_READ_ALLOW_TRAILING_COMMAS   = 1 << 2,  // Allow trailing commas at the end of arrays/objects
  YYJSON_READ_ALLOW_COMMENTS          = 1 << 3,  // Allow C-style comments (/* */) and line comments (//)
  YYJSON_READ_ALLOW_INF_AND_NAN       = 1 << 4,  // Allow nan/inf number (non-standard JSON)
  YYJSON_READ_NUMBER_AS_RAW           = 1 << 5,  // Read numbers as raw strings
  YYJSON_READ_ALLOW_INVALID_UNICODE   = 1 << 6,  // Allow invalid unicode when parsing string
  YYJSON_READ_BIGNUM_AS_RAW           = 1 << 7,  // Read big numbers as raw strings
  YYJSON_READ_ALLOW_BOM               = 1 << 8,  // Allow BOM (Byte Order Mark) at the beginning of the JSON string
  YYJSON_READ_ALLOW_EXT_NUMBER        = 1 << 9,  // Allow extended number (non-standard JSON)
  YYJSON_READ_ALLOW_EXT_ESCAPE        = 1 << 10, // Allow extended escape sequences in strings (non-standard)
  YYJSON_READ_ALLOW_EXT_WHITESPACE    = 1 << 11, // Allow extended whitespace characters (non-standard)
  YYJSON_READ_ALLOW_SINGLE_QUOTED_STR = 1 << 12, // Allow strings enclosed in single quotes (non-standard)
  YYJSON_READ_ALLOW_UNQUOTED_KEY      = 1 << 13, // Allow object keys without quotes (non-standard)
  YYJSON_READ_JSON5                   = YYJSON_READ_ALLOW_TRAILING_COMMAS |
                                        YYJSON_READ_ALLOW_COMMENTS |
                                        YYJSON_READ_ALLOW_INF_AND_NAN |
                                        YYJSON_READ_ALLOW_EXT_NUMBER |
                                        YYJSON_READ_ALLOW_EXT_ESCAPE |
                                        YYJSON_READ_ALLOW_EXT_WHITESPACE |
                                        YYJSON_READ_ALLOW_SINGLE_QUOTED_STR |
                                        YYJSON_READ_ALLOW_UNQUOTED_KEY // Allow JSON5 format, see: [https://json5.org]
}

// JSON writer flags for serialization behavior
enum YYJSON_WRITE_FLAG
{
  YYJSON_WRITE_NOFLAG                 = 0 << 0, // Default behavior
  YYJSON_WRITE_PRETTY                 = 1 << 0, // Pretty print with indent and newline
  YYJSON_WRITE_ESCAPE_UNICODE         = 1 << 1, // Escape unicode as \uXXXX
  YYJSON_WRITE_ESCAPE_SLASHES         = 1 << 2, // Escape '/' as '\/'
  YYJSON_WRITE_ALLOW_INF_AND_NAN      = 1 << 3, // Write inf/nan number (non-standard JSON)
  YYJSON_WRITE_INF_AND_NAN_AS_NULL    = 1 << 4, // Write inf/nan as null
  YYJSON_WRITE_ALLOW_INVALID_UNICODE  = 1 << 5, // Allow invalid unicode when encoding string
  YYJSON_WRITE_PRETTY_TWO_SPACES      = 1 << 6, // Use 2 spaces for indent when pretty print
  YYJSON_WRITE_NEWLINE_AT_END         = 1 << 7, // Add newline at the end of output
  YYJSON_WRITE_FP_TO_FLOAT            = 1 << 27 // Write floating-point numbers using single-precision (float)
}

/** Write floating-point number using fixed-point notation
 - This is similar to ECMAScript Number.prototype.toFixed(prec) but with trailing zeros removed. The prec ranges from 1 to 15
 - This will produce shorter output but may lose some precision
*/
stock YYJSON_WRITE_FLAG YYJSON_WRITE_FP_TO_FIXED(int n)
{
  return view_as<YYJSON_WRITE_FLAG>(n << 28);
}

// Sort order for arrays and objects
enum YYJSON_SORT_ORDER
{
  YYJSON_SORT_ASC    = 0, // Ascending order (default)
  YYJSON_SORT_DESC   = 1, // Descending order
  YYJSON_SORT_RANDOM = 2  // Random order
}

methodmap YYJSON < Handle
{
  /**
  * Creates a JSON value using a format string and arguments
  * Format specifiers:
  * - s: string
  * - i: integer
  * - f: float
  * - b: boolean
  * - n: null
  * - {: start object
  * - }: end object
  * - [: start array
  * - ]: end array
  *
  * @param format            Format string
  * @param ...               Arguments based on format string
  *
  * @return                  JSON handle
  * @error                   If format string is invalid or arguments don't match
  */
  public static native any Pack(const char[] format, any ...);

  /**
  * Iterates over the object's key-value pairs
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param buffer            Buffer to copy key name to
  * @param maxlength         Maximum length of the string buffer
  * @param value             JSON handle to store the current value
  *
  * @return                  True if there are more elements, false when iteration is complete
  * @error                   Invalid handle or handle is not an object
  */
  public native bool ForeachObject(char[] buffer, int maxlength, YYJSON &value);

  /**
  * Iterates over the array's values
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param index             Variable to store current array index (starting from 0)
  * @param value             JSON handle to store the current value
  *
  * @return                  True if there are more elements, false when iteration is complete
  * @error                   Invalid handle or handle is not an array
  */
  public native bool ForeachArray(int &index, YYJSON &value);

  /**
  * Same as ForeachObject, but only iterates over the object's keys
  *
  * @note                    Use this when you only need keys (faster than ForeachObject since it doesn't create value handles)
  *
  * @param buffer            Buffer to copy key name to
  * @param maxlength         Maximum length of the string buffer
  *
  * @return                  True if there are more elements, false when iteration is complete
  * @error                   Invalid handle or handle is not an object
  */
  public native bool ForeachKey(char[] buffer, int maxlength);

  /**
  * Same as ForeachArray, but only iterates over the array's indexes
  *
  * @note                    Use this when you only need indexes (faster than ForeachArray since it doesn't create value handles)
  *
  * @param index             Variable to store current array index (starting from 0)
  *
  * @return                  True if there are more elements, false when iteration is complete
  * @error                   Invalid handle or handle is not an array
  */
  public native bool ForeachIndex(int &index);

  /**
  * Converts an immutable JSON document to a mutable one
  *
  * @return                  Handle to the new mutable JSON document, INVALID_HANDLE on failure
  * @error                   If the document is already mutable
  */
  public native any ToMutable();

  /**
  * Converts a mutable JSON document to an immutable one
  *
  * @return                  Handle to the new immutable JSON document, INVALID_HANDLE on failure
  * @error                   If the document is already immutable
  */
  public native any ToImmutable();

  /**
  * Write a document to JSON file with options
  *
  * @note                    On 32-bit operating system, files larger than 2GB may fail to write
  *
  * @param file              The JSON file's path. If this path is NULL or invalid, the function will fail and return false. If this file is not empty, the content will be discarded
  * @param flag              The JSON write options
  *
  * @return                  True on success, false on failure
  */
  public native bool ToFile(const char[] file, YYJSON_WRITE_FLAG flag = YYJSON_WRITE_NOFLAG);

  /**
  * Write a value to JSON string
  *
  * @param buffer            String buffer to write to
  * @param maxlength         Maximum length of the string buffer
  * @param flag              The JSON write options
  *
  * @return                  Number of characters written to the buffer, including the null terminator. 0 on failure
  */
  public native int ToString(char[] buffer, int maxlength, YYJSON_WRITE_FLAG flag = YYJSON_WRITE_NOFLAG);

  /**
  * Parses JSON string or a file that contains JSON
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param string            String or file to parse
  * @param is_file           True to treat string param as file, false otherwise
  * @param is_mutable_doc    True to create a mutable document, false to create an immutable one
  * @param flag              The JSON read options
  *
  * @return                  JSON handle, false on failure
  */
  public static native any Parse(const char[] string, bool is_file = false, bool is_mutable_doc = false, YYJSON_READ_FLAG flag = YYJSON_READ_NOFLAG);

  /**
  * Creates a deep copy of a JSON value and returns it as a new JSON handle.
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  * @note                    This function is recursive and may cause a stack overflow if the object level is too deep
  * @note                    The mutability of the returned copy depends on the targetDoc parameter
  *
  * @param targetDoc         The target document that determines whether the copy will be mutable or immutable
  * @param sourceValue       The source JSON value to be copied
  *
  * @return                  New JSON handle on success, false on failure
  */
  public static native any DeepCopy(const YYJSON targetDoc, const YYJSON sourceValue);

  /**
  * Returns the JSON value's type description
  *
  * @param value             JSON handle
  * @param buffer            String buffer to write to
  * @param maxlength         Maximum length of the string buffer
  *
  * @return                  The return value should be one of these strings: "raw", "null", "string",
  *                          "array", "object", "true", "false", "uint", "sint", "real", "unknown".
  */
  public static native void GetTypeDesc(const YYJSON value, char[] buffer, int maxlength);

  /**
  * Returns whether two JSON values are equal (deep compare)
  *
  * @note                    This function is recursive and may cause a stack overflow if the object level is too deep
  * @note                    the result may be inaccurate if object has duplicate keys
  *
  * @param value1            JSON handle
  * @param value2            JSON handle
  *
  * @return                  True if they are the same, false otherwise
  */
  public static native bool Equals(const YYJSON value1, const YYJSON value2);

  /**
  * Creates and returns a boolean value
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param value             The boolean value to be set
  *
  * @return                  JSON handle, NULL on error
  */
  public static native YYJSON CreateBool(bool value);

  /**
  * Creates and returns a float value
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param value             The float value to be set
  *
  * @return                  JSON handle, NULL on error
  */
  public static native YYJSON CreateFloat(float value);

  /**
  * Creates and returns a int value
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param value             The int value to be set
  *
  * @return                  JSON handle, NULL on error
  */
  public static native YYJSON CreateInt(int value);

  /**
  * Creates and returns a intger64 value
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param value             The intger64 value to be set
  *
  * @return                  JSON handle, NULL on error
  */
  public static native YYJSON CreateInt64(const char[] value);

  /**
  * Creates and returns a string value
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param value             The string value to be set
  *
  * @return                  JSON handle, NULL on error
  */
  public static native YYJSON CreateString(const char[] value);

  /**
  * Creates and returns a null value
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @return                  JSON handle, NULL on error
  */
  public static native YYJSON CreateNull();

  /**
  * Get boolean value by a JSON Handle
  *
  * @param value             JSON handle
  *
  * @return                  Boolean value
  */
  public static native bool GetBool(const YYJSON value);

  /**
  * Get float value by a JSON Handle
  *
  * @param value             JSON handle
  *
  * @return                  float value
  */
  public static native float GetFloat(const YYJSON value);

  /**
  * Get int value by a JSON Handle
  *
  * @param value             JSON handle
  *
  * @return                  int value
  */
  public static native int GetInt(const YYJSON value);

  /**
  * Get intger64 value by a JSON Handle
  *
  * @param value             JSON handle
  * @param buffer            Buffer to copy to
  * @param maxlength         Maximum size of the buffer
  *
  * @return                  True on success, false on failure
  */
  public static native bool GetInt64(const YYJSON value, char[] buffer, int maxlength);

  /**
  * Get string value by a JSON Handle
  *
  * @param value             JSON handle
  * @param buffer            Buffer to copy to
  * @param maxlength         Maximum size of the buffer
  *
  * @return                  True on success, false on failure
  */
  public static native bool GetString(const YYJSON value, char[] buffer, int maxlength);

  /**
  * Get JSON Handle serialized size in bytes (including null-terminator)
  *
  * @param flag              The JSON write options
  *
  * @return                  serialized size
  */
  public native int GetSerializedSize(YYJSON_WRITE_FLAG flag = YYJSON_WRITE_NOFLAG);

  /**
  * Get value by a JSON Pointer
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param path              The JSON pointer string
  *
  * @return                  The value referenced by the JSON pointer
  */
  public native any PtrGet(const char[] path);

  /**
  * Get boolean value by a JSON Pointer
  *
  * @param path              The JSON pointer string
  *
  * @return                  boolean value referenced by the JSON pointer
  */
  public native bool PtrGetBool(const char[] path);

  /**
  * Get float value by a JSON Pointer
  *
  * @param path              The JSON pointer string
  *
  * @return                  float value referenced by the JSON pointer
  */
  public native float PtrGetFloat(const char[] path);

  /**
  * Get integer value by a JSON Pointer
  *
  * @param path              The JSON pointer string
  *
  * @return                  integer value referenced by the JSON pointer
  */
  public native int PtrGetInt(const char[] path);

  /**
  * Get integer64 value by a JSON Pointer
  *
  * @param path              The JSON pointer string
  * @param buffer            Buffer to copy to
  * @param maxlength         Maximum size of the buffer
  *
  * @return                  integer64 value referenced by the JSON pointer
  */
  public native bool PtrGetInt64(const char[] path, char[] buffer, int maxlength);

  /**
  * Get string value by a JSON Pointer
  *
  * @param path              The JSON pointer string
  * @param buffer            Buffer to copy to
  * @param maxlength         Maximum size of the buffer
  *
  * @return                  True on success, false on failure
  */
  public native bool PtrGetString(const char[] path, char[] buffer, int maxlength)

  /**
  * Get value is null by a JSON Pointer
  *
  * @param path              The JSON pointer string
  *
  * @return                  True if the value is null, false otherwise
  */
  public native bool PtrGetIsNull(const char[] path)

  /**
  * Get JSON content length (string length, array size, object size)
  * Returns 0 if val is NULL or type is not string/array/object
  * if str including null-terminator
  *
  * @param path              The JSON pointer string
  *
  * @return                  JSON content length
  */
  public native int PtrGetLength(const char[] path)

  /**
  * Set value by a JSON Pointer
  *
  * @note                    The parent nodes will be created if they do not exist. If the target value already exists, it will be replaced by the new value
  *
  * @param path              The JSON pointer string
  * @param value             The value to be set, pass NULL to remove
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrSet(const char[] path, YYJSON value);

  /**
  * Set boolean value by a JSON Pointer
  *
  * @note                    The parent nodes will be created if they do not exist. If the target value already exists, it will be replaced by the new value
  *
  * @param path              The JSON pointer string
  * @param value             The boolean value to be set, pass NULL to remove
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrSetBool(const char[] path, bool value);

  /**
  * Set float value by a JSON Pointer
  *
  * @note                    The parent nodes will be created if they do not exist. If the target value already exists, it will be replaced by the new value
  *
  * @param path              The JSON pointer string
  * @param value             The float value to be set, pass NULL to remove
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrSetFloat(const char[] path, float value);

  /**
  * Set integer value by a JSON Pointer
  *
  * @note                    The parent nodes will be created if they do not exist. If the target value already exists, it will be replaced by the new value
  *
  * @param path              The JSON pointer string
  * @param value             The integer value to be set, pass NULL to remove
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrSetInt(const char[] path, int value);

  /**
  * Set intger64 value by a JSON Pointer
  *
  * @note                    The parent nodes will be created if they do not exist. If the target value already exists, it will be replaced by the new value
  *
  * @param path              The JSON pointer string
  * @param value             The intger64 value to be set, pass NULL to remove
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrSetInt64(const char[] path, const char[] value);

  /**
  * Set string value by a JSON Pointer
  *
  * @note                    The parent nodes will be created if they do not exist. If the target value already exists, it will be replaced by the new value
  *
  * @param path              The JSON pointer string
  * @param value             The string value to be set, pass NULL to remove
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrSetString(const char[] path, const char[] value);

  /**
  * Set null value by a JSON Pointer
  *
  * @note                    The parent nodes will be created if they do not exist. If the target value already exists, it will be replaced by the new value
  *
  * @param path              The JSON pointer string
  * @param value             The null value to be set, pass NULL to remove
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrSetNull(const char[] path);

  /**
  * Add (insert) value by a JSON pointer
  *
  * @param path              The JSON pointer string
  * @param value             The value to be added
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrAdd(const char[] path, YYJSON value);

  /**
  * Add (insert) boolean value by a JSON pointer
  *
  * @param path              The JSON pointer string
  * @param value             The boolean value to be added
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrAddBool(const char[] path, bool value);

  /**
  * Add (insert) float value by a JSON pointer
  *
  * @param path              The JSON pointer string
  * @param value             The float value to be added
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrAddFloat(const char[] path, float value);

  /**
  * Add (insert) integer value by a JSON pointer
  *
  * @param path              The JSON pointer string
  * @param value             The int value to be added
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrAddInt(const char[] path, int value);

  /**
  * Add (insert) integer64 value by a JSON pointer
  *
  * @param path              The JSON pointer string
  * @param value             The intger64 value to be added
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrAddInt64(const char[] path, const char[] value);

  /**
  * Add (insert) string value by a JSON pointer
  *
  * @param path              The JSON pointer string
  * @param value             The str value to be added
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrAddString(const char[] path, const char[] value);

  /**
  * Add (insert) null value by a JSON pointer
  *
  * @param path              The JSON pointer string
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrAddNull(const char[] path);

  /**
  * Remove value by a JSON pointer
  *
  * @param path              The JSON pointer string
  *
  * @return                  true if removed value, false otherwise
  */
  public native bool PtrRemove(const char[] path)

  /**
  * Try to get value by a JSON Pointer
  *
  * @param path              The JSON pointer string
  * @param value             Handle to store value
  *
  * @return                  true on success, false otherwise
  */
  public native bool PtrTryGetVal(const char[] path, YYJSON &value);

  /**
  * Try to get boolean value by a JSON Pointer
  *
  * @param path              The JSON pointer string
  * @param value             Store the boolean value
  *
  * @return                  true on success, false otherwise
  */
  public native bool PtrTryGetBool(const char[] path, bool &value);

  /**
  * Try to get float value by a JSON Pointer
  *
  * @param path              The JSON pointer string
  * @param value             Store the float value
  *
  * @return                  true on success, false otherwise
  */
  public native bool PtrTryGetFloat(const char[] path, float &value);

  /**
  * Try to get integer value by a JSON Pointer
  *
  * @param path              The JSON pointer string
  * @param value             Store the integer value
  *
  * @return                  true on success, false otherwise
  */
  public native bool PtrTryGetInt(const char[] path, int &value);

  /**
  * Try to get integer64 value by a JSON Pointer
  *
  * @param path              The JSON pointer string
  * @param buffer            Buffer to store the integer64 value
  * @param maxlength         Maximum length of the buffer
  *
  * @return                  true on success, false otherwise
  */
  public native bool PtrTryGetInt64(const char[] path, char[] buffer, int maxlength);

  /**
  * Try to get string value by a JSON Pointer
  *
  * @param path              The JSON pointer string
  * @param buffer            Buffer to store the string value
  * @param maxlength         Maximum length of the buffer
  *
  * @return                  true on success, false otherwise
  */
  public native bool PtrTryGetString(const char[] path, char[] buffer, int maxlength);

  /**
  * Retrieves json type
  */
  property YYJSON_TYPE Type {
    public native get();
  }

  /**
  * Retrieves json subtype
  */
  property YYJSON_SUBTYPE SubType {
    public native get();
  }

  /**
  * Retrieves json is array
  */
  property bool IsArray {
    public native get();
  }

  /**
  * Retrieves json is object
  */
  property bool IsObject {
    public native get();
  }

  /**
  * Retrieves json is integer (uint64_t/int64_t)
  */
  property bool IsInt {
    public native get();
  }

  /**
  * Retrieves json is unsigned integer (uint64_t)
  */
  property bool IsUint {
    public native get();
  }

  /**
  * Retrieves json is signed integer (int64_t)
  */
  property bool IsSint {
    public native get();
  }

  /**
  * Retrieves json is number (uint64_t/int64_t/double)
  */
  property bool IsNum {
    public native get();
  }

  /**
  * Retrieves json is boolean
  */
  property bool IsBool {
    public native get();
  }

  /**
  * Retrieves json is true
  */
  property bool IsTrue {
    public native get();
  }

  /**
  * Retrieves json is false
  */
  property bool IsFalse {
    public native get();
  }

  /**
  * Retrieves json is float
  */
  property bool IsFloat {
    public native get();
  }

  /**
  * Retrieves json is string
  */
  property bool IsStr {
    public native get();
  }

  /**
  * Retrieves json is null
  */
  property bool IsNull {
    public native get();
  }

  /**
  * Retrieves json is container (array/object)
  */
  property bool IsCtn {
    public native get();
  }

  /**
  * Retrieves json is mutable doc
  */
  property bool IsMutable {
    public native get();
  }

  /**
  * Retrieves json is immutable doc
  */
  property bool IsImmutable {
    public native get();
  }

  /**
  * Retrieves read size of the JSON data
  *
  * @note This value reflects the size of the JSON data as read from the document
  *       - It does not auto update if the document is modified
  *       - For modified document, use GetSerializedSize to obtain the current size
  *
  */
  property bool ReadSize {
    public native get();
  }
};

methodmap YYJSONObject < YYJSON
{
  /**
  * Creates a JSON object A JSON object maps strings (called "keys") to values Keys in a
  * JSON object are unique That is, there is at most one entry in the map for a given key
  *
  * @note Needs to be freed using delete or CloseHandle()
  */
  public native YYJSONObject();

  /**
  * Creates a new JSON object from an array of strings representing key-value pairs.
  * The array must contain an even number of strings, where even indices are keys 
  * and odd indices are values. Keys cannot be empty strings.
  *
  * @param pairs             Array of strings containing alternating keys and values
  * @param size              Total size of the array (must be even)
  *
  * @return                  New JSON object handle
  * @error                   If array size is invalid, any key is empty, or if creation fails
  *
  */
  public static native YYJSONObject FromStrings(const char[][] pairs, int size);

  /**
  * Loads a JSON object from a file
  *
  * @param file              File to read from
  * @param flag              The JSON read options
  *
  * @return                  Object handle, or null on failure
  */
  public static native YYJSONObject FromFile(const char[] file, YYJSON_READ_FLAG flag = YYJSON_READ_NOFLAG);

  /**
  * Loads a JSON object from a string
  *
  * @param buffer            String buffer to load into the JSON object
  * @param flag              The JSON read options
  *
  * @return                  Object handle, or null on failure
  */
  public static native YYJSONObject FromString(const char[] buffer, YYJSON_READ_FLAG flag = YYJSON_READ_NOFLAG);

  /**
  * Gets a value from the object
  *
  * @note                    This function takes a linear search time
  *
  * @param key               Key name
  *
  * @return                  Returns the value to which the specified key is mapped, or null if this object contains no mapping for the key
  */
  public native any Get(const char[] key);

  /**
  * Sets a value in the object
  *
  * @param key               Key name
  * @param value             JSON handle to set
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool Set(const char[] key, const YYJSON value);

  /**
  * Gets a boolean value from the object
  *
  * @param key               Key name
  *
  * @return                  Boolean value
  */
  public native bool GetBool(const char[] key);

  /**
  * Gets a float value from the object
  *
  * @param key               Key name
  *
  * @return                  Float value
  */
  public native float GetFloat(const char[] key);

  /**
  * Gets a integer value from the object
  *
  * @param key               Key name
  *
  * @return                  Integer value
  */
  public native int GetInt(const char[] key);

  /**
  * Retrieves a 64-bit integer value from the object
  *
  * @param key               Key string
  * @param buffer            String buffer to store value
  * @param maxlength         Maximum length of the string buffer
  *
  * @return                  True on success, false if the key was not found
  */
  public native bool GetInt64(const char[] key, char[] buffer, int maxlength);

  /**
  * Gets name of the object's key
  *
  * @param index             Position from which get key name
  * @param buffer            Buffer to copy string to
  * @param maxlength         Maximum size of the buffer
  *
  * @return                  True on success, false on failure
  */
  public native bool GetKey(int index, char[] buffer, int maxlength);

  /**
  * Gets a value at the specified position from the object
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param index             Position from which get key name
  *
  * @return                  Returns the value to which index
  */
  public native any GetValueAt(int index);

  /**
  * Returns whether or not a key exists in the object
  *
  * @param key               Key string
  * @param ptr_use           Use JSON Pointer
  *
  * @return                  True if the key exists, false otherwise
  */
  public native bool HasKey(const char[] key, bool ptr_use = false);

  /**
  * Replaces all matching keys with the new key
  * The old_key and new_key should be a null-terminated UTF-8 string
  * The new_key is copied and held by doc
  *
  * @note                    This function takes a linear search time
  *
  * @param old_key           The key to rename
  * @param new_key           The new key name
  * @param allow_duplicate   Whether to allow renaming even if new key exists
  *
  * @return                  True if at least one key was renamed, false otherwise
  */
  public native bool RenameKey(const char[] old_key, const char[] new_key, bool allow_duplicate = false);

  /**
  * Gets string data from the object
  *
  * @param key               Key name
  * @param buffer            Buffer to copy string to
  * @param maxlength         Maximum size of the buffer
  *
  * @return                  True on success, false on failure
  */
  public native bool GetString(const char[] key, char[] buffer, int maxlength);

  /**
  * Returns whether or not a value in the object is null
  *
  * @param key               Key string
  *
  * @return                  True if the value is null, false otherwise
  */
  public native bool IsNull(const char[] key);

  /**
  * Sets a boolean value in the object
  *
  * @param key               Key name
  * @param value             Boolean value to set
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool SetBool(const char[] key, bool value);

  /**
  * Sets a float value in the object
  *
  * @param key               Key name
  * @param value             float to set
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool SetFloat(const char[] key, float value);

  /**
  * Sets a integer value in the object
  *
  * @param key               Key name
  * @param value             integer to set
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool SetInt(const char[] key, int value);

  /**
  * Sets a 64-bit integer value in the object, either inserting a new entry or replacing an old one
  *
  * @param key               Key string
  * @param value             Value to store at this key
  *
  * @return                  True on success, false on failure
  */
  public native bool SetInt64(const char[] key, const char[] value);

  /**
  * Sets a null in the object
  *
  * @param key               Key name
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool SetNull(const char[] key);

  /**
  * Sets string data in the object
  *
  * @param key               Key name
  * @param value             String to copy
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool SetString(const char[] key, const char[] value);

  /**
  * Removes a key and its value in the object
  *
  * @note                    This function takes a linear search time
  *
  * @param key               Key name
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool Remove(const char[] key);

  /**
  * Removes all keys and their values in the object
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool Clear();

  /**
  * Sorts the object's keys
  *
  * @note                    This function performs a lexicographical sort on the object's keys
  *                          - The values maintain their association with their respective keys
  *
  * @param order             Sort order, see YYJSON_SORT_ORDER enums
  *
  * @return                  True on success, false on failure
  * @error                   Invalid handle or handle is not an object
  */
  public native bool Sort(YYJSON_SORT_ORDER order = YYJSON_SORT_ASC);

  /**
  * Retrieves the size of the object
  */
  property int Size {
    public native get();
  }
};

methodmap YYJSONArray < YYJSON
{
  /**
  * Creates a JSON array
  *
  * @note Needs to be freed using delete or CloseHandle()
  */
  public native YYJSONArray();

  /**
  * Creates a new JSON array from an array of strings.
  *
  * @param strings           Array of strings to create array from.
  * @param size              Size of the array.
  * @return                  New JSON array handle, or null if creation failed.
  */
  public static native YYJSONArray FromStrings(const char[][] strings, int size);

  /**
  * Loads a JSON array from a file
  *
  * @param file              File to read from
  * @param flag              Read flag
  * @return                  Array handle, or null on failure
  */
  public static native YYJSONArray FromFile(const char[] file, YYJSON_READ_FLAG flag = YYJSON_READ_NOFLAG);

  /**
  * Loads a JSON array from a string
  *
  * @param buffer            String buffer to load into the JSON array
  * @param flag              Read flag
  * @return                  Array handle, or null on failure
  */
  public static native YYJSONArray FromString(const char[] buffer, YYJSON_READ_FLAG flag = YYJSON_READ_NOFLAG);

  /**
  * Gets a value from the array
  *
  * @note                    This function takes a linear search time
  *
  * @param index             Position in the array (starting from 0)
  *
  * @return                  Value handle on success, null if array is empty or index out of bounds
  */
  public native any Get(int index);

  /**
  * Gets a boolean value from the array
  *
  * @param index             Position in the array (starting from 0)
  *
  * @return                  Boolean value
  */
  public native bool GetBool(int index);

  /**
  * Gets a float value from the array
  *
  * @param index             Position in the array (starting from 0)
  *
  * @return                  The number as float
  */
  public native float GetFloat(int index);

  /**
  * Gets a integer value from the array
  *
  * @param index             Position in the array (starting from 0)
  *
  * @return                  integer value
  */
  public native int GetInt(int index);

  /**
  * Gets a 64-bit integer from the array
  *
  * @param index             Position in the array (starting from 0)
  * @param buffer            Buffer to copy to
  * @param maxlength         Maximum size of the buffer
  *
  * @return                  64-bit integer
  */
  public native void GetInt64(int index, char[] buffer, int maxlength);

  /**
  * Gets string data from the array
  *
  * @param index             Position in the array (starting from 0)
  * @param buffer            Buffer to copy string to
  * @param maxlength         Maximum size of the buffer
  *
  * @return                  True on success, false on failure
  */
  public native bool GetString(int index, char[] buffer, int maxlength);

  /**
  * Returns whether or not a value in the array is null
  *
  * @param index             Position in the array (starting from 0)
  *
  * @return                  True if the value is null, false otherwise
  */
  public native bool IsNull(int index);

  /**
  * Replaces a value at index
  *
  * @note                    This function takes a linear search time
  *
  * @param index             The index to which to replace the value
  * @param value             The new value to replace
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool Set(int index, const YYJSON value);

  /**
  * Replaces a boolean value at index
  *
  * @param index             The index to which to replace the value
  * @param value             The new boolean value to replace
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool SetBool(int index, bool value);

  /**
  * Replaces a float value at index
  *
  * @param index             The index to which to replace the value
  * @param value             The new float value to replace
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool SetFloat(int index, float value);

  /**
  * Replaces a integer value at index
  *
  * @param index             The index to which to replace the value
  * @param value             The new int value to replace
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool SetInt(int index, int value);

  /**
  * Replaces a intger64 value at index
  *
  * @param index             The index to which to replace the value
  * @param value             The new intger64 value to replace
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool SetInt64(int index, const char[] value);

  /**
  * Replaces a string value at index
  *
  * @param index             The index to which to replace the value
  * @param value             The new string value to replace
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool SetString(int index, const char[] value);

  /**
  * Replaces a null value at index
  *
  * @param index             The index to which to replace the value
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool SetNull(int index);

  /**
  * Inserts a value at the end of the array
  *
  * @param value             JSON handle to set
  *
  * @return                  The value to be inserted. Returns false if it is NULL
  */
  public native bool Push(const YYJSON value);

  /**
  * Inserts a boolean value at the end of the array
  *
  * @param value             Boolean value to set
  *
  * @return                  The value to be inserted. Returns false if it is NULL
  */
  public native bool PushBool(bool value);

  /**
  * Inserts a float value at the end of the array
  *
  * @param value             float to set
  *
  * @return                  The value to be inserted. Returns false if it is NULL
  */
  public native bool PushFloat(float value);

  /**
  * Inserts a integer value at the end of the array
  *
  * @param value             integer to set
  *
  * @return                  The value to be inserted. Returns false if it is NULL
  */
  public native bool PushInt(int value);

  /**
  * Inserts a intger64 value at the end of the array
  *
  * @param value             intger64 value
  *
  * @return                  The value to be inserted. Returns false if it is NULL
  */
  public native bool PushInt64(const char[] value);

  /**
  * Inserts a string value at the end of the array
  *
  * @param value             String to copy
  *
  * @return                  The value to be inserted. Returns false if it is NULL
  */
  public native bool PushString(const char[] value);

  /**
  * Inserts a null value at the end of the array
  *
  * @return                  The value to be inserted. Returns false if it is NULL
  */
  public native bool PushNull();

  /**
  * Removes an element from the array
  *
  * @note                    This function takes a linear search time
  *
  * @param index             Position in the array (starting from 0)
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool Remove(int index);

  /**
  * Removes the first value in this array
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool RemoveFirst();

  /**
  * Removes and returns the last value in this array
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool RemoveLast();

  /**
  * Removes all values within a specified range in the array
  *
  * @note                    This function takes a linear search time
  *
  * @param start_index       The start index of the range (0 is the first)
  * @param end_index         The number of items in the range (can be 0, but do nothing)
  *
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool RemoveRange(int start_index, int end_index);

  /**
  * Searches for a boolean value in the array and returns its index
  *
  * @param value             The boolean value to search for
  *
  * @return                  The index of the first matching element, or -1 if not found
  * @error                   Invalid handle or handle is not an array
  */
  public native int IndexOfBool(bool value);

  /**
  * Searches for a string value in the array and returns its index
  *
  * @param value             The string value to search for
  *
  * @return                  The index of the first matching element, or -1 if not found
  * @error                   Invalid handle or handle is not an array
  */
  public native int IndexOfString(const char[] value);

  /**
  * Searches for an integer value in the array and returns its index
  *
  * @param value             The integer value to search for
  *
  * @return                  The index of the first matching element, or -1 if not found
  * @error                   Invalid handle or handle is not an array
  */
  public native int IndexOfInt(int value);

  /**
  * Searches for an integer64 value in the array and returns its index
  *
  * @param value             The integer64 value to search for as string
  *
  * @return                  The index of the first matching element, or -1 if not found
  * @error                   Invalid handle, handle is not an array, or invalid integer64 string format
  */
  public native int IndexOfInt64(const char[] value);

  /**
  * Searches for a float value in the array and returns its index
  *
  * @param value             The float value to search for
  *
  * @return                  The index of the first matching element, or -1 if not found
  * @error                   Invalid handle or handle is not an array
  */
  public native int IndexOfFloat(float value);

  /**
  * Removes all elements from the array
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool Clear();

  /**
  * Sorts the array elements
  *
  * @note                    Sorting rules:
  *                          - Different types are sorted by their type ID
  *                          - Strings are sorted lexicographically
  *                          - Numbers are sorted by their numeric value
  *                          - Booleans are sorted with false before true
  *                          - Other types (null, object, array) are sorted by type only
  *
  * @param order             Sort order, see YYJSON_SORT_ORDER enums
  *
  * @return                  True on success, false on failure
  * @error                   Invalid handle or handle is not an array
  */
  public native bool Sort(YYJSON_SORT_ORDER order = YYJSON_SORT_ASC);

  /**
  * Retrieves the size of the array
  */
  property int Length {
    public native get();
  }

  /**
  * @note This function takes a linear search time
  * Returns the first element of this array
  * Returns NULL if arr is NULL/empty or type is not array
  * Needs to be freed using delete or CloseHandle()
  */
  property YYJSON First {
    public native get();
  }

  /**
  * @note This function takes a linear search time
  * Returns the last element of this array
  * Returns NULL if arr is NULL/empty or type is not array
  * Needs to be freed using delete or CloseHandle()
  */
  property YYJSON Last {
    public native get();
  }
};

public Extension __ext_yyjson = {
  name = "yyjson",
  file = "yyjson.ext",
#if defined AUTOLOAD_EXTENSIONS
  autoload = 1,
#else
  autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
  required = 1,
#else
  required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public void __pl_yyjson_SetNTVOptional()
{
  // JSONObject
  MarkNativeAsOptional("YYJSONObject.YYJSONObject");
  MarkNativeAsOptional("YYJSONObject.FromStrings");
  MarkNativeAsOptional("YYJSONObject.Size.get");
  MarkNativeAsOptional("YYJSONObject.Get");
  MarkNativeAsOptional("YYJSONObject.GetBool");
  MarkNativeAsOptional("YYJSONObject.GetFloat");
  MarkNativeAsOptional("YYJSONObject.GetInt");
  MarkNativeAsOptional("YYJSONObject.GetInt64");
  MarkNativeAsOptional("YYJSONObject.GetString");
  MarkNativeAsOptional("YYJSONObject.IsNull");
  MarkNativeAsOptional("YYJSONObject.GetKey");
  MarkNativeAsOptional("YYJSONObject.GetValueAt");
  MarkNativeAsOptional("YYJSONObject.HasKey");
  MarkNativeAsOptional("YYJSONObject.RenameKey");
  MarkNativeAsOptional("YYJSONObject.Set");
  MarkNativeAsOptional("YYJSONObject.SetBool");
  MarkNativeAsOptional("YYJSONObject.SetFloat");
  MarkNativeAsOptional("YYJSONObject.SetInt");
  MarkNativeAsOptional("YYJSONObject.SetInt64");
  MarkNativeAsOptional("YYJSONObject.SetNull");
  MarkNativeAsOptional("YYJSONObject.SetString");
  MarkNativeAsOptional("YYJSONObject.Remove");
  MarkNativeAsOptional("YYJSONObject.Clear");
  MarkNativeAsOptional("YYJSONObject.FromString");
  MarkNativeAsOptional("YYJSONObject.FromFile");
  MarkNativeAsOptional("YYJSONObject.Sort");

  // JSONArray
  MarkNativeAsOptional("YYJSONArray.YYJSONArray");
  MarkNativeAsOptional("YYJSONArray.FromStrings");
  MarkNativeAsOptional("YYJSONArray.Length.get");
  MarkNativeAsOptional("YYJSONArray.Get");
  MarkNativeAsOptional("YYJSONArray.First.get");
  MarkNativeAsOptional("YYJSONArray.Last.get");
  MarkNativeAsOptional("YYJSONArray.GetBool");
  MarkNativeAsOptional("YYJSONArray.GetFloat");
  MarkNativeAsOptional("YYJSONArray.GetInt");
  MarkNativeAsOptional("YYJSONArray.GetInt64");
  MarkNativeAsOptional("YYJSONArray.GetString");
  MarkNativeAsOptional("YYJSONArray.IsNull");
  MarkNativeAsOptional("YYJSONArray.Set");
  MarkNativeAsOptional("YYJSONArray.SetBool");
  MarkNativeAsOptional("YYJSONArray.SetFloat");
  MarkNativeAsOptional("YYJSONArray.SetInt");
  MarkNativeAsOptional("YYJSONArray.SetInt64");
  MarkNativeAsOptional("YYJSONArray.SetNull");
  MarkNativeAsOptional("YYJSONArray.SetString");
  MarkNativeAsOptional("YYJSONArray.Push");
  MarkNativeAsOptional("YYJSONArray.PushBool");
  MarkNativeAsOptional("YYJSONArray.PushFloat");
  MarkNativeAsOptional("YYJSONArray.PushInt");
  MarkNativeAsOptional("YYJSONArray.PushInt64");
  MarkNativeAsOptional("YYJSONArray.PushNull");
  MarkNativeAsOptional("YYJSONArray.PushString");
  MarkNativeAsOptional("YYJSONArray.Remove");
  MarkNativeAsOptional("YYJSONArray.RemoveFirst");
  MarkNativeAsOptional("YYJSONArray.RemoveLast");
  MarkNativeAsOptional("YYJSONArray.RemoveRange");
  MarkNativeAsOptional("YYJSONArray.Clear");
  MarkNativeAsOptional("YYJSONArray.FromString");
  MarkNativeAsOptional("YYJSONArray.FromFile");
  MarkNativeAsOptional("YYJSONArray.IndexOfBool");
  MarkNativeAsOptional("YYJSONArray.IndexOfString");
  MarkNativeAsOptional("YYJSONArray.IndexOfInt");
  MarkNativeAsOptional("YYJSONArray.IndexOfInt64");
  MarkNativeAsOptional("YYJSONArray.IndexOfFloat");
  MarkNativeAsOptional("YYJSONArray.Sort");

  // JSON
  MarkNativeAsOptional("YYJSON.ToString");
  MarkNativeAsOptional("YYJSON.ToFile");
  MarkNativeAsOptional("YYJSON.Parse");
  MarkNativeAsOptional("YYJSON.Equals");
  MarkNativeAsOptional("YYJSON.DeepCopy");
  MarkNativeAsOptional("YYJSON.GetTypeDesc");
  MarkNativeAsOptional("YYJSON.GetSerializedSize");
  MarkNativeAsOptional("YYJSON.ReadSize.get");
  MarkNativeAsOptional("YYJSON.Type.get");
  MarkNativeAsOptional("YYJSON.SubType.get");
  MarkNativeAsOptional("YYJSON.IsArray.get");
  MarkNativeAsOptional("YYJSON.IsObject.get");
  MarkNativeAsOptional("YYJSON.IsInt.get");
  MarkNativeAsOptional("YYJSON.IsUint.get");
  MarkNativeAsOptional("YYJSON.IsSint.get");
  MarkNativeAsOptional("YYJSON.IsNum.get");
  MarkNativeAsOptional("YYJSON.IsBool.get");
  MarkNativeAsOptional("YYJSON.IsTrue.get");
  MarkNativeAsOptional("YYJSON.IsFalse.get");
  MarkNativeAsOptional("YYJSON.IsFloat.get");
  MarkNativeAsOptional("YYJSON.IsStr.get");
  MarkNativeAsOptional("YYJSON.IsNull.get");
  MarkNativeAsOptional("YYJSON.IsCtn.get");
  MarkNativeAsOptional("YYJSON.IsMutable.get");
  MarkNativeAsOptional("YYJSON.IsImmutable.get");
  MarkNativeAsOptional("YYJSON.ForeachObject");
  MarkNativeAsOptional("YYJSON.ForeachArray");
  MarkNativeAsOptional("YYJSON.ForeachKey");
  MarkNativeAsOptional("YYJSON.ForeachIndex");
  MarkNativeAsOptional("YYJSON.ToMutable");
  MarkNativeAsOptional("YYJSON.ToImmutable");

  // JSON CREATE & GET
  MarkNativeAsOptional("YYJSON.Pack");
  MarkNativeAsOptional("YYJSON.CreateBool");
  MarkNativeAsOptional("YYJSON.CreateFloat");
  MarkNativeAsOptional("YYJSON.CreateInt");
  MarkNativeAsOptional("YYJSON.CreateInt64");
  MarkNativeAsOptional("YYJSON.CreateNull");
  MarkNativeAsOptional("YYJSON.CreateString");
  MarkNativeAsOptional("YYJSON.GetBool");
  MarkNativeAsOptional("YYJSON.GetFloat");
  MarkNativeAsOptional("YYJSON.GetInt");
  MarkNativeAsOptional("YYJSON.GetInt64");
  MarkNativeAsOptional("YYJSON.GetString");

  // JSON POINTER
  MarkNativeAsOptional("YYJSON.PtrGet");
  MarkNativeAsOptional("YYJSON.PtrGetBool");
  MarkNativeAsOptional("YYJSON.PtrGetFloat");
  MarkNativeAsOptional("YYJSON.PtrGetInt");
  MarkNativeAsOptional("YYJSON.PtrGetInt64");
  MarkNativeAsOptional("YYJSON.PtrGetString");
  MarkNativeAsOptional("YYJSON.PtrGetIsNull");
  MarkNativeAsOptional("YYJSON.PtrGetLength");
  MarkNativeAsOptional("YYJSON.PtrSet");
  MarkNativeAsOptional("YYJSON.PtrSetBool");
  MarkNativeAsOptional("YYJSON.PtrSetFloat");
  MarkNativeAsOptional("YYJSON.PtrSetInt");
  MarkNativeAsOptional("YYJSON.PtrSetInt64");
  MarkNativeAsOptional("YYJSON.PtrSetString");
  MarkNativeAsOptional("YYJSON.PtrSetNull");
  MarkNativeAsOptional("YYJSON.PtrAdd");
  MarkNativeAsOptional("YYJSON.PtrAddBool");
  MarkNativeAsOptional("YYJSON.PtrAddFloat");
  MarkNativeAsOptional("YYJSON.PtrAddInt");
  MarkNativeAsOptional("YYJSON.PtrAddInt64");
  MarkNativeAsOptional("YYJSON.PtrAddString");
  MarkNativeAsOptional("YYJSON.PtrAddNull");
  MarkNativeAsOptional("YYJSON.PtrRemove");
  MarkNativeAsOptional("YYJSON.PtrTryGetVal");
  MarkNativeAsOptional("YYJSON.PtrTryGetBool");
  MarkNativeAsOptional("YYJSON.PtrTryGetFloat");
  MarkNativeAsOptional("YYJSON.PtrTryGetInt");
  MarkNativeAsOptional("YYJSON.PtrTryGetInt64");
  MarkNativeAsOptional("YYJSON.PtrTryGetString");
}
#endif