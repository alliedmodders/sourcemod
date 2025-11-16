#if defined _json_included
  #endinput
#endif
#define _json_included

// JSON value types
enum JSON_TYPE
{
  JSON_TYPE_NONE = 0, // Invalid type
  JSON_TYPE_RAW  = 1, // Raw string (stored as is, used for number/literal)
  JSON_TYPE_NULL = 2, // null
  JSON_TYPE_BOOL = 3, // true/false
  JSON_TYPE_NUM  = 4, // Number (integer/real)
  JSON_TYPE_STR  = 5, // String
  JSON_TYPE_ARR  = 6, // Array
  JSON_TYPE_OBJ  = 7  // Object
}

// JSON value subtypes
enum JSON_SUBTYPE
{
  JSON_SUBTYPE_NONE  = 0 << 3, // Invalid subtype
  JSON_SUBTYPE_FALSE = 0 << 3, // Boolean false
  JSON_SUBTYPE_TRUE  = 1 << 3, // Boolean true
  JSON_SUBTYPE_UINT  = 0 << 3, // Unsigned integer
  JSON_SUBTYPE_SINT  = 1 << 3, // Signed integer
  JSON_SUBTYPE_REAL  = 2 << 3, // Real number (float/double)
  JSON_SUBTYPE_NOESC = 1 << 3  // String without escape character
}

// JSON reader flags for parsing behavior
enum JSON_READ_FLAG
{
  JSON_READ_NOFLAG                  = 0 << 0,  // Default behavior
  JSON_READ_INSITU                  = 1 << 0,  // Read JSON data in-situ (modify input string)
  JSON_READ_STOP_WHEN_DONE          = 1 << 1,  // Stop when done instead of issuing an error if there's additional content
  JSON_READ_ALLOW_TRAILING_COMMAS   = 1 << 2,  // Allow trailing commas at the end of arrays/objects
  JSON_READ_ALLOW_COMMENTS          = 1 << 3,  // Allow C-style comments (/* */) and line comments (//)
  JSON_READ_ALLOW_INF_AND_NAN       = 1 << 4,  // Allow nan/inf number (non-standard JSON)
  JSON_READ_NUMBER_AS_RAW           = 1 << 5,  // Read numbers as raw strings
  JSON_READ_ALLOW_INVALID_UNICODE   = 1 << 6,  // Allow invalid unicode when parsing string
  JSON_READ_BIGNUM_AS_RAW           = 1 << 7,  // Read big numbers as raw strings
  JSON_READ_ALLOW_BOM               = 1 << 8,  // Allow BOM (Byte Order Mark) at the beginning of the JSON string
  JSON_READ_ALLOW_EXT_NUMBER        = 1 << 9,  // Allow extended number (non-standard JSON)
  JSON_READ_ALLOW_EXT_ESCAPE        = 1 << 10, // Allow extended escape sequences in strings (non-standard)
  JSON_READ_ALLOW_EXT_WHITESPACE    = 1 << 11, // Allow extended whitespace characters (non-standard)
  JSON_READ_ALLOW_SINGLE_QUOTED_STR = 1 << 12, // Allow strings enclosed in single quotes (non-standard)
  JSON_READ_ALLOW_UNQUOTED_KEY      = 1 << 13, // Allow object keys without quotes (non-standard)
  JSON_READ_JSON5                   = JSON_READ_ALLOW_TRAILING_COMMAS |
                                        JSON_READ_ALLOW_COMMENTS |
                                        JSON_READ_ALLOW_INF_AND_NAN |
                                        JSON_READ_ALLOW_EXT_NUMBER |
                                        JSON_READ_ALLOW_EXT_ESCAPE |
                                        JSON_READ_ALLOW_EXT_WHITESPACE |
                                        JSON_READ_ALLOW_SINGLE_QUOTED_STR |
                                        JSON_READ_ALLOW_UNQUOTED_KEY // Allow JSON5 format, see: [https://json5.org]
}

// JSON writer flags for serialization behavior
enum JSON_WRITE_FLAG
{
  JSON_WRITE_NOFLAG                 = 0 << 0, // Default behavior
  JSON_WRITE_PRETTY                 = 1 << 0, // Pretty print with indent and newline
  JSON_WRITE_ESCAPE_UNICODE         = 1 << 1, // Escape unicode as \uXXXX
  JSON_WRITE_ESCAPE_SLASHES         = 1 << 2, // Escape '/' as '\/'
  JSON_WRITE_ALLOW_INF_AND_NAN      = 1 << 3, // Write inf/nan number (non-standard JSON)
  JSON_WRITE_INF_AND_NAN_AS_NULL    = 1 << 4, // Write inf/nan as null
  JSON_WRITE_ALLOW_INVALID_UNICODE  = 1 << 5, // Allow invalid unicode when encoding string
  JSON_WRITE_PRETTY_TWO_SPACES      = 1 << 6, // Use 2 spaces for indent when pretty print
  JSON_WRITE_NEWLINE_AT_END         = 1 << 7, // Add newline at the end of output
  JSON_WRITE_FP_TO_FLOAT            = 1 << 27 // Write floating-point numbers using single-precision (float)
}

/**
 * Write floating-point number using fixed-point notation
 * This is similar to ECMAScript Number.prototype.toFixed(prec) but with trailing zeros removed
 *
 * @param n    Precision digits (1-7)
 * @return     JSON write flag with precision setting
 * @note       This will produce shorter output but may lose some precision
 */
stock JSON_WRITE_FLAG JSON_WRITE_FP_TO_FIXED(int n)
{
  n = (n < 1) ? 1 : (n > 7 ? 7 : n);
  return view_as<JSON_WRITE_FLAG>(n << 28);
}

// Sort order for arrays and objects
enum JSON_SORT_ORDER
{
  JSON_SORT_ASC    = 0, // Ascending order (default)
  JSON_SORT_DESC   = 1, // Descending order
  JSON_SORT_RANDOM = 2  // Random order
}

methodmap JSON < Handle
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
  * @note                    Needs to be freed using delete or CloseHandle()
  * @note                    There is a limit of 32 parameters (defined in SourcePawn)
  *                          including the format string, so the number of arguments must be less than or equal to 31
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
  * @deprecated              Use JSONObjIter instead
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
  #pragma deprecated Use JSONObjIter instead.
  public native bool ForeachObject(char[] buffer, int maxlength, JSON &value);

  /**
  * Iterates over the array's values
  *
  * @deprecated              Use JSONArrIter instead
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param index             Variable to store current array index (starting from 0)
  * @param value             JSON handle to store the current value
  *
  * @return                  True if there are more elements, false when iteration is complete
  * @error                   Invalid handle or handle is not an array
  */
  #pragma deprecated Use JSONArrIter instead.
  public native bool ForeachArray(int &index, JSON &value);

  /**
  * Same as ForeachObject, but only iterates over the object's keys
  *
  * @deprecated              Use JSONObjIter instead
  *
  * @note                    Use this when you only need keys (faster than ForeachObject since it doesn't create value handles)
  *
  * @param buffer            Buffer to copy key name to
  * @param maxlength         Maximum length of the string buffer
  *
  * @return                  True if there are more elements, false when iteration is complete
  * @error                   Invalid handle or handle is not an object
  */
  #pragma deprecated Use JSONObjIter instead.
  public native bool ForeachKey(char[] buffer, int maxlength);

  /**
  * Same as ForeachArray, but only iterates over the array's indexes
  *
  * @deprecated              Use JSONArrIter instead
  *
  * @note                    Use this when you only need indexes (faster than ForeachArray since it doesn't create value handles)
  *
  * @param index             Variable to store current array index (starting from 0)
  *
  * @return                  True if there are more elements, false when iteration is complete
  * @error                   Invalid handle or handle is not an array
  */
  #pragma deprecated Use JSONArrIter instead.
  public native bool ForeachIndex(int &index);

  /**
  * Converts an immutable JSON document to a mutable one
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @return                  Handle to the new mutable JSON document, INVALID_HANDLE on failure
  * @error                   If the document is already mutable
  */
  public native any ToMutable();

  /**
  * Converts a mutable JSON document to an immutable one
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @return                  Handle to the new immutable JSON document, INVALID_HANDLE on failure
  * @error                   If the document is already immutable
  */
  public native any ToImmutable();

  /**
   * Apply a JSON Patch (RFC 6902) to this value and return a new JSON handle
   *
   * @note                    Needs to be freed using delete or CloseHandle()
   *
   * @param patch             JSON Patch document
   * @param resultMutable     True to return a mutable result, false for immutable
   *
   * @return                  New JSON handle on success
   * @error                   Throws if the patch cannot be applied
   */
  public native any ApplyJsonPatch(const JSON patch, bool resultMutable = false);

  /**
   * Apply a JSON Patch (RFC 6902) to this value in place
   *
   * @param patch             JSON Patch document
   *
   * @return                  True on success, false otherwise
   * @error                   Throws if this value is immutable or patch failed
   */
  public native bool JsonPatchInPlace(const JSON patch);

  /**
   * Apply a JSON Merge Patch (RFC 7396) to this value and return a new JSON handle
   *
   * @note                    Needs to be freed using delete or CloseHandle()
   *
   * @param patch             JSON Merge Patch document
   * @param resultMutable     True to return a mutable result, false for immutable
   *
   * @return                  New JSON handle on success
   * @error                   Throws if the merge patch cannot be applied
   */
  public native any ApplyMergePatch(const JSON patch, bool resultMutable = false);

  /**
   * Apply a JSON Merge Patch (RFC 7396) to this value in place
   *
   * @param patch             JSON Merge Patch document
   *
   * @return                  True on success, false otherwise
   * @error                   Throws if this value is immutable or merge patch failed
   */
  public native bool MergePatchInPlace(const JSON patch);

  /**
  * Write a document to JSON file with options
  *
  * @note                    On 32-bit operating system, files larger than 2GB may fail to write
  *
  * @param file              The JSON file's path. If this path is null or invalid, the function will fail and return false.
  *                          If this file is not empty, the content will be discarded
  * @param flag              The JSON write options
  *
  * @return                  True on success, false on failure
  */
  public native bool ToFile(const char[] file, JSON_WRITE_FLAG flag = JSON_WRITE_NOFLAG);

  /**
  * Write a value to JSON string
  *
  * @param buffer            String buffer to write to
  * @param maxlength         Maximum length of the string buffer
  * @param flag              The JSON write options
  *
  * @return                  Number of characters written to the buffer (including null terminator) or 0 on failure
  */
  public native int ToString(char[] buffer, int maxlength, JSON_WRITE_FLAG flag = JSON_WRITE_NOFLAG);

  /**
  * Write a JSON number value to string buffer
  *
  * @note                    The buffer must be large enough to hold the number string (at least 40 bytes for floating-point, 21 bytes for integer)
  *
  * @param buffer            String buffer to write to
  * @param maxlength         Maximum length of the string buffer
  * @param written           Variable to store number of characters written (excluding null terminator) (optional)
  *
  * @return                  True on success, false on failure
  * @error                   Invalid handle, handle is not a number, or buffer too small
  */
  public native bool WriteNumber(char[] buffer, int maxlength, int &written = 0);

  /**
  * Set floating-point number's output format to single-precision
  *
  * @note                    Only works on floating-point numbers (not integers)
  * @note                    This affects how the number is serialized in all write operations
  *
  * @param flt               True to use single-precision (float), false to use double-precision (double)
  *
  * @return                  True on success, false if handle is not a floating-point number
  * @error                   Invalid handle or handle is not a floating-point number
  */
  public native bool SetFpToFloat(bool flt);

  /**
  * Set floating-point number's output format to fixed-point notation
  *
  * @note                    Only works on floating-point numbers (not integers)
  * @note                    This is similar to ECMAScript Number.prototype.toFixed(prec) but with trailing zeros removed
  * @note                    This will produce shorter output but may lose some precision
  * @note                    This affects how the number is serialized in all write operations
  *
  * @param prec              Precision (1-7)
  *
  * @return                  True on success, false if handle is not a floating-point number or prec is out of range
  * @error                   Invalid handle, handle is not a floating-point number, or precision out of range (1-7)
  */
  public native bool SetFpToFixed(int prec);

  /**
  * Directly modify a JSON value to boolean type
  *
  * @note                    This modifies the value in-place without creating a new value
  * @note                    For immutable documents, this breaks immutability. Use with caution
  *
  * @param value             Boolean value
  *
  * @return                  True on success, false if value is object or array
  * @error                   Invalid handle or handle is object or array
  */
  public native bool SetBool(bool value);

  /**
  * Directly modify a JSON value to integer type
  *
  * @note                    This modifies the value in-place without creating a new value
  * @note                    For immutable documents, this breaks immutability. Use with caution
  *
  * @param value             Integer value
  *
  * @return                  True on success, false if value is object or array
  * @error                   Invalid handle or handle is object or array
  */
  public native bool SetInt(int value);

  /**
  * Directly modify a JSON value to 64-bit integer type
  *
  * @note                    This modifies the value in-place without creating a new value
  * @note                    For immutable documents, this breaks immutability. Use with caution
  * @note                    This function auto-detects whether the value is signed or unsigned
  *
  * @param value             64-bit integer value (as string, can be signed or unsigned)
  *
  * @return                  True on success, false if value is object or array
  * @error                   Invalid handle, handle is object or array, or invalid int64 string format
  */
  public native bool SetInt64(const char[] value);

  /**
  * Directly modify a JSON value to floating-point type
  *
  * @note                    This modifies the value in-place without creating a new value
  * @note                    For immutable documents, this breaks immutability. Use with caution
  *
  * @param value             Float value
  *
  * @return                  True on success, false if value is object or array
  * @error                   Invalid handle or handle is object or array
  */
  public native bool SetFloat(float value);

  /**
  * Directly modify a JSON value to string type
  *
  * @note                    This modifies the value in-place without creating a new value
  * @note                    For immutable documents, this breaks immutability and does NOT copy the string. Use with caution
  *
  * @param value             String value
  *
  * @return                  True on success, false if value is object or array or value is null
  * @error                   Invalid handle or handle is object or array
  */
  public native bool SetString(const char[] value);

  /**
  * Directly modify a JSON value to null type
  *
  * @note                    This modifies the value in-place without creating a new value
  * @note                    For immutable documents, this breaks immutability. Use with caution
  *
  * @return                  True on success, false if value is object or array
  * @error                   Invalid handle or handle is object or array
  */
  public native bool SetNull();

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
  public static native any Parse(const char[] string, bool is_file = false, bool is_mutable_doc = false, JSON_READ_FLAG flag = JSON_READ_NOFLAG);

  /**
  * Read a JSON number from string
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  * @note                    The input string must be null-terminated UTF-8 without BOM
  * @note                    The returned value is a mutable number value
  *
  * @param dat               The JSON data (UTF-8 without BOM), null-terminator is required
  * @param flag              Read flags (JSON_READ_FLAG values, default: JSON_READ_NOFLAG)
  * @param consumed          Variable to store number of characters consumed (optional)
  *
  * @return                  New JSON number value or null on error
  * @error                   If input data is invalid or number parsing fails
  */
  public static native JSON ReadNumber(const char[] dat, JSON_READ_FLAG flag = JSON_READ_NOFLAG, int &consumed = 0);

  /**
  * Creates a deep copy of a JSON value and returns it as a new JSON handle
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
  public static native any DeepCopy(const JSON targetDoc, const JSON sourceValue);

  /**
  * Returns the JSON value's type description
  *
  * @param buffer            String buffer to write to
  * @param maxlength         Maximum length of the string buffer
  *
  * @return                  The return value should be one of these strings: "raw", "null", "string",
  *                          "array", "object", "true", "false", "uint", "sint", "real", "unknown"
  */
  public native void GetTypeDesc(char[] buffer, int maxlength);

  /**
  * Returns whether two JSON values are equal (deep compare)
  *
  * @note                    This function is recursive and may cause a stack overflow if the object level is too deep
  * @note                    the result may be inaccurate if object has duplicate keys
  *
  * @param value1            First JSON value to compare
  * @param value2            Second JSON value to compare
  *
  * @return                  True if they are the same, false otherwise
  */
  public static native bool Equals(const JSON value1, const JSON value2);

  /**
  * Check if JSON value equals a string
  *
  * @param value              JSON handle
  * @param str                String to compare with
  *
  * @return                   True if value is a string and equals the given string, false otherwise
  */
  public static native bool EqualsStr(const JSON value, const char[] str);

  /**
  * Creates and returns a boolean value
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param value             The boolean value to be set
  *
  * @return                  JSON handle, null on error
  */
  public static native JSON CreateBool(bool value);

  /**
  * Creates and returns a float value
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param value             The float value to be set
  *
  * @return                  JSON handle, null on error
  */
  public static native JSON CreateFloat(float value);

  /**
  * Creates and returns an int value
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param value             The int value to be set
  *
  * @return                  JSON handle, null on error
  */
  public static native JSON CreateInt(int value);

  /**
  * Creates and returns an integer64 value
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  * @note                    This function auto-detects whether the value is signed or unsigned
  *
  * @param value             The integer64 value to be set (can be signed or unsigned)
  *
  * @return                  JSON handle, null on error
  */
  public static native JSON CreateInt64(const char[] value);

  /**
  * Creates and returns a string value
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param value             The string value to be set
  *
  * @return                  JSON handle, null on error
  */
  public static native JSON CreateString(const char[] value);

  /**
  * Creates and returns a null value
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @return                  JSON handle, null on error
  */
  public static native JSON CreateNull();

  /**
  * Get boolean value
  *
  * @return                  Boolean value
  */
  public native bool GetBool();

  /**
  * Get float value
  *
  * @return                  float value
  */
  public native float GetFloat();

  /**
  * Get int value
  *
  * @return                  int value
  */
  public native int GetInt();

  /**
   * Get integer64 value (auto-detects signed/unsigned)
   *
   * @param buffer            Buffer to copy to
   * @param maxlength         Maximum size of the buffer
   *
   * @return                  True on success, false on failure
   */
  public native bool GetInt64(char[] buffer, int maxlength);

  /**
  * Get string value
  *
  * @param buffer            Buffer to copy to
  * @param maxlength         Maximum size of the buffer
  *
  * @return                  True on success, false on failure
  */
  public native bool GetString(char[] buffer, int maxlength);

  /**
   * Get JSON Handle serialized size in bytes (including null-terminator)
   *
   * @param flag              The JSON write options
   *
   * @return                  Size in bytes (including null terminator)
   *
   * @note                    The returned size depends on the flag parameter.
   *                          You MUST use the same flags when calling both GetSerializedSize()
   *                          and ToString(). Using different flags will return different sizes
   *                          and may cause buffer overflow
   */
  public native int GetSerializedSize(JSON_WRITE_FLAG flag = JSON_WRITE_NOFLAG);

  /**
  * Get value by a JSON Pointer
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  *
  * @param path              The JSON pointer string
  *
  * @return                  The value referenced by the JSON pointer
  */
  public native any PtrGet(const char[] path);

  /**
  * Get boolean value by a JSON Pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  *
  * @param path              The JSON pointer string
  *
  * @return                  boolean value referenced by the JSON pointer
  */
  public native bool PtrGetBool(const char[] path);

  /**
  * Get float value by a JSON Pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  *
  * @param path              The JSON pointer string
  *
  * @return                  float value referenced by the JSON pointer
  */
  public native float PtrGetFloat(const char[] path);

  /**
  * Get integer value by a JSON Pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  *
  * @param path              The JSON pointer string
  *
  * @return                  integer value referenced by the JSON pointer
  */
  public native int PtrGetInt(const char[] path);

  /**
   * Get integer64 value by a JSON Pointer (auto-detects signed/unsigned)
   *
   * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
   *
   * @param path              The JSON pointer string
   * @param buffer            Buffer to copy to
   * @param maxlength         Maximum size of the buffer
   *
   * @return                  True on success, false on failure
   */
  public native bool PtrGetInt64(const char[] path, char[] buffer, int maxlength);

  /**
  * Get string value by a JSON Pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  *
  * @param path              The JSON pointer string
  * @param buffer            Buffer to copy to
  * @param maxlength         Maximum size of the buffer
  *
  * @return                  True on success, false on failure
  */
  public native bool PtrGetString(const char[] path, char[] buffer, int maxlength);

  /**
  * Get value is null by a JSON Pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  *
  * @param path              The JSON pointer string
  *
  * @return                  True if the value is null, false otherwise
  */
  public native bool PtrGetIsNull(const char[] path);

  /**
  * Get JSON content length (string length, array size, object size)
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  * @note                    For strings: returns string length including null-terminator
  * @note                    For arrays/objects: returns number of elements
  * @note                    Returns 0 if value is null or type is not string/array/object
  *
  * @param path              The JSON pointer string
  *
  * @return                  JSON content length
  */
  public native int PtrGetLength(const char[] path);

  /**
  * Set value by a JSON Pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  * @note                    The parent nodes will be created if they do not exist.
  *                          If the target value already exists, it will be replaced by the new value
  *
  * @param path              The JSON pointer string
  * @param value             The value to be set
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrSet(const char[] path, JSON value);

  /**
  * Set boolean value by a JSON Pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  * @note                    The parent nodes will be created if they do not exist.
  *                          If the target value already exists, it will be replaced by the new value
  *
  * @param path              The JSON pointer string
  * @param value             The boolean value to be set
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrSetBool(const char[] path, bool value);

  /**
  * Set float value by a JSON Pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  * @note                    The parent nodes will be created if they do not exist.
  *                          If the target value already exists, it will be replaced by the new value
  *
  * @param path              The JSON pointer string
  * @param value             The float value to be set
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrSetFloat(const char[] path, float value);

  /**
  * Set integer value by a JSON Pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  * @note                    The parent nodes will be created if they do not exist.
  *                          If the target value already exists, it will be replaced by the new value
  *
  * @param path              The JSON pointer string
  * @param value             The integer value to be set
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrSetInt(const char[] path, int value);

  /**
  * Set integer64 value by a JSON Pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  * @note                    The parent nodes will be created if they do not exist.
  *                          If the target value already exists, it will be replaced by the new value
  * @note                    This function auto-detects whether the value is signed or unsigned
  *
  * @param path              The JSON pointer string
  * @param value             The integer64 value to be set (can be signed or unsigned)
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrSetInt64(const char[] path, const char[] value);

  /**
  * Set string value by a JSON Pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  * @note                    The parent nodes will be created if they do not exist.
  *                          If the target value already exists, it will be replaced by the new value
  *
  * @param path              The JSON pointer string
  * @param value             The string value to be set
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrSetString(const char[] path, const char[] value);

  /**
  * Set null value by a JSON Pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  * @note                    The parent nodes will be created if they do not exist.
  *                          If the target value already exists, it will be replaced by the new value
  *
  * @param path              The JSON pointer string
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrSetNull(const char[] path);

  /**
  * Add (insert) value by a JSON pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  *
  * @param path              The JSON pointer string
  * @param value             The value to be added
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrAdd(const char[] path, JSON value);

  /**
  * Add (insert) boolean value by a JSON pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
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
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
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
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
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
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  *
  * @param path              The JSON pointer string
  * @param value             The integer64 value to be added
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrAddInt64(const char[] path, const char[] value);

  /**
  * Add (insert) string value by a JSON pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
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
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  *
  * @param path              The JSON pointer string
  *
  * @return                  true if JSON pointer is valid and new value is set, false otherwise
  */
  public native bool PtrAddNull(const char[] path);

  /**
  * Remove value by a JSON pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  *
  * @param path              The JSON pointer string
  *
  * @return                  true if removed value, false otherwise
  */
  public native bool PtrRemove(const char[] path);

  /**
  * Try to get value by a JSON Pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  *
  * @param path              The JSON pointer string
  * @param value             Handle to store value
  *
  * @return                  true on success, false otherwise
  */
  public native bool PtrTryGetVal(const char[] path, JSON &value);

  /**
  * Try to get boolean value by a JSON Pointer
  *
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
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
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
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
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
  *
  * @param path              The JSON pointer string
  * @param value             Store the integer value
  *
  * @return                  true on success, false otherwise
  */
  public native bool PtrTryGetInt(const char[] path, int &value);

  /**
   * Try to get integer64 value by a JSON Pointer (automatically detects signed/unsigned)
   *
   * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
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
  * @note                    JSON Pointer paths are always resolved from the document root, not from the current value
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
  property JSON_TYPE Type {
    public native get();
  }

  /**
  * Retrieves json subtype
  */
  property JSON_SUBTYPE SubType {
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
  * Retrieves the size of the JSON data as it was originally read from parsing
  *
  * @return                  Size in bytes (including null terminator) or 0 if not from parsing
  *
  * @note                    This value only applies to documents created from parsing (Parse, FromString, FromFile)
  * @note                    Manually created documents (new JSONObject(), JSON.CreateBool(), etc.) will return 0
  * @note                    This value does not auto-update if the document is modified
  * @note                    For modified documents, use GetSerializedSize() to obtain the current size
  */
  property int ReadSize {
    public native get();
  }

  /**
  * Get the reference count of the document (debugging purpose)
  *
  * @return                  Reference count (number of JSON objects sharing this document)
  *
  * @note                    Returns 0 if the handle is invalid
  */
  property int RefCount {
    public native get();
  }
};

methodmap JSONObject < JSON
{
  /**
  * Creates a JSON object A JSON object maps strings (called "keys") to values Keys in a
  * JSON object are unique That is, there is at most one entry in the map for a given key
  *
  * @note Needs to be freed using delete or CloseHandle()
  */
  public native JSONObject();

  /**
  * Creates a new JSON object from an array of strings representing key-value pairs.
  * The array must contain an even number of strings, where even indices are keys
  * and odd indices are values. Keys cannot be empty strings
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param pairs             Array of strings containing alternating keys and values
  * @param size              Total size of the array (must be even)
  *
  * @return                  New JSON object handle
  * @error                   If array size is invalid, any key is empty, or if creation fails
  *
  */
  public static native JSONObject FromStrings(const char[][] pairs, int size);

  /**
  * Loads a JSON object from a file
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param file              File to read from
  * @param flag              The JSON read options
  *
  * @return                  Object handle, or null on failure
  */
  public static native JSONObject FromFile(const char[] file, JSON_READ_FLAG flag = JSON_READ_NOFLAG);

  /**
  * Loads a JSON object from a string
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param buffer            String buffer to load into the JSON object
  * @param flag              The JSON read options
  *
  * @return                  Object handle, or null on failure
  */
  public static native JSONObject FromString(const char[] buffer, JSON_READ_FLAG flag = JSON_READ_NOFLAG);

  /**
  * Gets a value from the object
  *
  * @note                    Needs to be freed using delete or CloseHandle()
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
  public native bool Set(const char[] key, const JSON value);

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
   * Retrieves a 64-bit integer value from the object (automatically detects signed/unsigned)
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
  * @note                    This function automatically detects whether the value is signed or unsigned
  *
  * @param key               Key string
  * @param value             Value to store at this key (can be signed or unsigned)
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
  * @param order             Sort order, see JSON_SORT_ORDER enums
  *
  * @return                  True on success, false on failure
  * @error                   Invalid handle or handle is not an object
  */
  public native bool Sort(JSON_SORT_ORDER order = JSON_SORT_ASC);

  /**
  * Retrieves the size of the object
  */
  property int Size {
    public native get();
  }
};

methodmap JSONArray < JSON
{
  /**
  * Creates a JSON array
  *
  * @note Needs to be freed using delete or CloseHandle()
  */
  public native JSONArray();

  /**
  * Creates a new JSON array from an array of strings
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param strings           Array of strings to create array from
  * @param size              Size of the array
  * @return                  New JSON array handle, or null if creation failed
  */
  public static native JSONArray FromStrings(const char[][] strings, int size);

  /**
  * Creates a new JSON array from an array of integer values
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param values            Array of int values to create array from
  * @param size              Size of the array
  * @return                  New JSON array handle, or null if creation failed
  */
  public static native JSONArray FromInt(const int[] values, int size);

  /**
  * Creates a new JSON array from an array of 64-bit integer strings (auto-detects signed/unsigned)
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param values            Array of int64 string values (can be signed or unsigned)
  * @param size              Size of the array
  * @return                  New JSON array handle, or null if creation failed
  * @error                   If any value cannot be parsed as int64
  */
  public static native JSONArray FromInt64(const char[][] values, int size);

  /**
  * Creates a new JSON array from an array of boolean values
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param values            Array of bool values to create array from
  * @param size              Size of the array
  * @return                  New JSON array handle, or null if creation failed
  */
  public static native JSONArray FromBool(const bool[] values, int size);

  /**
  * Creates a new JSON array from an array of float values
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param values            Array of float values to create array from
  * @param size              Size of the array
  * @return                  New JSON array handle, or null if creation failed
  */
  public static native JSONArray FromFloat(const float[] values, int size);

  /**
  * Loads a JSON array from a file
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param file              File to read from
  * @param flag              Read flag
  * @return                  Array handle, or null on failure
  */
  public static native JSONArray FromFile(const char[] file, JSON_READ_FLAG flag = JSON_READ_NOFLAG);

  /**
  * Loads a JSON array from a string
  *
  * @note                    Needs to be freed using delete or CloseHandle()
  *
  * @param buffer            String buffer to load into the JSON array
  * @param flag              Read flag
  * @return                  Array handle, or null on failure
  */
  public static native JSONArray FromString(const char[] buffer, JSON_READ_FLAG flag = JSON_READ_NOFLAG);

  /**
  * Gets a value from the array
  *
  * @note                    Needs to be freed using delete or CloseHandle()
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
   * Gets a 64-bit integer from the array (automatically detects signed/unsigned)
   *
   * @param index             Position in the array (starting from 0)
   * @param buffer            Buffer to copy to
   * @param maxlength         Maximum size of the buffer
   *
   * @return                  True on success, false on failure
   */
  public native bool GetInt64(int index, char[] buffer, int maxlength);

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
  public native bool Set(int index, const JSON value);

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
  * Replaces an integer value at index
  *
  * @param index             The index to which to replace the value
  * @param value             The new int value to replace
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool SetInt(int index, int value);

  /**
  * Replaces an integer64 value at index
  *
  * @note                    This function automatically detects whether the value is signed or unsigned
  *
  * @param index             The index to which to replace the value
  * @param value             The new integer64 value to replace (can be signed or unsigned)
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
  * @return                  True if succeed, false otherwise
  */
  public native bool Push(const JSON value);

  /**
  * Inserts a boolean value at the end of the array
  *
  * @param value             Boolean value to set
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool PushBool(bool value);

  /**
  * Inserts a float value at the end of the array
  *
  * @param value             float to set
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool PushFloat(float value);

  /**
  * Inserts an integer value at the end of the array
  *
  * @param value             integer to set
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool PushInt(int value);

  /**
  * Inserts an integer64 value at the end of the array
  *
  * @param value             integer64 value
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool PushInt64(const char[] value);

  /**
  * Inserts a string value at the end of the array
  *
  * @param value             String to copy
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool PushString(const char[] value);

  /**
  * Inserts a null value at the end of the array
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool PushNull();

  /**
  * Inserts a JSON value at specific index
  *
  * @note                    This function takes a linear search time
  *
  * @param index             Position to insert (0 to size, size means append)
  * @param value             JSON handle to insert
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool Insert(int index, const JSON value);

  /**
  * Inserts a boolean value at specific index
  *
  * @note                    This function takes a linear search time
  *
  * @param index             Position to insert
  * @param value             Boolean value
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool InsertBool(int index, bool value);

  /**
  * Inserts an integer value at specific index
  *
  * @note                    This function takes a linear search time
  *
  * @param index             Position to insert
  * @param value             Integer value
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool InsertInt(int index, int value);

  /**
  * Inserts an integer64 value at specific index
  *
  * @note                    This function takes a linear search time
  *
  * @param index             Position to insert
  * @param value             Integer64 value (as string)
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool InsertInt64(int index, const char[] value);

  /**
  * Inserts a float value at specific index
  *
  * @note                    This function takes a linear search time
  *
  * @param index             Position to insert
  * @param value             Float value
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool InsertFloat(int index, float value);

  /**
  * Inserts a string value at specific index
  *
  * @note                    This function takes a linear search time
  *
  * @param index             Position to insert
  * @param value             String value
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool InsertString(int index, const char[] value);

  /**
  * Inserts a null value at specific index
  *
  * @note                    This function takes a linear search time
  *
  * @param index             Position to insert
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool InsertNull(int index);

  /**
  * Prepends a JSON value to the beginning of the array
  *
  * @param value             JSON handle to prepend
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool Prepend(const JSON value);

  /**
  * Prepends a boolean value to the beginning of the array
  *
  * @param value             Boolean value
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool PrependBool(bool value);

  /**
  * Prepends an integer value to the beginning of the array
  *
  * @param value             Integer value
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool PrependInt(int value);

  /**
  * Prepends an integer64 value to the beginning of the array
  *
  * @param value             Integer64 value (as string)
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool PrependInt64(const char[] value);

  /**
  * Prepends a float value to the beginning of the array
  *
  * @param value             Float value
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool PrependFloat(float value);

  /**
  * Prepends a string value to the beginning of the array
  *
  * @param value             String value
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool PrependString(const char[] value);

  /**
  * Prepends a null value to the beginning of the array
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool PrependNull();

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
  * Removes the last value in this array
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
  * @param count             Number of items to remove (can be 0, in which case nothing happens)
  *
  * @return                  True if succeed, false otherwise
  */
  public native bool RemoveRange(int start_index, int count);

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
  * @param order             Sort order, see JSON_SORT_ORDER enums
  *
  * @return                  True on success, false on failure
  * @error                   Invalid handle or handle is not an array
  */
  public native bool Sort(JSON_SORT_ORDER order = JSON_SORT_ASC);

  /**
  * Retrieves the size of the array
  */
  property int Length {
    public native get();
  }

  /**
  * @note This function takes a linear search time
  * Returns the first element of this array
  * Returns null if arr is null/empty or type is not array
  * Needs to be freed using delete or CloseHandle()
  */
  property JSON First {
    public native get();
  }

  /**
  * @note This function takes a linear search time
  * Returns the last element of this array
  * Returns null if arr is null/empty or type is not array
  * Needs to be freed using delete or CloseHandle()
  */
  property JSON Last {
    public native get();
  }
};

methodmap JSONArrIter < Handle
{
  /**
   * Creates an array iterator from a JSON array value
   *
   * @note                    Needs to be freed using delete or CloseHandle()
   * @note                    Iterators are single-pass. Once Next returns null, call Reset() or create
   *                          a new iterator to traverse the array again
   *
   * @param array             JSON array value to iterate
   *
   * @return                  Array iterator handle, or null on failure
   * @error                   Invalid handle or handle is not an array
   */
  public native JSONArrIter(JSON array);

  /**
   * Resets the iterator to the beginning of the array
   *
   * @note                    Only resets the iterator, does not free the iterator handle
   *
   * @return                  True on success, false on failure
   * @error                   Invalid iterator handle
   */
  public native bool Reset();

  /**
   * Moves the iterator to the next element
   *
   * @note                    Needs to be freed using delete or CloseHandle()
   *
   * @return                  JSON value handle for the next element, or null if iteration is complete
   * @error                   Invalid iterator handle
   */
  property JSON Next {
    public native get();
  }

  /**
   * Checks if there are more elements to iterate
   *
   * @return                  True if there are more elements, false otherwise
   * @error                   Invalid iterator handle
   */
  property bool HasNext {
    public native get();
  }

  /**
   * Gets the current index in the array iteration
   *
   * @return                  Current index (0-based), or -1 if iterator is not positioned
   * @error                   Invalid iterator handle
   */
  property int Index {
    public native get();
  }

  /**
   * Removes the current element from the array (mutable iterators only)
   *
   * @note                    This function can only be called on mutable iterators
   * @note                    After calling Remove(), the iterator should not be used to continue iteration
   *
   * @return                  True on success, false on failure
   * @error                   Invalid iterator handle or iterator is not mutable
   */
  public native bool Remove();
};

methodmap JSONObjIter < Handle
{
  /**
   * Creates an object iterator from a JSON object value
   *
   * @note                    Needs to be freed using delete or CloseHandle()
   * @note                    Iterators are single-pass. Once Next returns null, call Reset() or create
   *                          a new iterator to traverse the object again
   *
   * @param obj               JSON object value to iterate
   *
   * @return                  Object iterator handle, or null on failure
   * @error                   Invalid handle or handle is not an object
   */
  public native JSONObjIter(JSON obj);

  /**
   * Resets the iterator to the beginning of the object
   *
   * @note                    Only resets the iterator, does not free the iterator handle
   *
   * @return                  True on success, false on failure
   * @error                   Invalid iterator handle
   */
  public native bool Reset();

  /**
   * Moves the iterator to the next key
   *
   * @param buffer            Buffer to copy key name to
   * @param maxlength         Maximum length of the string buffer
   *
   * @return                  True if there are more elements, false when iteration is complete
   * @error                   Invalid iterator handle
   */
  public native bool Next(char[] buffer, int maxlength);

  /**
   * Checks if there are more elements to iterate
   *
   * @return                  True if there are more elements, false otherwise
   * @error                   Invalid iterator handle
   */
  property bool HasNext {
    public native get();
  }

  /**
   * Gets the value associated with the current key
   *
   * @note                    Needs to be freed using delete or CloseHandle()
   * @note                    This should be called after Next() to get the value for the current key
   *
   * @return                  JSON value handle for the current key's value, or null on failure
   * @error                   Invalid iterator handle
   */
  property JSON Value {
    public native get();
  }

  /**
   * Iterates to a specified key and returns the value
   *
   * @note                    Needs to be freed using delete or CloseHandle()
   * @note                    This function does the same thing as JSONObject.Get(), but is much faster
   *                          if the ordering of the keys is known at compile-time and you are using the same
   *                          order to look up the values
   * @note                    If the key exists in this object, then the iterator will stop at the next key,
   *                          otherwise the iterator will not change and null is returned
   * @note                    This function takes a linear search time if the key is not nearby
   *
   * @param key               Key name to search for (should be a UTF-8 string with null-terminator)
   *
   * @return                  JSON value handle for the key's value, or null if key not found or input is invalid
   * @error                   Invalid iterator handle
   */
  public native JSON Get(const char[] key);

  /**
   * Gets the current index in the object iteration
   *
   * @return                  Current index (0-based), or -1 if iterator is not positioned
   * @error                   Invalid iterator handle
   */
  property int Index {
    public native get();
  }

  /**
   * Removes the current key-value pair from the object (mutable iterators only)
   *
   * @note                    This function can only be called on mutable iterators
   * @note                    After calling Remove(), the iterator should not be used to continue iteration
   *
   * @return                  True on success, false on failure
   * @error                   Invalid iterator handle or iterator is not mutable
   */
  public native bool Remove();
};

public Extension __ext_json = {
  name = "json",
  file = "json.ext",
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
public void __pl_json_SetNTVOptional()
{
  // JSONObject
  MarkNativeAsOptional("JSONObject.JSONObject");
  MarkNativeAsOptional("JSONObject.FromStrings");
  MarkNativeAsOptional("JSONObject.Size.get");
  MarkNativeAsOptional("JSONObject.Get");
  MarkNativeAsOptional("JSONObject.GetBool");
  MarkNativeAsOptional("JSONObject.GetFloat");
  MarkNativeAsOptional("JSONObject.GetInt");
  MarkNativeAsOptional("JSONObject.GetInt64");
  MarkNativeAsOptional("JSONObject.GetString");
  MarkNativeAsOptional("JSONObject.IsNull");
  MarkNativeAsOptional("JSONObject.GetKey");
  MarkNativeAsOptional("JSONObject.GetValueAt");
  MarkNativeAsOptional("JSONObject.HasKey");
  MarkNativeAsOptional("JSONObject.RenameKey");
  MarkNativeAsOptional("JSONObject.Set");
  MarkNativeAsOptional("JSONObject.SetBool");
  MarkNativeAsOptional("JSONObject.SetFloat");
  MarkNativeAsOptional("JSONObject.SetInt");
  MarkNativeAsOptional("JSONObject.SetInt64");
  MarkNativeAsOptional("JSONObject.SetNull");
  MarkNativeAsOptional("JSONObject.SetString");
  MarkNativeAsOptional("JSONObject.Remove");
  MarkNativeAsOptional("JSONObject.Clear");
  MarkNativeAsOptional("JSONObject.FromString");
  MarkNativeAsOptional("JSONObject.FromFile");
  MarkNativeAsOptional("JSONObject.Sort");

  // JSONArray
  MarkNativeAsOptional("JSONArray.JSONArray");
  MarkNativeAsOptional("JSONArray.FromStrings");
  MarkNativeAsOptional("JSONArray.FromInt");
  MarkNativeAsOptional("JSONArray.FromInt64");
  MarkNativeAsOptional("JSONArray.FromBool");
  MarkNativeAsOptional("JSONArray.FromFloat");
  MarkNativeAsOptional("JSONArray.Length.get");
  MarkNativeAsOptional("JSONArray.Get");
  MarkNativeAsOptional("JSONArray.First.get");
  MarkNativeAsOptional("JSONArray.Last.get");
  MarkNativeAsOptional("JSONArray.GetBool");
  MarkNativeAsOptional("JSONArray.GetFloat");
  MarkNativeAsOptional("JSONArray.GetInt");
  MarkNativeAsOptional("JSONArray.GetInt64");
  MarkNativeAsOptional("JSONArray.GetString");
  MarkNativeAsOptional("JSONArray.IsNull");
  MarkNativeAsOptional("JSONArray.Set");
  MarkNativeAsOptional("JSONArray.SetBool");
  MarkNativeAsOptional("JSONArray.SetFloat");
  MarkNativeAsOptional("JSONArray.SetInt");
  MarkNativeAsOptional("JSONArray.SetInt64");
  MarkNativeAsOptional("JSONArray.SetNull");
  MarkNativeAsOptional("JSONArray.SetString");
  MarkNativeAsOptional("JSONArray.Push");
  MarkNativeAsOptional("JSONArray.PushBool");
  MarkNativeAsOptional("JSONArray.PushFloat");
  MarkNativeAsOptional("JSONArray.PushInt");
  MarkNativeAsOptional("JSONArray.PushInt64");
  MarkNativeAsOptional("JSONArray.PushNull");
  MarkNativeAsOptional("JSONArray.PushString");
  MarkNativeAsOptional("JSONArray.Insert");
  MarkNativeAsOptional("JSONArray.InsertBool");
  MarkNativeAsOptional("JSONArray.InsertInt");
  MarkNativeAsOptional("JSONArray.InsertInt64");
  MarkNativeAsOptional("JSONArray.InsertFloat");
  MarkNativeAsOptional("JSONArray.InsertString");
  MarkNativeAsOptional("JSONArray.InsertNull");
  MarkNativeAsOptional("JSONArray.Prepend");
  MarkNativeAsOptional("JSONArray.PrependBool");
  MarkNativeAsOptional("JSONArray.PrependInt");
  MarkNativeAsOptional("JSONArray.PrependInt64");
  MarkNativeAsOptional("JSONArray.PrependFloat");
  MarkNativeAsOptional("JSONArray.PrependString");
  MarkNativeAsOptional("JSONArray.PrependNull");
  MarkNativeAsOptional("JSONArray.Remove");
  MarkNativeAsOptional("JSONArray.RemoveFirst");
  MarkNativeAsOptional("JSONArray.RemoveLast");
  MarkNativeAsOptional("JSONArray.RemoveRange");
  MarkNativeAsOptional("JSONArray.Clear");
  MarkNativeAsOptional("JSONArray.FromString");
  MarkNativeAsOptional("JSONArray.FromFile");
  MarkNativeAsOptional("JSONArray.IndexOfBool");
  MarkNativeAsOptional("JSONArray.IndexOfString");
  MarkNativeAsOptional("JSONArray.IndexOfInt");
  MarkNativeAsOptional("JSONArray.IndexOfInt64");
  MarkNativeAsOptional("JSONArray.IndexOfFloat");
  MarkNativeAsOptional("JSONArray.Sort");

  // JSON
  MarkNativeAsOptional("JSON.ToString");
  MarkNativeAsOptional("JSON.ToFile");
  MarkNativeAsOptional("JSON.Parse");
  MarkNativeAsOptional("JSON.Equals");
  MarkNativeAsOptional("JSON.EqualsStr");
  MarkNativeAsOptional("JSON.DeepCopy");
  MarkNativeAsOptional("JSON.GetTypeDesc");
  MarkNativeAsOptional("JSON.GetSerializedSize");
  MarkNativeAsOptional("JSON.ReadSize.get");
  MarkNativeAsOptional("JSON.RefCount.get");
  MarkNativeAsOptional("JSON.Type.get");
  MarkNativeAsOptional("JSON.SubType.get");
  MarkNativeAsOptional("JSON.IsArray.get");
  MarkNativeAsOptional("JSON.IsObject.get");
  MarkNativeAsOptional("JSON.IsInt.get");
  MarkNativeAsOptional("JSON.IsUint.get");
  MarkNativeAsOptional("JSON.IsSint.get");
  MarkNativeAsOptional("JSON.IsNum.get");
  MarkNativeAsOptional("JSON.IsBool.get");
  MarkNativeAsOptional("JSON.IsTrue.get");
  MarkNativeAsOptional("JSON.IsFalse.get");
  MarkNativeAsOptional("JSON.IsFloat.get");
  MarkNativeAsOptional("JSON.IsStr.get");
  MarkNativeAsOptional("JSON.IsNull.get");
  MarkNativeAsOptional("JSON.IsCtn.get");
  MarkNativeAsOptional("JSON.IsMutable.get");
  MarkNativeAsOptional("JSON.IsImmutable.get");
  MarkNativeAsOptional("JSON.ForeachObject");
  MarkNativeAsOptional("JSON.ForeachArray");
  MarkNativeAsOptional("JSON.ForeachKey");
  MarkNativeAsOptional("JSON.ForeachIndex");
  MarkNativeAsOptional("JSON.ToMutable");
  MarkNativeAsOptional("JSON.ToImmutable");
  MarkNativeAsOptional("JSON.ApplyJsonPatch");
  MarkNativeAsOptional("JSON.JsonPatchInPlace");
  MarkNativeAsOptional("JSON.ApplyMergePatch");
  MarkNativeAsOptional("JSON.MergePatchInPlace");

  // JSON CREATE & GET
  MarkNativeAsOptional("JSON.Pack");
  MarkNativeAsOptional("JSON.CreateBool");
  MarkNativeAsOptional("JSON.CreateFloat");
  MarkNativeAsOptional("JSON.CreateInt");
  MarkNativeAsOptional("JSON.CreateInt64");
  MarkNativeAsOptional("JSON.CreateNull");
  MarkNativeAsOptional("JSON.CreateString");
  MarkNativeAsOptional("JSON.GetBool");
  MarkNativeAsOptional("JSON.GetFloat");
  MarkNativeAsOptional("JSON.GetInt");
  MarkNativeAsOptional("JSON.GetInt64");
  MarkNativeAsOptional("JSON.GetString");
  MarkNativeAsOptional("JSON.ReadNumber");
  MarkNativeAsOptional("JSON.WriteNumber");
  MarkNativeAsOptional("JSON.SetFpToFloat");
  MarkNativeAsOptional("JSON.SetFpToFixed");
  MarkNativeAsOptional("JSON.SetBool");
  MarkNativeAsOptional("JSON.SetInt");
  MarkNativeAsOptional("JSON.SetInt64");
  MarkNativeAsOptional("JSON.SetFloat");
  MarkNativeAsOptional("JSON.SetString");
  MarkNativeAsOptional("JSON.SetNull");

  // JSON POINTER
  MarkNativeAsOptional("JSON.PtrGet");
  MarkNativeAsOptional("JSON.PtrGetBool");
  MarkNativeAsOptional("JSON.PtrGetFloat");
  MarkNativeAsOptional("JSON.PtrGetInt");
  MarkNativeAsOptional("JSON.PtrGetInt64");
  MarkNativeAsOptional("JSON.PtrGetString");
  MarkNativeAsOptional("JSON.PtrGetIsNull");
  MarkNativeAsOptional("JSON.PtrGetLength");
  MarkNativeAsOptional("JSON.PtrSet");
  MarkNativeAsOptional("JSON.PtrSetBool");
  MarkNativeAsOptional("JSON.PtrSetFloat");
  MarkNativeAsOptional("JSON.PtrSetInt");
  MarkNativeAsOptional("JSON.PtrSetInt64");
  MarkNativeAsOptional("JSON.PtrSetString");
  MarkNativeAsOptional("JSON.PtrSetNull");
  MarkNativeAsOptional("JSON.PtrAdd");
  MarkNativeAsOptional("JSON.PtrAddBool");
  MarkNativeAsOptional("JSON.PtrAddFloat");
  MarkNativeAsOptional("JSON.PtrAddInt");
  MarkNativeAsOptional("JSON.PtrAddInt64");
  MarkNativeAsOptional("JSON.PtrAddString");
  MarkNativeAsOptional("JSON.PtrAddNull");
  MarkNativeAsOptional("JSON.PtrRemove");
  MarkNativeAsOptional("JSON.PtrTryGetVal");
  MarkNativeAsOptional("JSON.PtrTryGetBool");
  MarkNativeAsOptional("JSON.PtrTryGetFloat");
  MarkNativeAsOptional("JSON.PtrTryGetInt");
  MarkNativeAsOptional("JSON.PtrTryGetInt64");
  MarkNativeAsOptional("JSON.PtrTryGetString");

  // JSONArrIter
  MarkNativeAsOptional("JSONArrIter.JSONArrIter");
  MarkNativeAsOptional("JSONArrIter.Next.get");
  MarkNativeAsOptional("JSONArrIter.HasNext.get");
  MarkNativeAsOptional("JSONArrIter.Index.get");
  MarkNativeAsOptional("JSONArrIter.Remove");
  MarkNativeAsOptional("JSONArrIter.Reset");

  // JSONObjIter
  MarkNativeAsOptional("JSONObjIter.JSONObjIter");
  MarkNativeAsOptional("JSONObjIter.Next");
  MarkNativeAsOptional("JSONObjIter.HasNext.get");
  MarkNativeAsOptional("JSONObjIter.Value.get");
  MarkNativeAsOptional("JSONObjIter.Get");
  MarkNativeAsOptional("JSONObjIter.Index.get");
  MarkNativeAsOptional("JSONObjIter.Remove");
  MarkNativeAsOptional("JSONObjIter.Reset");
}
#endif