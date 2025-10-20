#include "extension.h"
#include "YYJSONManager.h"

class SourceModPackParamProvider : public IPackParamProvider
{
private:
	IPluginContext* m_pContext;
	const cell_t* m_pParams;
	unsigned int m_currentIndex;

public:
	SourceModPackParamProvider(IPluginContext* pContext, const cell_t* params, unsigned int startIndex)
		: m_pContext(pContext), m_pParams(params), m_currentIndex(startIndex) {}

	bool GetNextString(const char** out_str) override {
		char* str;
		if (m_pContext->LocalToString(m_pParams[m_currentIndex++], &str) != SP_ERROR_NONE) {
			return false;
		}
		*out_str = str;
		return str != nullptr;
	}

	bool GetNextInt(int* out_value) override {
		cell_t* val;
		if (m_pContext->LocalToPhysAddr(m_pParams[m_currentIndex++], &val) != SP_ERROR_NONE) {
			return false;
		}
		*out_value = *val;
		return true;
	}

	bool GetNextFloat(float* out_value) override {
		cell_t* val;
		if (m_pContext->LocalToPhysAddr(m_pParams[m_currentIndex++], &val) != SP_ERROR_NONE) {
			return false;
		}
		*out_value = sp_ctof(*val);
		return true;
	}

	bool GetNextBool(bool* out_value) override {
		cell_t* val;
		if (m_pContext->LocalToPhysAddr(m_pParams[m_currentIndex++], &val) != SP_ERROR_NONE) {
			return false;
		}
		*out_value = (*val != 0);
		return true;
	}
};

/**
 * Helper function: Create a SourceMod handle for YYJSONValue and return it directly
 * Used by functions that return Handle_t
 *
 * @param pContext      Plugin context
 * @param pYYJSONValue  JSON value to wrap (will be released on failure)
 * @param error_context Descriptive context for error messages
 * @return Handle on success, throws native error on failure
 */
static cell_t CreateAndReturnHandle(IPluginContext* pContext, YYJSONValue* pYYJSONValue, const char* error_context)
{
	if (!pYYJSONValue) {
		return pContext->ThrowNativeError("Failed to create %s", error_context);
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	pYYJSONValue->m_handle = handlesys->CreateHandleEx(g_htJSON, pYYJSONValue, &sec, nullptr, &err);

	if (!pYYJSONValue->m_handle) {
		g_pYYJSONManager->Release(pYYJSONValue);
		return pContext->ThrowNativeError("Failed to create handle for %s (error code: %d)", error_context, err);
	}

	return pYYJSONValue->m_handle;
}

/**
 * Helper function: Create a SourceMod handle for YYJSONValue and assign to output parameter
 * Used by iterator functions (foreach) that assign handle via reference
 *
 * @param pContext      Plugin context
 * @param pYYJSONValue  JSON value to wrap (will be released on failure)
 * @param param_index   Parameter index for output handle
 * @param error_context Descriptive context for error messages
 * @return true on success, false on failure (throws native error)
 */
static bool CreateAndAssignHandle(IPluginContext* pContext, YYJSONValue* pYYJSONValue, 
                                   cell_t param_index, const char* error_context)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	pYYJSONValue->m_handle = handlesys->CreateHandleEx(g_htJSON, pYYJSONValue, &sec, nullptr, &err);

	if (!pYYJSONValue->m_handle) {
		g_pYYJSONManager->Release(pYYJSONValue);
		pContext->ThrowNativeError("Failed to create handle for %s (error code: %d)", error_context, err);
		return false;
	}

	cell_t* valHandle;
	pContext->LocalToPhysAddr(param_index, &valHandle);
	*valHandle = pYYJSONValue->m_handle;
	return true;
}

static cell_t json_val_pack(IPluginContext* pContext, const cell_t* params) {
	char* fmt;
	pContext->LocalToString(params[1], &fmt);

	SourceModPackParamProvider provider(pContext, params, 2);

	char error[YYJSON_PACK_ERROR_SIZE];
	YYJSONValue* pYYJSONValue = g_pYYJSONManager->Pack(fmt, &provider, error, sizeof(error));

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError("Failed to pack JSON: %s", error);
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "packed JSON");
}

static cell_t json_doc_parse(IPluginContext* pContext, const cell_t* params)
{
	char* str;
	pContext->LocalToString(params[1], &str);

	bool is_file = params[2];
	bool is_mutable_doc = params[3];
	yyjson_read_flag read_flg = static_cast<yyjson_read_flag>(params[4]);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	YYJSONValue* pYYJSONValue = g_pYYJSONManager->ParseJSON(str, is_file, is_mutable_doc, read_flg, error, sizeof(error));

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError(error);
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "parsed JSON document");
}

static cell_t json_doc_equals(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle1 = g_pYYJSONManager->GetFromHandle(pContext, params[1]);
	YYJSONValue* handle2 = g_pYYJSONManager->GetFromHandle(pContext, params[2]);

	if (!handle1 || !handle2) return 0;

	return g_pYYJSONManager->Equals(handle1, handle2);
}

static cell_t json_doc_copy_deep(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* targetDoc = g_pYYJSONManager->GetFromHandle(pContext, params[1]);
	YYJSONValue* sourceValue = g_pYYJSONManager->GetFromHandle(pContext, params[2]);

	if (!targetDoc || !sourceValue) return 0;

	YYJSONValue* pYYJSONValue = g_pYYJSONManager->DeepCopy(targetDoc, sourceValue);

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError("Failed to copy JSON value");
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "copied JSON value");
}

static cell_t json_val_get_type_desc(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	const char* type_desc = g_pYYJSONManager->GetTypeDesc(handle);
	pContext->StringToLocalUTF8(params[2], params[3], type_desc, nullptr);

	return 1;
}

static cell_t json_obj_parse_str(IPluginContext* pContext, const cell_t* params)
{
	char* str;
	pContext->LocalToString(params[1], &str);
	yyjson_read_flag read_flg = static_cast<yyjson_read_flag>(params[2]);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	YYJSONValue* pYYJSONValue = g_pYYJSONManager->ObjectParseString(str, read_flg, error, sizeof(error));

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError(error);
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON object from string");
}

static cell_t json_obj_parse_file(IPluginContext* pContext, const cell_t* params)
{
	char* path;
	pContext->LocalToString(params[1], &path);
	yyjson_read_flag read_flg = static_cast<yyjson_read_flag>(params[2]);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	YYJSONValue* pYYJSONValue = g_pYYJSONManager->ObjectParseFile(path, read_flg, error, sizeof(error));

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError(error);
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON object from file");
}

static cell_t json_arr_parse_str(IPluginContext* pContext, const cell_t* params)
{
	char* str;
	pContext->LocalToString(params[1], &str);
	yyjson_read_flag read_flg = static_cast<yyjson_read_flag>(params[2]);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	YYJSONValue* pYYJSONValue = g_pYYJSONManager->ArrayParseString(str, read_flg, error, sizeof(error));

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError(error);
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON array from string");
}

static cell_t json_arr_parse_file(IPluginContext* pContext, const cell_t* params)
{
	char* path;
	pContext->LocalToString(params[1], &path);
	yyjson_read_flag read_flg = static_cast<yyjson_read_flag>(params[2]);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	YYJSONValue* pYYJSONValue = g_pYYJSONManager->ArrayParseFile(path, read_flg, error, sizeof(error));

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError(error);
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON array from file");
}

static cell_t json_arr_index_of_bool(IPluginContext *pContext, const cell_t *params)
{
	YYJSONValue *handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	bool searchValue = params[2];
	return g_pYYJSONManager->ArrayIndexOfBool(handle, searchValue);
}

static cell_t json_arr_index_of_str(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* searchStr;
	pContext->LocalToString(params[2], &searchStr);

	return g_pYYJSONManager->ArrayIndexOfString(handle, searchStr);
}

static cell_t json_arr_index_of_int(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	int searchValue = params[2];
	return g_pYYJSONManager->ArrayIndexOfInt(handle, searchValue);
}

static cell_t json_arr_index_of_integer64(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* searchStr;
	pContext->LocalToString(params[2], &searchStr);

	char* endptr;
	errno = 0;
	long long searchValue = strtoll(searchStr, &endptr, 10);

	if (errno == ERANGE || *endptr != '\0') {
		return pContext->ThrowNativeError("Invalid integer64 value: %s", searchStr);
	}

	return g_pYYJSONManager->ArrayIndexOfInt64(handle, searchValue);
}

static cell_t json_arr_index_of_float(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	double searchValue = static_cast<double>(sp_ctof(params[2]));
	return g_pYYJSONManager->ArrayIndexOfFloat(handle, searchValue);
}

static cell_t json_val_get_type(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->GetType(handle);
}

static cell_t json_val_get_subtype(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->GetSubtype(handle);
}

static cell_t json_val_is_array(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->IsArray(handle);
}

static cell_t json_val_is_object(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->IsObject(handle);
}

static cell_t json_val_is_int(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->IsInt(handle);
}

static cell_t json_val_is_uint(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->IsUint(handle);
}

static cell_t json_val_is_sint(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->IsSint(handle);
}

static cell_t json_val_is_num(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->IsNum(handle);
}

static cell_t json_val_is_bool(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->IsBool(handle);
}

static cell_t json_val_is_true(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->IsTrue(handle);
}

static cell_t json_val_is_false(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->IsFalse(handle);
}

static cell_t json_val_is_float(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->IsFloat(handle);
}

static cell_t json_val_is_str(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->IsStr(handle);
}

static cell_t json_val_is_null(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->IsNull(handle);
}

static cell_t json_val_is_ctn(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->IsCtn(handle);
}

static cell_t json_val_is_mutable(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->IsMutable(handle);
}

static cell_t json_val_is_immutable(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pYYJSONManager->IsImmutable(handle);
}

static cell_t json_obj_init(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* pYYJSONValue = g_pYYJSONManager->ObjectInit();
	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON object");
}

static cell_t json_obj_init_with_str(IPluginContext* pContext, const cell_t* params)
{
	cell_t* addr;
	pContext->LocalToPhysAddr(params[1], &addr);
	cell_t array_size = params[2];

	if (array_size < 2) {
		return pContext->ThrowNativeError("Array must contain at least one key-value pair");
	}
	if (array_size % 2 != 0) {
		return pContext->ThrowNativeError("Array must contain an even number of strings (got %d)", array_size);
	}

	std::vector<const char*> kv_pairs;
	kv_pairs.reserve(array_size);

	for (cell_t i = 0; i < array_size; i += 2) {
		char* key;
		char* value;

		if (pContext->LocalToString(addr[i], &key) != SP_ERROR_NONE) {
			return pContext->ThrowNativeError("Failed to read key at index %d", i);
		}
		if (!key || !key[0]) {
			return pContext->ThrowNativeError("Empty key at index %d", i);
		}

		if (pContext->LocalToString(addr[i + 1], &value) != SP_ERROR_NONE) {
			return pContext->ThrowNativeError("Failed to read value at index %d", i + 1);
		}
		if (!value) {
			return pContext->ThrowNativeError("Invalid value at index %d", i + 1);
		}

		kv_pairs.push_back(key);
		kv_pairs.push_back(value);
	}

	YYJSONValue* pYYJSONValue = g_pYYJSONManager->ObjectInitWithStrings(kv_pairs.data(), array_size / 2);

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError("Failed to create JSON object from key-value pairs");
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON object from key-value pairs");
}

static cell_t json_val_create_bool(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* pYYJSONValue = g_pYYJSONManager->CreateBool(params[1]);
	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON boolean value");
}

static cell_t json_val_create_float(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* pYYJSONValue = g_pYYJSONManager->CreateFloat(sp_ctof(params[1]));
	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON float value");
}

static cell_t json_val_create_int(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* pYYJSONValue = g_pYYJSONManager->CreateInt(params[1]);
	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON integer value");
}

static cell_t json_val_create_integer64(IPluginContext* pContext, const cell_t* params)
{
	char* value;
	pContext->LocalToString(params[1], &value);

	char* endptr;
	errno = 0;
	long long num = strtoll(value, &endptr, 10);

	if (errno == ERANGE || *endptr != '\0') {
		return pContext->ThrowNativeError("Invalid integer64 value: %s", value);
	}

	YYJSONValue* pYYJSONValue = g_pYYJSONManager->CreateInt64(num);

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError("Failed to create JSON integer64 value");
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON integer64 value");
}

static cell_t json_val_create_str(IPluginContext* pContext, const cell_t* params)
{
	char* str;
	pContext->LocalToString(params[1], &str);

	YYJSONValue* pYYJSONValue = g_pYYJSONManager->CreateString(str);

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError("Failed to create JSON string value");
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON string value");
}

static cell_t json_val_get_bool(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	bool value;
	if (!g_pYYJSONManager->GetBool(handle, &value)) {
		return pContext->ThrowNativeError("Type mismatch: expected boolean value");
	}

	return value;
}

static cell_t json_val_get_float(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	double value;
	if (!g_pYYJSONManager->GetFloat(handle, &value)) {
		return pContext->ThrowNativeError("Type mismatch: expected float value");
	}

	return sp_ftoc(static_cast<float>(value));
}

static cell_t json_val_get_int(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	int value;
	if (!g_pYYJSONManager->GetInt(handle, &value)) {
		return pContext->ThrowNativeError("Type mismatch: expected integer value");
	}

	return value;
}

static cell_t json_val_get_integer64(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	int64_t value;
	if (!g_pYYJSONManager->GetInt64(handle, &value)) {
		return pContext->ThrowNativeError("Type mismatch: expected integer64 value");
	}

	char result[21];
	snprintf(result, sizeof(result), "%" PRId64, value);
	pContext->StringToLocalUTF8(params[2], params[3], result, nullptr);

	return 1;
}

static cell_t json_val_get_str(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	const char* str = nullptr;
	size_t len = 0;

	if (!g_pYYJSONManager->GetString(handle, &str, &len)) {
		return pContext->ThrowNativeError("Type mismatch: expected string value");
	}

	size_t maxlen = static_cast<size_t>(params[3]);

	if (len + 1 > maxlen) {
		return pContext->ThrowNativeError("Buffer is too small (need %d, have %d)", len + 1, maxlen);
	}

	pContext->StringToLocalUTF8(params[2], maxlen, str, nullptr);

	return 1;
}

static cell_t json_val_get_serialized_size(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	yyjson_write_flag write_flg = static_cast<yyjson_write_flag>(params[2]);
	size_t size = g_pYYJSONManager->GetSerializedSize(handle, write_flg);

	return static_cast<cell_t>(size);
}

static cell_t json_val_get_read_size(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t size = g_pYYJSONManager->GetReadSize(handle);
	if (size == 0) return 0;

	return static_cast<cell_t>(size);
}

static cell_t json_val_create_null(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* pYYJSONValue = g_pYYJSONManager->CreateNull();
	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON null value");
}

static cell_t json_arr_init(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* pYYJSONValue = g_pYYJSONManager->ArrayInit();
	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON array");
}

static cell_t json_arr_init_with_str(IPluginContext* pContext, const cell_t* params)
{
	cell_t* addr;
	pContext->LocalToPhysAddr(params[1], &addr);
	cell_t array_size = params[2];

	std::vector<const char*> strs;
	strs.reserve(array_size);

	for (cell_t i = 0; i < array_size; i++) {
		char* str;
		pContext->LocalToString(addr[i], &str);
		strs.push_back(str);
	}

	YYJSONValue* pYYJSONValue = g_pYYJSONManager->ArrayInitWithStrings(strs.data(), strs.size());

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError("Failed to create JSON array from strings");
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON array from strings");
}

static cell_t json_arr_get_size(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t size = g_pYYJSONManager->ArrayGetSize(handle);
	return static_cast<cell_t>(size);
}

static cell_t json_arr_get_val(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);

	YYJSONValue* pYYJSONValue = g_pYYJSONManager->ArrayGet(handle, index);

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError("Index %d is out of bounds", index);
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON array value");
}

static cell_t json_arr_get_first(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	YYJSONValue* pYYJSONValue = g_pYYJSONManager->ArrayGetFirst(handle);

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError("Array is empty");
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "first JSON array value");
}

static cell_t json_arr_get_last(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	YYJSONValue* pYYJSONValue = g_pYYJSONManager->ArrayGetLast(handle);

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError("Array is empty");
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "last JSON array value");
}

static cell_t json_arr_get_bool(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);

	bool value;
	if (!g_pYYJSONManager->ArrayGetBool(handle, index, &value)) {
		return pContext->ThrowNativeError("Failed to get boolean at index %d", index);
	}

	return value;
}

static cell_t json_arr_get_float(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);

	double value;
	if (!g_pYYJSONManager->ArrayGetFloat(handle, index, &value)) {
		return pContext->ThrowNativeError("Failed to get float at index %d", index);
	}

	return sp_ftoc(static_cast<float>(value));
}

static cell_t json_arr_get_integer(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);

	int value;
	if (!g_pYYJSONManager->ArrayGetInt(handle, index, &value)) {
		return pContext->ThrowNativeError("Failed to get integer at index %d", index);
	}

	return value;
}

static cell_t json_arr_get_integer64(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);
	int64_t value;

	if (!g_pYYJSONManager->ArrayGetInt64(handle, index, &value)) {
		return pContext->ThrowNativeError("Failed to get integer64 at index %d", index);
	}

	char result[21];
	snprintf(result, sizeof(result), "%" PRId64, value);
	pContext->StringToLocalUTF8(params[3], params[4], result, nullptr);

	return 1;
}

static cell_t json_arr_get_str(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);
	const char* str = nullptr;
	size_t len = 0;

	if (!g_pYYJSONManager->ArrayGetString(handle, index, &str, &len)) {
		return pContext->ThrowNativeError("Failed to get string at index %d", index);
	}

	size_t maxlen = static_cast<size_t>(params[4]);
	if (len + 1 > maxlen) {
		return pContext->ThrowNativeError("Buffer is too small (need %d, have %d)", len + 1, maxlen);
	}

	pContext->StringToLocalUTF8(params[3], maxlen, str, nullptr);

	return 1;
}

static cell_t json_arr_is_null(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);

	return g_pYYJSONManager->ArrayIsNull(handle, index);
}

static cell_t json_arr_replace_val(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle1 = g_pYYJSONManager->GetFromHandle(pContext, params[1]);
	YYJSONValue* handle2 = g_pYYJSONManager->GetFromHandle(pContext, params[3]);

	if (!handle1 || !handle2) return 0;

	if (!handle1->IsMutable()) {
		return pContext->ThrowNativeError("Cannot replace value in an immutable JSON array");
	}

	size_t index = static_cast<size_t>(params[2]);
	return g_pYYJSONManager->ArrayReplace(handle1, index, handle2);
}

static cell_t json_arr_replace_bool(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot replace value in an immutable JSON array");
	}

	size_t index = static_cast<size_t>(params[2]);
	return g_pYYJSONManager->ArrayReplaceBool(handle, index, params[3]);
}

static cell_t json_arr_replace_float(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot replace value in an immutable JSON array");
	}

	size_t index = static_cast<size_t>(params[2]);
	return g_pYYJSONManager->ArrayReplaceFloat(handle, index, sp_ctof(params[3]));
}

static cell_t json_arr_replace_integer(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot replace value in an immutable JSON array");
	}

	size_t index = static_cast<size_t>(params[2]);
	return g_pYYJSONManager->ArrayReplaceInt(handle, index, params[3]);
}

static cell_t json_arr_replace_integer64(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot replace value in an immutable JSON array");
	}

	char* value;
	pContext->LocalToString(params[3], &value);

	char* endptr;
	errno = 0;
	long long num = strtoll(value, &endptr, 10);

	if (errno == ERANGE || *endptr != '\0') {
		return pContext->ThrowNativeError("Invalid integer64 value: %s", value);
	}

	size_t index = static_cast<size_t>(params[2]);
	return g_pYYJSONManager->ArrayReplaceInt64(handle, index, num);
}

static cell_t json_arr_replace_null(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot replace value in an immutable JSON array");
	}

	size_t index = static_cast<size_t>(params[2]);
	return g_pYYJSONManager->ArrayReplaceNull(handle, index);
}

static cell_t json_arr_replace_str(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot replace value in an immutable JSON array");
	}

	char* val;
	pContext->LocalToString(params[3], &val);

	size_t index = static_cast<size_t>(params[2]);
	return g_pYYJSONManager->ArrayReplaceString(handle, index, val);
}

static cell_t json_arr_append_val(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle1 = g_pYYJSONManager->GetFromHandle(pContext, params[1]);
	YYJSONValue* handle2 = g_pYYJSONManager->GetFromHandle(pContext, params[2]);

	if (!handle1 || !handle2) return 0;

	if (!handle1->IsMutable()) {
		return pContext->ThrowNativeError("Cannot append value to an immutable JSON array");
	}

	return g_pYYJSONManager->ArrayAppend(handle1, handle2);
}

static cell_t json_arr_append_bool(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot append value to an immutable JSON array");
	}

	return g_pYYJSONManager->ArrayAppendBool(handle, params[2]);
}

static cell_t json_arr_append_float(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot append value to an immutable JSON array");
	}

	return g_pYYJSONManager->ArrayAppendFloat(handle, sp_ctof(params[2]));
}

static cell_t json_arr_append_int(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot append value to an immutable JSON array");
	}

	return g_pYYJSONManager->ArrayAppendInt(handle, params[2]);
}

static cell_t json_arr_append_integer64(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot append value to an immutable JSON array");
	}

	char* value;
	pContext->LocalToString(params[2], &value);

	char* endptr;
	errno = 0;
	long long num = strtoll(value, &endptr, 10);

	if (errno == ERANGE || *endptr != '\0') {
		return pContext->ThrowNativeError("Invalid integer64 value: %s", value);
	}

	return g_pYYJSONManager->ArrayAppendInt64(handle, num);
}

static cell_t json_arr_append_null(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot append value to an immutable JSON array");
	}

	return g_pYYJSONManager->ArrayAppendNull(handle);
}

static cell_t json_arr_append_str(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot append value to an immutable JSON array");
	}

	char* str;
	pContext->LocalToString(params[2], &str);

	return g_pYYJSONManager->ArrayAppendString(handle, str);
}

static cell_t json_arr_remove(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot remove value from an immutable JSON array");
	}

	size_t index = static_cast<size_t>(params[2]);
	return g_pYYJSONManager->ArrayRemove(handle, index);
}

static cell_t json_arr_remove_first(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot remove value from an immutable JSON array");
	}

	return g_pYYJSONManager->ArrayRemoveFirst(handle);
}

static cell_t json_arr_remove_last(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot remove value from an immutable JSON array");
	}

	return g_pYYJSONManager->ArrayRemoveLast(handle);
}

static cell_t json_arr_remove_range(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot remove value from an immutable JSON array");
	}

	size_t start_index = static_cast<size_t>(params[2]);
	size_t end_index = static_cast<size_t>(params[3]);

	return g_pYYJSONManager->ArrayRemoveRange(handle, start_index, end_index);
}

static cell_t json_arr_clear(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot clear an immutable JSON array");
	}

	return g_pYYJSONManager->ArrayClear(handle);
}

static cell_t json_doc_write_to_str(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t buffer_size = static_cast<size_t>(params[3]);
	yyjson_write_flag write_flg = static_cast<yyjson_write_flag>(params[4]);

	char* temp_buffer = (char*)malloc(buffer_size);
	if (!temp_buffer) {
		return pContext->ThrowNativeError("Failed to allocate buffer");
	}

	size_t output_len = 0;
	if (!g_pYYJSONManager->WriteToString(handle, temp_buffer, buffer_size, write_flg, &output_len)) {
		free(temp_buffer);
		return pContext->ThrowNativeError("Buffer too small or write failed");
	}

	pContext->StringToLocalUTF8(params[2], buffer_size, temp_buffer, nullptr);
	free(temp_buffer);
	return static_cast<cell_t>(output_len);
}

static cell_t json_doc_write_to_file(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);
	yyjson_write_flag write_flg = static_cast<yyjson_write_flag>(params[3]);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->WriteToFile(handle, path, write_flg, error, sizeof(error))) {
		return pContext->ThrowNativeError(error);
	}

	return true;
}

static cell_t json_obj_get_size(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t size = g_pYYJSONManager->ObjectGetSize(handle);
	return static_cast<cell_t>(size);
}

static cell_t json_obj_get_key(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);
	const char* key = nullptr;

	if (!g_pYYJSONManager->ObjectGetKey(handle, index, &key)) {
		return pContext->ThrowNativeError("Index %d is out of bounds", index);
	}

	pContext->StringToLocalUTF8(params[3], params[4], key, nullptr);
	return 1;
}

static cell_t json_obj_get_val_at(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);

	YYJSONValue* pYYJSONValue = g_pYYJSONManager->ObjectGetValueAt(handle, index);

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError("Index %d is out of bounds", index);
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON object value");
}

static cell_t json_obj_get_val(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	YYJSONValue* pYYJSONValue = g_pYYJSONManager->ObjectGet(handle, key);

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError("Key not found: %s", key);
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON object value");
}

static cell_t json_obj_get_bool(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	bool value;
	if (!g_pYYJSONManager->ObjectGetBool(handle, key, &value)) {
		return pContext->ThrowNativeError("Failed to get boolean for key '%s'", key);
	}

	return value;
}

static cell_t json_obj_get_float(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	double value;
	if (!g_pYYJSONManager->ObjectGetFloat(handle, key, &value)) {
		return pContext->ThrowNativeError("Failed to get float for key '%s'", key);
	}

	return sp_ftoc(static_cast<float>(value));
}

static cell_t json_obj_get_int(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	int value;
	if (!g_pYYJSONManager->ObjectGetInt(handle, key, &value)) {
		return pContext->ThrowNativeError("Failed to get integer for key '%s'", key);
	}

	return value;
}

static cell_t json_obj_get_integer64(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	int64_t value;
	if (!g_pYYJSONManager->ObjectGetInt64(handle, key, &value)) {
		return pContext->ThrowNativeError("Failed to get integer64 for key '%s'", key);
	}

	char result[21];
	snprintf(result, sizeof(result), "%" PRId64, value);
	pContext->StringToLocalUTF8(params[3], params[4], result, nullptr);

	return 1;
}

static cell_t json_obj_get_str(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	const char* str = nullptr;
	size_t len = 0;
	if (!g_pYYJSONManager->ObjectGetString(handle, key, &str, &len)) {
		return pContext->ThrowNativeError("Failed to get string for key '%s'", key);
	}

	size_t maxlen = static_cast<size_t>(params[4]);
	if (len + 1 > maxlen) {
		return pContext->ThrowNativeError("Buffer is too small (need %d, have %d)", len + 1, maxlen);
	}

	pContext->StringToLocalUTF8(params[3], maxlen, str, nullptr);

	return 1;
}

static cell_t json_obj_clear(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot clear an immutable JSON object");
	}

	return g_pYYJSONManager->ObjectClear(handle);
}

static cell_t json_obj_is_null(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	bool is_null = false;
	if (!g_pYYJSONManager->ObjectIsNull(handle, key, &is_null)) {
		return pContext->ThrowNativeError("Key not found: %s", key);
	}

	return is_null;
}

static cell_t json_obj_has_key(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	bool ptr_use = params[3];

	return g_pYYJSONManager->ObjectHasKey(handle, key, ptr_use);
}

static cell_t json_obj_rename_key(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot rename key in an immutable JSON object");
	}

	char* old_key;
	pContext->LocalToString(params[2], &old_key);

	char* new_key;
	pContext->LocalToString(params[3], &new_key);

	bool allow_duplicate = params[4];

	if (!g_pYYJSONManager->ObjectRenameKey(handle, old_key, new_key, allow_duplicate)) {
		return pContext->ThrowNativeError("Failed to rename key from '%s' to '%s'", old_key, new_key);
	}

	return true;
}

static cell_t json_obj_set_val(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle1 = g_pYYJSONManager->GetFromHandle(pContext, params[1]);
	YYJSONValue* handle2 = g_pYYJSONManager->GetFromHandle(pContext, params[3]);

	if (!handle1 || !handle2) return 0;

	if (!handle1->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON object");
	}

	char* key;
	pContext->LocalToString(params[2], &key);

	return g_pYYJSONManager->ObjectSet(handle1, key, handle2);
}

static cell_t json_obj_set_bool(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON object");
	}

	char* key;
	pContext->LocalToString(params[2], &key);

	return g_pYYJSONManager->ObjectSetBool(handle, key, params[3]);
}

static cell_t json_obj_set_float(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON object");
	}

	char* key;
	pContext->LocalToString(params[2], &key);

	return g_pYYJSONManager->ObjectSetFloat(handle, key, sp_ctof(params[3]));
}

static cell_t json_obj_set_int(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON object");
	}

	char* key;
	pContext->LocalToString(params[2], &key);

	return g_pYYJSONManager->ObjectSetInt(handle, key, params[3]);
}

static cell_t json_obj_set_integer64(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON object");
	}

	char* key, * value;
	pContext->LocalToString(params[2], &key);
	pContext->LocalToString(params[3], &value);

	char* endptr;
	errno = 0;
	long long num = strtoll(value, &endptr, 10);

	if (errno == ERANGE || *endptr != '\0') {
		return pContext->ThrowNativeError("Invalid integer64 value: %s", value);
	}

	return g_pYYJSONManager->ObjectSetInt64(handle, key, num);
}

static cell_t json_obj_set_null(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON object");
	}

	char* key;
	pContext->LocalToString(params[2], &key);

	return g_pYYJSONManager->ObjectSetNull(handle, key);
}

static cell_t json_obj_set_str(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON object");
	}

	char* key, * value;
	pContext->LocalToString(params[2], &key);
	pContext->LocalToString(params[3], &value);

	return g_pYYJSONManager->ObjectSetString(handle, key, value);
}

static cell_t json_obj_remove(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot remove value from an immutable JSON object");
	}

	char* key;
	pContext->LocalToString(params[2], &key);

	return g_pYYJSONManager->ObjectRemove(handle, key);
}

static cell_t json_ptr_get_val(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	YYJSONValue* pYYJSONValue = g_pYYJSONManager->PtrGet(handle, path, error, sizeof(error));

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError("%s", error);
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "JSON pointer value");
}

static cell_t json_ptr_get_bool(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	bool value;
	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrGetBool(handle, path, &value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return value;
}

static cell_t json_ptr_get_float(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	double value;
	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrGetFloat(handle, path, &value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return sp_ftoc(static_cast<float>(value));
}

static cell_t json_ptr_get_int(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	int value;
	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrGetInt(handle, path, &value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return value;
}

static cell_t json_ptr_get_integer64(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	int64_t value;
	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrGetInt64(handle, path, &value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	char result[21];
	snprintf(result, sizeof(result), "%" PRId64, value);
	pContext->StringToLocalUTF8(params[3], params[4], result, nullptr);

	return 1;
}

static cell_t json_ptr_get_str(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	const char* str = nullptr;
	size_t len = 0;
	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrGetString(handle, path, &str, &len, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	size_t maxlen = static_cast<size_t>(params[4]);
	if (len + 1 > maxlen) {
		return pContext->ThrowNativeError("Buffer is too small (need %d, have %d)", len + 1, maxlen);
	}

	pContext->StringToLocalUTF8(params[3], maxlen, str, nullptr);

	return 1;
}

static cell_t json_ptr_get_is_null(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	bool is_null;
	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrGetIsNull(handle, path, &is_null, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return is_null;
}

static cell_t json_ptr_get_length(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	size_t len;
	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrGetLength(handle, path, &len, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return static_cast<cell_t>(len);
}

static cell_t json_ptr_set_val(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle1 = g_pYYJSONManager->GetFromHandle(pContext, params[1]);
	YYJSONValue* handle2 = g_pYYJSONManager->GetFromHandle(pContext, params[3]);

	if (!handle1 || !handle2) return 0;

	if (!handle1->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrSet(handle1, path, handle2, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_set_bool(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrSetBool(handle, path, params[3], error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_set_float(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrSetFloat(handle, path, sp_ctof(params[3]), error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_set_int(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrSetInt(handle, path, params[3], error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_set_integer64(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON document using pointer");
	}

	char* path, * value;
	pContext->LocalToString(params[2], &path);
	pContext->LocalToString(params[3], &value);

	char* endptr;
	errno = 0;
	long long num = strtoll(value, &endptr, 10);

	if (errno == ERANGE || *endptr != '\0') {
		return pContext->ThrowNativeError("Invalid integer64 value: %s", value);
	}

	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrSetInt64(handle, path, num, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_set_str(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON document using pointer");
	}

	char* path, * str;
	pContext->LocalToString(params[2], &path);
	pContext->LocalToString(params[3], &str);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrSetString(handle, path, str, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_set_null(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrSetNull(handle, path, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_add_val(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle1 = g_pYYJSONManager->GetFromHandle(pContext, params[1]);
	YYJSONValue* handle2 = g_pYYJSONManager->GetFromHandle(pContext, params[3]);

	if (!handle1 || !handle2) return 0;

	if (!handle1->IsMutable()) {
		return pContext->ThrowNativeError("Cannot add value to an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrAdd(handle1, path, handle2, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_add_bool(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot add value to an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrAddBool(handle, path, params[3], error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_add_float(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot add value to an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrAddFloat(handle, path, sp_ctof(params[3]), error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_add_int(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot add value to an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrAddInt(handle, path, params[3], error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_add_integer64(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot add value to an immutable JSON document using pointer");
	}

	char* path, * value;
	pContext->LocalToString(params[2], &path);
	pContext->LocalToString(params[3], &value);

	char* endptr;
	errno = 0;
	long long num = strtoll(value, &endptr, 10);

	if (errno == ERANGE || *endptr != '\0') {
		return pContext->ThrowNativeError("Invalid integer64 value: %s", value);
	}

	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrAddInt64(handle, path, num, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_add_str(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot add value to an immutable JSON document using pointer");
	}

	char* path, * str;
	pContext->LocalToString(params[2], &path);
	pContext->LocalToString(params[3], &str);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrAddString(handle, path, str, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_add_null(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot add value to an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrAddNull(handle, path, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_remove_val(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot remove value from an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[YYJSON_ERROR_BUFFER_SIZE];
	if (!g_pYYJSONManager->PtrRemove(handle, path, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_try_get_val(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	YYJSONValue* pYYJSONValue = g_pYYJSONManager->PtrTryGet(handle, path);

	if (!pYYJSONValue) {
		return 0;
	}

	return CreateAndAssignHandle(pContext, pYYJSONValue, params[3], "JSON pointer value");
}

static cell_t json_ptr_try_get_bool(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	bool value;
	if (!g_pYYJSONManager->PtrTryGetBool(handle, path, &value)) {
		return 0;
	}

	cell_t* addr;
	pContext->LocalToPhysAddr(params[3], &addr);
	*addr = value;

	return 1;
}

static cell_t json_ptr_try_get_float(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	double value;
	if (!g_pYYJSONManager->PtrTryGetFloat(handle, path, &value)) {
		return 0;
	}

	cell_t* addr;
	pContext->LocalToPhysAddr(params[3], &addr);
	*addr = sp_ftoc(static_cast<float>(value));

	return 1;
}

static cell_t json_ptr_try_get_int(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	int value;
	if (!g_pYYJSONManager->PtrTryGetInt(handle, path, &value)) {
		return 0;
	}

	cell_t* addr;
	pContext->LocalToPhysAddr(params[3], &addr);
	*addr = value;

	return 1;
}

static cell_t json_ptr_try_get_integer64(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	int64_t value;
	if (!g_pYYJSONManager->PtrTryGetInt64(handle, path, &value)) {
		return 0;
	}

	size_t maxlen = static_cast<size_t>(params[4]);
	char result[21];
	snprintf(result, sizeof(result), "%" PRId64, value);
	pContext->StringToLocalUTF8(params[3], maxlen, result, nullptr);
	return 1;
}

static cell_t json_ptr_try_get_str(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	const char* str = nullptr;
	size_t len = 0;

	if (!g_pYYJSONManager->PtrTryGetString(handle, path, &str, &len)) {
		return 0;
	}

	size_t maxlen = static_cast<size_t>(params[4]);
	if (len + 1 > maxlen) {
		return 0;
	}

	pContext->StringToLocalUTF8(params[3], maxlen, str, nullptr);

	return 1;
}

static cell_t json_obj_foreach(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	const char* key = nullptr;
	YYJSONValue* pYYJSONValue = nullptr;

	if (!g_pYYJSONManager->ObjectForeachNext(handle, &key, nullptr, &pYYJSONValue)) {
		return false;
	}

	pContext->StringToLocalUTF8(params[2], params[3], key, nullptr);

	return CreateAndAssignHandle(pContext, pYYJSONValue, params[4], "JSON object value");
}

static cell_t json_arr_foreach(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	size_t index = 0;
	YYJSONValue* pYYJSONValue = nullptr;

	if (!g_pYYJSONManager->ArrayForeachNext(handle, &index, &pYYJSONValue)) {
		return false;
	}

	cell_t* indexPtr;
	pContext->LocalToPhysAddr(params[2], &indexPtr);
	*indexPtr = static_cast<cell_t>(index);

	return CreateAndAssignHandle(pContext, pYYJSONValue, params[3], "JSON array value");
}

static cell_t json_obj_foreach_key(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	const char* key = nullptr;

	if (!g_pYYJSONManager->ObjectForeachKeyNext(handle, &key, nullptr)) {
		return false;
	}

	pContext->StringToLocalUTF8(params[2], params[3], key, nullptr);

	return true;
}

static cell_t json_arr_foreach_index(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	size_t index = 0;

	if (!g_pYYJSONManager->ArrayForeachIndexNext(handle, &index)) {
		return false;
	}

	cell_t* indexPtr;
	pContext->LocalToPhysAddr(params[2], &indexPtr);
	*indexPtr = static_cast<cell_t>(index);

	return true;
}

static cell_t json_arr_sort(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot sort an immutable JSON array");
	}

	YYJSON_SORT_ORDER sort_mode = static_cast<YYJSON_SORT_ORDER>(params[2]);
	if (sort_mode < YYJSON_SORT_ASC || sort_mode > YYJSON_SORT_RANDOM) {
		return pContext->ThrowNativeError("Invalid sort mode: %d (expected 0=ascending, 1=descending, 2=random)", sort_mode);
	}

	return g_pYYJSONManager->ArraySort(handle, sort_mode);
}

static cell_t json_obj_sort(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot sort an immutable JSON object");
	}

	YYJSON_SORT_ORDER sort_mode = static_cast<YYJSON_SORT_ORDER>(params[2]);
	if (sort_mode < YYJSON_SORT_ASC || sort_mode > YYJSON_SORT_RANDOM) {
		return pContext->ThrowNativeError("Invalid sort mode: %d (expected 0=ascending, 1=descending, 2=random)", sort_mode);
	}

	return g_pYYJSONManager->ObjectSort(handle, sort_mode);
}

static cell_t json_doc_to_mutable(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (handle->IsMutable()) {
		return pContext->ThrowNativeError("Document is already mutable");
	}

	YYJSONValue* pYYJSONValue = g_pYYJSONManager->ToMutable(handle);

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError("Failed to convert to mutable document");
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "mutable JSON document");
}

static cell_t json_doc_to_immutable(IPluginContext* pContext, const cell_t* params)
{
	YYJSONValue* handle = g_pYYJSONManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Document is already immutable");
	}

	YYJSONValue* pYYJSONValue = g_pYYJSONManager->ToImmutable(handle);

	if (!pYYJSONValue) {
		return pContext->ThrowNativeError("Failed to convert to immutable document");
	}

	return CreateAndReturnHandle(pContext, pYYJSONValue, "immutable JSON document");
}

const sp_nativeinfo_t json_natives[] =
{
	// JSONObject
	{"YYJSONObject.YYJSONObject", json_obj_init},
	{"YYJSONObject.FromStrings", json_obj_init_with_str},
	{"YYJSONObject.Size.get", json_obj_get_size},
	{"YYJSONObject.Get", json_obj_get_val},
	{"YYJSONObject.GetBool", json_obj_get_bool},
	{"YYJSONObject.GetFloat", json_obj_get_float},
	{"YYJSONObject.GetInt", json_obj_get_int},
	{"YYJSONObject.GetInt64", json_obj_get_integer64},
	{"YYJSONObject.GetString", json_obj_get_str},
	{"YYJSONObject.IsNull", json_obj_is_null},
	{"YYJSONObject.GetKey", json_obj_get_key},
	{"YYJSONObject.GetValueAt", json_obj_get_val_at},
	{"YYJSONObject.HasKey", json_obj_has_key},
	{"YYJSONObject.RenameKey", json_obj_rename_key},
	{"YYJSONObject.Set", json_obj_set_val},
	{"YYJSONObject.SetBool", json_obj_set_bool},
	{"YYJSONObject.SetFloat", json_obj_set_float},
	{"YYJSONObject.SetInt", json_obj_set_int},
	{"YYJSONObject.SetInt64", json_obj_set_integer64},
	{"YYJSONObject.SetNull", json_obj_set_null},
	{"YYJSONObject.SetString", json_obj_set_str},
	{"YYJSONObject.Remove", json_obj_remove},
	{"YYJSONObject.Clear", json_obj_clear},
	{"YYJSONObject.FromString", json_obj_parse_str},
	{"YYJSONObject.FromFile", json_obj_parse_file},
	{"YYJSONObject.Sort", json_obj_sort},

	// JSONArray
	{"YYJSONArray.YYJSONArray", json_arr_init},
	{"YYJSONArray.FromStrings", json_arr_init_with_str},
	{"YYJSONArray.Length.get", json_arr_get_size},
	{"YYJSONArray.Get", json_arr_get_val},
	{"YYJSONArray.First.get", json_arr_get_first},
	{"YYJSONArray.Last.get", json_arr_get_last},
	{"YYJSONArray.GetBool", json_arr_get_bool},
	{"YYJSONArray.GetFloat", json_arr_get_float},
	{"YYJSONArray.GetInt", json_arr_get_integer},
	{"YYJSONArray.GetInt64", json_arr_get_integer64},
	{"YYJSONArray.GetString", json_arr_get_str},
	{"YYJSONArray.IsNull", json_arr_is_null},
	{"YYJSONArray.Set", json_arr_replace_val},
	{"YYJSONArray.SetBool", json_arr_replace_bool},
	{"YYJSONArray.SetFloat", json_arr_replace_float},
	{"YYJSONArray.SetInt", json_arr_replace_integer},
	{"YYJSONArray.SetInt64", json_arr_replace_integer64},
	{"YYJSONArray.SetNull", json_arr_replace_null},
	{"YYJSONArray.SetString", json_arr_replace_str},
	{"YYJSONArray.Push", json_arr_append_val},
	{"YYJSONArray.PushBool", json_arr_append_bool},
	{"YYJSONArray.PushFloat", json_arr_append_float},
	{"YYJSONArray.PushInt", json_arr_append_int},
	{"YYJSONArray.PushInt64", json_arr_append_integer64},
	{"YYJSONArray.PushNull", json_arr_append_null},
	{"YYJSONArray.PushString", json_arr_append_str},
	{"YYJSONArray.Remove", json_arr_remove},
	{"YYJSONArray.RemoveFirst", json_arr_remove_first},
	{"YYJSONArray.RemoveLast", json_arr_remove_last},
	{"YYJSONArray.RemoveRange", json_arr_remove_range},
	{"YYJSONArray.Clear", json_arr_clear},
	{"YYJSONArray.FromString", json_arr_parse_str},
	{"YYJSONArray.FromFile", json_arr_parse_file},
	{"YYJSONArray.IndexOfBool", json_arr_index_of_bool},
	{"YYJSONArray.IndexOfString", json_arr_index_of_str},
	{"YYJSONArray.IndexOfInt", json_arr_index_of_int},
	{"YYJSONArray.IndexOfInt64", json_arr_index_of_integer64},
	{"YYJSONArray.IndexOfFloat", json_arr_index_of_float},
	{"YYJSONArray.Sort", json_arr_sort},

	// JSON
	{"YYJSON.ToString", json_doc_write_to_str},
	{"YYJSON.ToFile", json_doc_write_to_file},
	{"YYJSON.Parse", json_doc_parse},
	{"YYJSON.Equals", json_doc_equals},
	{"YYJSON.DeepCopy", json_doc_copy_deep},
	{"YYJSON.GetTypeDesc", json_val_get_type_desc},
	{"YYJSON.GetSerializedSize", json_val_get_serialized_size},
	{"YYJSON.ReadSize.get", json_val_get_read_size},
	{"YYJSON.Type.get", json_val_get_type},
	{"YYJSON.SubType.get", json_val_get_subtype},
	{"YYJSON.IsArray.get", json_val_is_array},
	{"YYJSON.IsObject.get", json_val_is_object},
	{"YYJSON.IsInt.get", json_val_is_int},
	{"YYJSON.IsUint.get", json_val_is_uint},
	{"YYJSON.IsSint.get", json_val_is_sint},
	{"YYJSON.IsNum.get", json_val_is_num},
	{"YYJSON.IsBool.get", json_val_is_bool},
	{"YYJSON.IsTrue.get", json_val_is_true},
	{"YYJSON.IsFalse.get", json_val_is_false},
	{"YYJSON.IsFloat.get", json_val_is_float},
	{"YYJSON.IsStr.get", json_val_is_str},
	{"YYJSON.IsNull.get", json_val_is_null},
	{"YYJSON.IsCtn.get", json_val_is_ctn},
	{"YYJSON.IsMutable.get", json_val_is_mutable},
	{"YYJSON.IsImmutable.get", json_val_is_immutable},
	{"YYJSON.ForeachObject", json_obj_foreach},
	{"YYJSON.ForeachArray", json_arr_foreach},
	{"YYJSON.ForeachKey", json_obj_foreach_key},
	{"YYJSON.ForeachIndex", json_arr_foreach_index},
	{"YYJSON.ToMutable", json_doc_to_mutable},
	{"YYJSON.ToImmutable", json_doc_to_immutable},

	// JSON CREATE & GET
	{"YYJSON.Pack", json_val_pack},
	{"YYJSON.CreateBool", json_val_create_bool},
	{"YYJSON.CreateFloat", json_val_create_float},
	{"YYJSON.CreateInt", json_val_create_int},
	{"YYJSON.CreateInt64", json_val_create_integer64},
	{"YYJSON.CreateNull", json_val_create_null},
	{"YYJSON.CreateString", json_val_create_str},
	{"YYJSON.GetBool", json_val_get_bool},
	{"YYJSON.GetFloat", json_val_get_float},
	{"YYJSON.GetInt", json_val_get_int},
	{"YYJSON.GetInt64", json_val_get_integer64},
	{"YYJSON.GetString", json_val_get_str},

	// JSON POINTER
	{"YYJSON.PtrGet", json_ptr_get_val},
	{"YYJSON.PtrGetBool", json_ptr_get_bool},
	{"YYJSON.PtrGetFloat", json_ptr_get_float},
	{"YYJSON.PtrGetInt", json_ptr_get_int},
	{"YYJSON.PtrGetInt64", json_ptr_get_integer64},
	{"YYJSON.PtrGetString", json_ptr_get_str},
	{"YYJSON.PtrGetIsNull", json_ptr_get_is_null},
	{"YYJSON.PtrGetLength", json_ptr_get_length},
	{"YYJSON.PtrSet", json_ptr_set_val},
	{"YYJSON.PtrSetBool", json_ptr_set_bool},
	{"YYJSON.PtrSetFloat", json_ptr_set_float},
	{"YYJSON.PtrSetInt", json_ptr_set_int},
	{"YYJSON.PtrSetInt64", json_ptr_set_integer64},
	{"YYJSON.PtrSetString", json_ptr_set_str},
	{"YYJSON.PtrSetNull", json_ptr_set_null},
	{"YYJSON.PtrAdd", json_ptr_add_val},
	{"YYJSON.PtrAddBool", json_ptr_add_bool},
	{"YYJSON.PtrAddFloat", json_ptr_add_float},
	{"YYJSON.PtrAddInt", json_ptr_add_int},
	{"YYJSON.PtrAddInt64", json_ptr_add_integer64},
	{"YYJSON.PtrAddString", json_ptr_add_str},
	{"YYJSON.PtrAddNull", json_ptr_add_null},
	{"YYJSON.PtrRemove", json_ptr_remove_val},
	{"YYJSON.PtrTryGetVal", json_ptr_try_get_val},
	{"YYJSON.PtrTryGetBool", json_ptr_try_get_bool},
	{"YYJSON.PtrTryGetFloat", json_ptr_try_get_float},
	{"YYJSON.PtrTryGetInt", json_ptr_try_get_int},
	{"YYJSON.PtrTryGetInt64", json_ptr_try_get_integer64},
	{"YYJSON.PtrTryGetString", json_ptr_try_get_str},
	{nullptr, nullptr}
};