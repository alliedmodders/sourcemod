#include "extension.h"
#include "JsonManager.h"

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
 * Helper function: Create a SourceMod handle for JsonValue and return it directly
 * Used by functions that return Handle_t
 *
 * @param pContext      Plugin context
 * @param pJSONValue    JSON value to wrap (will be released on failure)
 * @param error_context Descriptive context for error messages
 * @return Handle on success, throws native error on failure
 */
static cell_t CreateAndReturnHandle(IPluginContext* pContext, JsonValue* pJSONValue, const char* error_context)
{
	if (!pJSONValue) {
		return pContext->ThrowNativeError("Failed to create %s", error_context);
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	pJSONValue->m_handle = handlesys->CreateHandleEx(g_JsonType, pJSONValue, &sec, nullptr, &err);

	if (!pJSONValue->m_handle) {
		g_pJsonManager->Release(pJSONValue);
		return pContext->ThrowNativeError("Failed to create handle for %s (error code: %d)", error_context, err);
	}

	return pJSONValue->m_handle;
}

/**
 * Helper function: Create a SourceMod handle for JsonValue and assign to output parameter
 * Used by iterator functions (foreach) that assign handle via reference
 *
 * @param pContext      Plugin context
 * @param pJSONValue    JSON value to wrap (will be released on failure)
 * @param param_index   Parameter index for output handle
 * @param error_context Descriptive context for error messages
 * @return true on success, false on failure (throws native error)
 */
static bool CreateAndAssignHandle(IPluginContext* pContext, JsonValue* pJSONValue,
                                   cell_t param_index, const char* error_context)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	pJSONValue->m_handle = handlesys->CreateHandleEx(g_JsonType, pJSONValue, &sec, nullptr, &err);

	if (!pJSONValue->m_handle) {
		g_pJsonManager->Release(pJSONValue);
		pContext->ThrowNativeError("Failed to create handle for %s (error code: %d)", error_context, err);
		return false;
	}

	cell_t* valHandle;
	pContext->LocalToPhysAddr(param_index, &valHandle);
	*valHandle = pJSONValue->m_handle;
	return true;
}

/**
 * Helper function: Create a SourceMod handle for JsonArrIter and return it directly
 */
static cell_t CreateAndReturnArrIterHandle(IPluginContext* pContext, JsonArrIter* iter, const char* error_context)
{
	if (!iter) {
		return pContext->ThrowNativeError("Failed to create %s", error_context);
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	Handle_t handle = handlesys->CreateHandleEx(g_ArrIterType, iter, &sec, nullptr, &err);

	if (!handle) {
		g_pJsonManager->ReleaseArrIter(iter);
		return pContext->ThrowNativeError("Failed to create handle for %s (error code: %d)", error_context, err);
	}

	return handle;
}

/**
 * Helper function: Create a SourceMod handle for JsonObjIter and return it directly
 */
static cell_t CreateAndReturnObjIterHandle(IPluginContext* pContext, JsonObjIter* iter, const char* error_context)
{
	if (!iter) {
		return pContext->ThrowNativeError("Failed to create %s", error_context);
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());
	Handle_t handle = handlesys->CreateHandleEx(g_ObjIterType, iter, &sec, nullptr, &err);

	if (!handle) {
		g_pJsonManager->ReleaseObjIter(iter);
		return pContext->ThrowNativeError("Failed to create handle for %s (error code: %d)", error_context, err);
	}

	return handle;
}

static cell_t json_pack(IPluginContext* pContext, const cell_t* params) {
	char* fmt;
	pContext->LocalToString(params[1], &fmt);

	SourceModPackParamProvider provider(pContext, params, 2);

	char error[JSON_PACK_ERROR_SIZE];
	JsonValue* pJSONValue = g_pJsonManager->Pack(fmt, &provider, error, sizeof(error));

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Failed to pack JSON: %s", error);
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "packed JSON");
}

static cell_t json_doc_parse(IPluginContext* pContext, const cell_t* params)
{
	char* str;
	pContext->LocalToString(params[1], &str);

	bool is_file = params[2];
	bool is_mutable_doc = params[3];
	yyjson_read_flag read_flg = static_cast<yyjson_read_flag>(params[4]);

	char error[JSON_ERROR_BUFFER_SIZE];
	JsonValue* pJSONValue = g_pJsonManager->ParseJSON(str, is_file, is_mutable_doc, read_flg, error, sizeof(error));

	if (!pJSONValue) {
		return pContext->ThrowNativeError(error);
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "parsed JSON document");
}

static cell_t json_doc_equals(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle1 = g_pJsonManager->GetFromHandle(pContext, params[1]);
	JsonValue* handle2 = g_pJsonManager->GetFromHandle(pContext, params[2]);

	if (!handle1 || !handle2) return 0;

	return g_pJsonManager->Equals(handle1, handle2);
}

static cell_t json_doc_copy_deep(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* targetDoc = g_pJsonManager->GetFromHandle(pContext, params[1]);
	JsonValue* sourceValue = g_pJsonManager->GetFromHandle(pContext, params[2]);

	if (!targetDoc || !sourceValue) return 0;

	JsonValue* pJSONValue = g_pJsonManager->DeepCopy(targetDoc, sourceValue);

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Failed to copy JSON value");
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "copied JSON value");
}

static cell_t json_get_type_desc(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	const char* type_desc = g_pJsonManager->GetTypeDesc(handle);
	pContext->StringToLocalUTF8(params[2], params[3], type_desc, nullptr);

	return 1;
}

static cell_t json_obj_parse_str(IPluginContext* pContext, const cell_t* params)
{
	char* str;
	pContext->LocalToString(params[1], &str);
	yyjson_read_flag read_flg = static_cast<yyjson_read_flag>(params[2]);

	char error[JSON_PACK_ERROR_SIZE];
	JsonValue* pJSONValue = g_pJsonManager->ObjectParseString(str, read_flg, error, sizeof(error));

	if (!pJSONValue) {
		return pContext->ThrowNativeError(error);
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "JSON object from string");
}

static cell_t json_obj_parse_file(IPluginContext* pContext, const cell_t* params)
{
	char* path;
	pContext->LocalToString(params[1], &path);
	yyjson_read_flag read_flg = static_cast<yyjson_read_flag>(params[2]);

	char error[JSON_PACK_ERROR_SIZE];
	JsonValue* pJSONValue = g_pJsonManager->ObjectParseFile(path, read_flg, error, sizeof(error));

	if (!pJSONValue) {
		return pContext->ThrowNativeError(error);
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "JSON object from file");
}

static cell_t json_arr_parse_str(IPluginContext* pContext, const cell_t* params)
{
	char* str;
	pContext->LocalToString(params[1], &str);
	yyjson_read_flag read_flg = static_cast<yyjson_read_flag>(params[2]);

	char error[JSON_PACK_ERROR_SIZE];
	JsonValue* pJSONValue = g_pJsonManager->ArrayParseString(str, read_flg, error, sizeof(error));

	if (!pJSONValue) {
		return pContext->ThrowNativeError(error);
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "JSON array from string");
}

static cell_t json_arr_parse_file(IPluginContext* pContext, const cell_t* params)
{
	char* path;
	pContext->LocalToString(params[1], &path);
	yyjson_read_flag read_flg = static_cast<yyjson_read_flag>(params[2]);

	char error[JSON_PACK_ERROR_SIZE];
	JsonValue* pJSONValue = g_pJsonManager->ArrayParseFile(path, read_flg, error, sizeof(error));

	if (!pJSONValue) {
		return pContext->ThrowNativeError(error);
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "JSON array from file");
}

static cell_t json_arr_index_of_bool(IPluginContext *pContext, const cell_t *params)
{
	JsonValue *handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	bool searchValue = params[2];
	return g_pJsonManager->ArrayIndexOfBool(handle, searchValue);
}

static cell_t json_arr_index_of_str(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* searchStr;
	pContext->LocalToString(params[2], &searchStr);

	return g_pJsonManager->ArrayIndexOfString(handle, searchStr);
}

static cell_t json_arr_index_of_int(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	int searchValue = params[2];
	return g_pJsonManager->ArrayIndexOfInt(handle, searchValue);
}

static cell_t json_arr_index_of_integer64(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* searchStr;
	pContext->LocalToString(params[2], &searchStr);

	char* endptr;
	errno = 0;
	long long searchValue = strtoll(searchStr, &endptr, 10);

	if (errno == ERANGE || *endptr != '\0') {
		return pContext->ThrowNativeError("Invalid integer64 value: %s", searchStr);
	}

	return g_pJsonManager->ArrayIndexOfInt64(handle, searchValue);
}

static cell_t json_arr_index_of_uint64(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* searchStr;
	pContext->LocalToString(params[2], &searchStr);

	char* endptr;
	errno = 0;
	unsigned long long searchValue = strtoull(searchStr, &endptr, 10);

	if (errno == ERANGE || *endptr != '\0') {
		return pContext->ThrowNativeError("Invalid unsigned integer64 value: %s", searchStr);
	}

	return g_pJsonManager->ArrayIndexOfUint64(handle, searchValue);
}

static cell_t json_arr_index_of_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	double searchValue = static_cast<double>(sp_ctof(params[2]));
	return g_pJsonManager->ArrayIndexOfFloat(handle, searchValue);
}

static cell_t json_get_type(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->GetType(handle);
}

static cell_t json_get_subtype(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->GetSubtype(handle);
}

static cell_t json_is_array(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->IsArray(handle);
}

static cell_t json_is_object(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->IsObject(handle);
}

static cell_t json_is_int(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->IsInt(handle);
}

static cell_t json_is_uint(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->IsUint(handle);
}

static cell_t json_is_sint(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->IsSint(handle);
}

static cell_t json_is_num(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->IsNum(handle);
}

static cell_t json_is_bool(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->IsBool(handle);
}

static cell_t json_is_true(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->IsTrue(handle);
}

static cell_t json_is_false(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->IsFalse(handle);
}

static cell_t json_is_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->IsFloat(handle);
}

static cell_t json_is_str(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->IsStr(handle);
}

static cell_t json_is_null(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->IsNull(handle);
}

static cell_t json_is_ctn(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->IsCtn(handle);
}

static cell_t json_is_mutable(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->IsMutable(handle);
}

static cell_t json_is_immutable(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	return g_pJsonManager->IsImmutable(handle);
}

static cell_t json_obj_init(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* pJSONValue = g_pJsonManager->ObjectInit();
	return CreateAndReturnHandle(pContext, pJSONValue, "JSON object");
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

	JsonValue* pJSONValue = g_pJsonManager->ObjectInitWithStrings(kv_pairs.data(), array_size / 2);

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Failed to create JSON object from key-value pairs");
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "JSON object from key-value pairs");
}

static cell_t json_create_bool(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* pJSONValue = g_pJsonManager->CreateBool(params[1]);
	return CreateAndReturnHandle(pContext, pJSONValue, "JSON boolean value");
}

static cell_t json_create_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* pJSONValue = g_pJsonManager->CreateFloat(sp_ctof(params[1]));
	return CreateAndReturnHandle(pContext, pJSONValue, "JSON float value");
}

static cell_t json_create_int(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* pJSONValue = g_pJsonManager->CreateInt(params[1]);
	return CreateAndReturnHandle(pContext, pJSONValue, "JSON integer value");
}

static cell_t json_create_integer64(IPluginContext* pContext, const cell_t* params)
{
	char* value;
	pContext->LocalToString(params[1], &value);

	std::variant<int64_t, uint64_t> variant_value;
	char error[JSON_ERROR_BUFFER_SIZE];
	if (!g_pJsonManager->ParseInt64Variant(value, &variant_value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	JsonValue* pJSONValue = g_pJsonManager->CreateInt64(variant_value);

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Failed to create JSON integer64 value");
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "JSON integer64 value");
}

static cell_t json_create_str(IPluginContext* pContext, const cell_t* params)
{
	char* str;
	pContext->LocalToString(params[1], &str);

	JsonValue* pJSONValue = g_pJsonManager->CreateString(str);

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Failed to create JSON string value");
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "JSON string value");
}

static cell_t json_get_bool(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	bool value;
	if (!g_pJsonManager->GetBool(handle, &value)) {
		return pContext->ThrowNativeError("Type mismatch: expected boolean value");
	}

	return value;
}

static cell_t json_get_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	double value;
	if (!g_pJsonManager->GetFloat(handle, &value)) {
		return pContext->ThrowNativeError("Type mismatch: expected float value");
	}

	return sp_ftoc(static_cast<float>(value));
}

static cell_t json_get_int(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	int value;
	if (!g_pJsonManager->GetInt(handle, &value)) {
		return pContext->ThrowNativeError("Type mismatch: expected integer value");
	}

	return value;
}

static cell_t json_get_integer64(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	std::variant<int64_t, uint64_t> value;
	if (!g_pJsonManager->GetInt64(handle, &value)) {
		return pContext->ThrowNativeError("Type mismatch: expected integer64 value");
	}

	char result[JSON_INT64_BUFFER_SIZE];
	if (std::holds_alternative<uint64_t>(value)) {
		snprintf(result, sizeof(result), "%" PRIu64, std::get<uint64_t>(value));
	} else {
		snprintf(result, sizeof(result), "%" PRId64, std::get<int64_t>(value));
	}
	pContext->StringToLocalUTF8(params[2], params[3], result, nullptr);

	return 1;
}

static cell_t json_get_str(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	const char* str = nullptr;
	size_t len = 0;

	if (!g_pJsonManager->GetString(handle, &str, &len)) {
		return pContext->ThrowNativeError("Type mismatch: expected string value");
	}

	size_t maxlen = static_cast<size_t>(params[3]);

	if (len + 1 > maxlen) {
		return pContext->ThrowNativeError("Buffer is too small (need %d, have %d)", len + 1, maxlen);
	}

	pContext->StringToLocalUTF8(params[2], maxlen, str, nullptr);

	return 1;
}

static cell_t json_equals_str(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	char* str;
	pContext->LocalToString(params[2], &str);

	return g_pJsonManager->EqualsStr(handle, str);
}

static cell_t json_get_serialized_size(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	yyjson_write_flag write_flg = static_cast<yyjson_write_flag>(params[2]);
	size_t size = g_pJsonManager->GetSerializedSize(handle, write_flg);

	return static_cast<cell_t>(size);
}

static cell_t json_get_read_size(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t size = g_pJsonManager->GetReadSize(handle);
	if (size == 0) return 0;

	return static_cast<cell_t>(size);
}

static cell_t json_create_null(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* pJSONValue = g_pJsonManager->CreateNull();
	return CreateAndReturnHandle(pContext, pJSONValue, "JSON null value");
}

static cell_t json_arr_init(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* pJSONValue = g_pJsonManager->ArrayInit();
	return CreateAndReturnHandle(pContext, pJSONValue, "JSON array");
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

	JsonValue* pJSONValue = g_pJsonManager->ArrayInitWithStrings(strs.data(), strs.size());

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Failed to create JSON array from strings");
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "JSON array from strings");
}

static cell_t json_arr_init_with_int32(IPluginContext* pContext, const cell_t* params)
{
	cell_t* addr;
	pContext->LocalToPhysAddr(params[1], &addr);
	cell_t array_size = params[2];

	std::vector<int32_t> values;
	values.reserve(array_size);

	for (cell_t i = 0; i < array_size; i++) {
		values.push_back(static_cast<int32_t>(addr[i]));
	}

	JsonValue* pJSONValue = g_pJsonManager->ArrayInitWithInt32(values.data(), values.size());

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Failed to create JSON array from int32 values");
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "JSON array from int32 values");
}

static cell_t json_arr_init_with_int64(IPluginContext* pContext, const cell_t* params)
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

	char error[JSON_ERROR_BUFFER_SIZE];
	JsonValue* pJSONValue = g_pJsonManager->ArrayInitWithInt64(strs.data(), strs.size(), error, sizeof(error));

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Failed to create JSON array from int64 values: %s", error);
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "JSON array from int64 values");
}

static cell_t json_arr_init_with_bool(IPluginContext* pContext, const cell_t* params)
{
	cell_t* addr;
	pContext->LocalToPhysAddr(params[1], &addr);
	cell_t array_size = params[2];

	// std::vector<bool> is specialized and doesn't work with .data() so we use a unique_ptr
	auto values = std::make_unique<bool[]>(array_size);

	for (cell_t i = 0; i < array_size; i++) {
		values[i] = (addr[i] != 0);
	}

	JsonValue* pJSONValue = g_pJsonManager->ArrayInitWithBool(values.get(), array_size);

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Failed to create JSON array from bool values");
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "JSON array from bool values");
}

static cell_t json_arr_init_with_float(IPluginContext* pContext, const cell_t* params)
{
	cell_t* addr;
	pContext->LocalToPhysAddr(params[1], &addr);
	cell_t array_size = params[2];

	std::vector<double> values;
	values.reserve(array_size);

	for (cell_t i = 0; i < array_size; i++) {
		values.push_back(sp_ctof(addr[i]));
	}

	JsonValue* pJSONValue = g_pJsonManager->ArrayInitWithFloat(values.data(), values.size());

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Failed to create JSON array from float values");
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "JSON array from float values");
}

static cell_t json_arr_get_size(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t size = g_pJsonManager->ArrayGetSize(handle);
	return static_cast<cell_t>(size);
}

static cell_t json_arr_get_val(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);

	JsonValue* pJSONValue = g_pJsonManager->ArrayGet(handle, index);

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Index %d is out of bounds", index);
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "JSON array value");
}

static cell_t json_arr_get_first(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	JsonValue* pJSONValue = g_pJsonManager->ArrayGetFirst(handle);

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Array is empty");
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "first JSON array value");
}

static cell_t json_arr_get_last(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	JsonValue* pJSONValue = g_pJsonManager->ArrayGetLast(handle);

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Array is empty");
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "last JSON array value");
}

static cell_t json_arr_get_bool(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);

	bool value;
	if (!g_pJsonManager->ArrayGetBool(handle, index, &value)) {
		return pContext->ThrowNativeError("Failed to get boolean at index %d", index);
	}

	return value;
}

static cell_t json_arr_get_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);

	double value;
	if (!g_pJsonManager->ArrayGetFloat(handle, index, &value)) {
		return pContext->ThrowNativeError("Failed to get float at index %d", index);
	}

	return sp_ftoc(static_cast<float>(value));
}

static cell_t json_arr_get_integer(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);

	int value;
	if (!g_pJsonManager->ArrayGetInt(handle, index, &value)) {
		return pContext->ThrowNativeError("Failed to get integer at index %d", index);
	}

	return value;
}

static cell_t json_arr_get_integer64(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);
	std::variant<int64_t, uint64_t> value;

	if (!g_pJsonManager->ArrayGetInt64(handle, index, &value)) {
		return pContext->ThrowNativeError("Failed to get integer64 at index %d", index);
	}

	char result[JSON_INT64_BUFFER_SIZE];
	if (std::holds_alternative<uint64_t>(value)) {
		snprintf(result, sizeof(result), "%" PRIu64, std::get<uint64_t>(value));
	} else {
		snprintf(result, sizeof(result), "%" PRId64, std::get<int64_t>(value));
	}
	pContext->StringToLocalUTF8(params[3], params[4], result, nullptr);

	return 1;
}

static cell_t json_arr_get_str(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);
	const char* str = nullptr;
	size_t len = 0;

	if (!g_pJsonManager->ArrayGetString(handle, index, &str, &len)) {
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
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);

	return g_pJsonManager->ArrayIsNull(handle, index);
}

static cell_t json_arr_replace_val(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle1 = g_pJsonManager->GetFromHandle(pContext, params[1]);
	JsonValue* handle2 = g_pJsonManager->GetFromHandle(pContext, params[3]);

	if (!handle1 || !handle2) return 0;

	if (!handle1->IsMutable()) {
		return pContext->ThrowNativeError("Cannot replace value in an immutable JSON array");
	}

	size_t index = static_cast<size_t>(params[2]);
	return g_pJsonManager->ArrayReplace(handle1, index, handle2);
}

static cell_t json_arr_replace_bool(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot replace value in an immutable JSON array");
	}

	size_t index = static_cast<size_t>(params[2]);
	return g_pJsonManager->ArrayReplaceBool(handle, index, params[3]);
}

static cell_t json_arr_replace_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot replace value in an immutable JSON array");
	}

	size_t index = static_cast<size_t>(params[2]);
	return g_pJsonManager->ArrayReplaceFloat(handle, index, sp_ctof(params[3]));
}

static cell_t json_arr_replace_integer(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot replace value in an immutable JSON array");
	}

	size_t index = static_cast<size_t>(params[2]);
	return g_pJsonManager->ArrayReplaceInt(handle, index, params[3]);
}

static cell_t json_arr_replace_integer64(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot replace value in an immutable JSON array");
	}

	char* value;
	pContext->LocalToString(params[3], &value);

	std::variant<int64_t, uint64_t> variant_value;
	char error[JSON_ERROR_BUFFER_SIZE];
	if (!g_pJsonManager->ParseInt64Variant(value, &variant_value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	size_t index = static_cast<size_t>(params[2]);
	return g_pJsonManager->ArrayReplaceInt64(handle, index, variant_value);
}

static cell_t json_arr_replace_null(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot replace value in an immutable JSON array");
	}

	size_t index = static_cast<size_t>(params[2]);
	return g_pJsonManager->ArrayReplaceNull(handle, index);
}

static cell_t json_arr_replace_str(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot replace value in an immutable JSON array");
	}

	char* val;
	pContext->LocalToString(params[3], &val);

	size_t index = static_cast<size_t>(params[2]);
	return g_pJsonManager->ArrayReplaceString(handle, index, val);
}

static cell_t json_arr_append_val(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle1 = g_pJsonManager->GetFromHandle(pContext, params[1]);
	JsonValue* handle2 = g_pJsonManager->GetFromHandle(pContext, params[2]);

	if (!handle1 || !handle2) return 0;

	if (!handle1->IsMutable()) {
		return pContext->ThrowNativeError("Cannot append value to an immutable JSON array");
	}

	return g_pJsonManager->ArrayAppend(handle1, handle2);
}

static cell_t json_arr_append_bool(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot append value to an immutable JSON array");
	}

	return g_pJsonManager->ArrayAppendBool(handle, params[2]);
}

static cell_t json_arr_append_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot append value to an immutable JSON array");
	}

	return g_pJsonManager->ArrayAppendFloat(handle, sp_ctof(params[2]));
}

static cell_t json_arr_append_int(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot append value to an immutable JSON array");
	}

	return g_pJsonManager->ArrayAppendInt(handle, params[2]);
}

static cell_t json_arr_append_integer64(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot append value to an immutable JSON array");
	}

	char* value;
	pContext->LocalToString(params[2], &value);

	std::variant<int64_t, uint64_t> variant_value;
	char error[JSON_ERROR_BUFFER_SIZE];
	if (!g_pJsonManager->ParseInt64Variant(value, &variant_value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return g_pJsonManager->ArrayAppendInt64(handle, variant_value);
}

static cell_t json_arr_append_null(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot append value to an immutable JSON array");
	}

	return g_pJsonManager->ArrayAppendNull(handle);
}

static cell_t json_arr_append_str(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot append value to an immutable JSON array");
	}

	char* str;
	pContext->LocalToString(params[2], &str);

	return g_pJsonManager->ArrayAppendString(handle, str);
}

static cell_t json_arr_insert(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot insert value into an immutable JSON array");
	}

	size_t index = params[2];
	JsonValue* value = g_pJsonManager->GetFromHandle(pContext, params[3]);
	if (!value) return 0;

	return g_pJsonManager->ArrayInsert(handle, index, value);
}

static cell_t json_arr_insert_bool(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot insert value into an immutable JSON array");
	}

	size_t index = params[2];
	bool value = params[3] != 0;

	return g_pJsonManager->ArrayInsertBool(handle, index, value);
}

static cell_t json_arr_insert_int(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot insert value into an immutable JSON array");
	}

	size_t index = params[2];
	int value = params[3];

	return g_pJsonManager->ArrayInsertInt(handle, index, value);
}

static cell_t json_arr_insert_int64(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot insert value into an immutable JSON array");
	}

	size_t index = params[2];
	char* value;
	pContext->LocalToString(params[3], &value);

	std::variant<int64_t, uint64_t> variant_value;
	char error[JSON_ERROR_BUFFER_SIZE];
	if (!g_pJsonManager->ParseInt64Variant(value, &variant_value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return g_pJsonManager->ArrayInsertInt64(handle, index, variant_value);
}

static cell_t json_arr_insert_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot insert value into an immutable JSON array");
	}

	size_t index = params[2];
	double value = sp_ctof(params[3]);

	return g_pJsonManager->ArrayInsertFloat(handle, index, value);
}

static cell_t json_arr_insert_str(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot insert value into an immutable JSON array");
	}

	size_t index = params[2];
	char* str;
	pContext->LocalToString(params[3], &str);

	return g_pJsonManager->ArrayInsertString(handle, index, str);
}

static cell_t json_arr_insert_null(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot insert value into an immutable JSON array");
	}

	size_t index = params[2];

	return g_pJsonManager->ArrayInsertNull(handle, index);
}

static cell_t json_arr_prepend(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot prepend value to an immutable JSON array");
	}

	JsonValue* value = g_pJsonManager->GetFromHandle(pContext, params[2]);
	if (!value) return 0;

	return g_pJsonManager->ArrayPrepend(handle, value);
}

static cell_t json_arr_prepend_bool(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot prepend value to an immutable JSON array");
	}

	bool value = params[2] != 0;

	return g_pJsonManager->ArrayPrependBool(handle, value);
}

static cell_t json_arr_prepend_int(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot prepend value to an immutable JSON array");
	}

	int value = params[2];

	return g_pJsonManager->ArrayPrependInt(handle, value);
}

static cell_t json_arr_prepend_int64(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot prepend value to an immutable JSON array");
	}

	char* value;
	pContext->LocalToString(params[2], &value);

	std::variant<int64_t, uint64_t> variant_value;
	char error[JSON_ERROR_BUFFER_SIZE];
	if (!g_pJsonManager->ParseInt64Variant(value, &variant_value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return g_pJsonManager->ArrayPrependInt64(handle, variant_value);
}

static cell_t json_arr_prepend_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot prepend value to an immutable JSON array");
	}

	double value = sp_ctof(params[2]);

	return g_pJsonManager->ArrayPrependFloat(handle, value);
}

static cell_t json_arr_prepend_str(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot prepend value to an immutable JSON array");
	}

	char* str;
	pContext->LocalToString(params[2], &str);

	return g_pJsonManager->ArrayPrependString(handle, str);
}

static cell_t json_arr_prepend_null(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot prepend value to an immutable JSON array");
	}

	return g_pJsonManager->ArrayPrependNull(handle);
}

static cell_t json_arr_remove(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot remove value from an immutable JSON array");
	}

	size_t index = static_cast<size_t>(params[2]);
	return g_pJsonManager->ArrayRemove(handle, index);
}

static cell_t json_arr_remove_first(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot remove value from an immutable JSON array");
	}

	return g_pJsonManager->ArrayRemoveFirst(handle);
}

static cell_t json_arr_remove_last(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot remove value from an immutable JSON array");
	}

	return g_pJsonManager->ArrayRemoveLast(handle);
}

static cell_t json_arr_remove_range(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot remove value from an immutable JSON array");
	}

	size_t start_index = static_cast<size_t>(params[2]);
	size_t end_index = static_cast<size_t>(params[3]);

	return g_pJsonManager->ArrayRemoveRange(handle, start_index, end_index);
}

static cell_t json_arr_clear(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot clear an immutable JSON array");
	}

	return g_pJsonManager->ArrayClear(handle);
}

static cell_t json_doc_write_to_str(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t buffer_size = static_cast<size_t>(params[3]);
	yyjson_write_flag write_flg = static_cast<yyjson_write_flag>(params[4]);

	char* temp_buffer = (char*)malloc(buffer_size);
	if (!temp_buffer) {
		return pContext->ThrowNativeError("Failed to allocate buffer");
	}

	size_t output_len = 0;
	if (!g_pJsonManager->WriteToString(handle, temp_buffer, buffer_size, write_flg, &output_len)) {
		free(temp_buffer);
		return pContext->ThrowNativeError("Buffer too small or write failed");
	}

	pContext->StringToLocalUTF8(params[2], buffer_size, temp_buffer, nullptr);
	free(temp_buffer);
	return static_cast<cell_t>(output_len);
}

static cell_t json_doc_write_to_file(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);
	yyjson_write_flag write_flg = static_cast<yyjson_write_flag>(params[3]);

	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->WriteToFile(handle, path, write_flg, error, sizeof(error))) {
		return pContext->ThrowNativeError(error);
	}

	return true;
}

static cell_t json_obj_get_size(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t size = g_pJsonManager->ObjectGetSize(handle);
	return static_cast<cell_t>(size);
}

static cell_t json_obj_get_key(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);
	const char* key = nullptr;

	if (!g_pJsonManager->ObjectGetKey(handle, index, &key)) {
		return pContext->ThrowNativeError("Index %d is out of bounds", index);
	}

	pContext->StringToLocalUTF8(params[3], params[4], key, nullptr);
	return 1;
}

static cell_t json_obj_get_val_at(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	size_t index = static_cast<size_t>(params[2]);

	JsonValue* pJSONValue = g_pJsonManager->ObjectGetValueAt(handle, index);

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Index %d is out of bounds", index);
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "JSON object value");
}

static cell_t json_obj_get_val(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	JsonValue* pJSONValue = g_pJsonManager->ObjectGet(handle, key);

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Key not found: %s", key);
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "JSON object value");
}

static cell_t json_obj_get_bool(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	bool value;
	if (!g_pJsonManager->ObjectGetBool(handle, key, &value)) {
		return pContext->ThrowNativeError("Failed to get boolean for key '%s'", key);
	}

	return value;
}

static cell_t json_obj_get_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	double value;
	if (!g_pJsonManager->ObjectGetFloat(handle, key, &value)) {
		return pContext->ThrowNativeError("Failed to get float for key '%s'", key);
	}

	return sp_ftoc(static_cast<float>(value));
}

static cell_t json_obj_get_int(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	int value;
	if (!g_pJsonManager->ObjectGetInt(handle, key, &value)) {
		return pContext->ThrowNativeError("Failed to get integer for key '%s'", key);
	}

	return value;
}

static cell_t json_obj_get_integer64(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	std::variant<int64_t, uint64_t> value;
	if (!g_pJsonManager->ObjectGetInt64(handle, key, &value)) {
		return pContext->ThrowNativeError("Failed to get integer64 for key '%s'", key);
	}

	char result[JSON_INT64_BUFFER_SIZE];
	if (std::holds_alternative<uint64_t>(value)) {
		snprintf(result, sizeof(result), "%" PRIu64, std::get<uint64_t>(value));
	} else {
		snprintf(result, sizeof(result), "%" PRId64, std::get<int64_t>(value));
	}
	pContext->StringToLocalUTF8(params[3], params[4], result, nullptr);

	return 1;
}

static cell_t json_obj_get_str(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	const char* str = nullptr;
	size_t len = 0;
	if (!g_pJsonManager->ObjectGetString(handle, key, &str, &len)) {
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
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot clear an immutable JSON object");
	}

	return g_pJsonManager->ObjectClear(handle);
}

static cell_t json_obj_is_null(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	bool is_null = false;
	if (!g_pJsonManager->ObjectIsNull(handle, key, &is_null)) {
		return pContext->ThrowNativeError("Key not found: %s", key);
	}

	return is_null;
}

static cell_t json_obj_has_key(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	bool ptr_use = params[3];

	return g_pJsonManager->ObjectHasKey(handle, key, ptr_use);
}

static cell_t json_obj_rename_key(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot rename key in an immutable JSON object");
	}

	char* old_key;
	pContext->LocalToString(params[2], &old_key);

	char* new_key;
	pContext->LocalToString(params[3], &new_key);

	bool allow_duplicate = params[4];

	if (!g_pJsonManager->ObjectRenameKey(handle, old_key, new_key, allow_duplicate)) {
		return pContext->ThrowNativeError("Failed to rename key from '%s' to '%s'", old_key, new_key);
	}

	return true;
}

static cell_t json_obj_set_val(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle1 = g_pJsonManager->GetFromHandle(pContext, params[1]);
	JsonValue* handle2 = g_pJsonManager->GetFromHandle(pContext, params[3]);

	if (!handle1 || !handle2) return 0;

	if (!handle1->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON object");
	}

	char* key;
	pContext->LocalToString(params[2], &key);

	return g_pJsonManager->ObjectSet(handle1, key, handle2);
}

static cell_t json_obj_set_bool(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON object");
	}

	char* key;
	pContext->LocalToString(params[2], &key);

	return g_pJsonManager->ObjectSetBool(handle, key, params[3]);
}

static cell_t json_obj_set_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON object");
	}

	char* key;
	pContext->LocalToString(params[2], &key);

	return g_pJsonManager->ObjectSetFloat(handle, key, sp_ctof(params[3]));
}

static cell_t json_obj_set_int(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON object");
	}

	char* key;
	pContext->LocalToString(params[2], &key);

	return g_pJsonManager->ObjectSetInt(handle, key, params[3]);
}

static cell_t json_obj_set_integer64(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON object");
	}

	char* key, * value;
	pContext->LocalToString(params[2], &key);
	pContext->LocalToString(params[3], &value);

	std::variant<int64_t, uint64_t> variant_value;
	char error[JSON_ERROR_BUFFER_SIZE];
	if (!g_pJsonManager->ParseInt64Variant(value, &variant_value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return g_pJsonManager->ObjectSetInt64(handle, key, variant_value);
}

static cell_t json_obj_set_null(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON object");
	}

	char* key;
	pContext->LocalToString(params[2], &key);

	return g_pJsonManager->ObjectSetNull(handle, key);
}

static cell_t json_obj_set_str(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON object");
	}

	char* key, * value;
	pContext->LocalToString(params[2], &key);
	pContext->LocalToString(params[3], &value);

	return g_pJsonManager->ObjectSetString(handle, key, value);
}

static cell_t json_obj_remove(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot remove value from an immutable JSON object");
	}

	char* key;
	pContext->LocalToString(params[2], &key);

	return g_pJsonManager->ObjectRemove(handle, key);
}

static cell_t json_ptr_get_val(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[JSON_PACK_ERROR_SIZE];
	JsonValue* pJSONValue = g_pJsonManager->PtrGet(handle, path, error, sizeof(error));

	if (!pJSONValue) {
		return pContext->ThrowNativeError("%s", error);
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "JSON pointer value");
}

static cell_t json_ptr_get_bool(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	bool value;
	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrGetBool(handle, path, &value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return value;
}

static cell_t json_ptr_get_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	double value;
	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrGetFloat(handle, path, &value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return sp_ftoc(static_cast<float>(value));
}

static cell_t json_ptr_get_int(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	int value;
	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrGetInt(handle, path, &value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return value;
}

static cell_t json_ptr_get_integer64(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	std::variant<int64_t, uint64_t> value;
	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrGetInt64(handle, path, &value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	char result[JSON_INT64_BUFFER_SIZE];
	if (std::holds_alternative<uint64_t>(value)) {
		snprintf(result, sizeof(result), "%" PRIu64, std::get<uint64_t>(value));
	} else {
		snprintf(result, sizeof(result), "%" PRId64, std::get<int64_t>(value));
	}
	pContext->StringToLocalUTF8(params[3], params[4], result, nullptr);

	return 1;
}

static cell_t json_ptr_get_str(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	const char* str = nullptr;
	size_t len = 0;
	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrGetString(handle, path, &str, &len, error, sizeof(error))) {
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
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	bool is_null;
	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrGetIsNull(handle, path, &is_null, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return is_null;
}

static cell_t json_ptr_get_length(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	size_t len;
	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrGetLength(handle, path, &len, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return static_cast<cell_t>(len);
}

static cell_t json_ptr_set_val(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle1 = g_pJsonManager->GetFromHandle(pContext, params[1]);
	JsonValue* handle2 = g_pJsonManager->GetFromHandle(pContext, params[3]);

	if (!handle1 || !handle2) return 0;

	if (!handle1->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrSet(handle1, path, handle2, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_set_bool(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrSetBool(handle, path, params[3], error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_set_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrSetFloat(handle, path, sp_ctof(params[3]), error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_set_int(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrSetInt(handle, path, params[3], error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_set_integer64(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON document using pointer");
	}

	char* path, * value;
	pContext->LocalToString(params[2], &path);
	pContext->LocalToString(params[3], &value);

	std::variant<int64_t, uint64_t> variant_value;
	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->ParseInt64Variant(value, &variant_value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	if (!g_pJsonManager->PtrSetInt64(handle, path, variant_value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_set_str(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON document using pointer");
	}

	char* path, * str;
	pContext->LocalToString(params[2], &path);
	pContext->LocalToString(params[3], &str);

	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrSetString(handle, path, str, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_set_null(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot set value in an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrSetNull(handle, path, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_add_val(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle1 = g_pJsonManager->GetFromHandle(pContext, params[1]);
	JsonValue* handle2 = g_pJsonManager->GetFromHandle(pContext, params[3]);

	if (!handle1 || !handle2) return 0;

	if (!handle1->IsMutable()) {
		return pContext->ThrowNativeError("Cannot add value to an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrAdd(handle1, path, handle2, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_add_bool(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot add value to an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrAddBool(handle, path, params[3], error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_add_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot add value to an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrAddFloat(handle, path, sp_ctof(params[3]), error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_add_int(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot add value to an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrAddInt(handle, path, params[3], error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_add_integer64(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot add value to an immutable JSON document using pointer");
	}

	char* path, * value;
	pContext->LocalToString(params[2], &path);
	pContext->LocalToString(params[3], &value);

	std::variant<int64_t, uint64_t> variant_value;
	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->ParseInt64Variant(value, &variant_value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	if (!g_pJsonManager->PtrAddInt64(handle, path, variant_value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_add_str(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot add value to an immutable JSON document using pointer");
	}

	char* path, * str;
	pContext->LocalToString(params[2], &path);
	pContext->LocalToString(params[3], &str);

	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrAddString(handle, path, str, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_add_null(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot add value to an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrAddNull(handle, path, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_remove_val(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot remove value from an immutable JSON document using pointer");
	}

	char* path;
	pContext->LocalToString(params[2], &path);

	char error[JSON_PACK_ERROR_SIZE];
	if (!g_pJsonManager->PtrRemove(handle, path, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	return true;
}

static cell_t json_ptr_try_get_val(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	JsonValue* pJSONValue = g_pJsonManager->PtrTryGet(handle, path);

	if (!pJSONValue) {
		return 0;
	}

	return CreateAndAssignHandle(pContext, pJSONValue, params[3], "JSON pointer value");
}

static cell_t json_ptr_try_get_bool(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	bool value;
	if (!g_pJsonManager->PtrTryGetBool(handle, path, &value)) {
		return 0;
	}

	cell_t* addr;
	pContext->LocalToPhysAddr(params[3], &addr);
	*addr = value;

	return 1;
}

static cell_t json_ptr_try_get_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	double value;
	if (!g_pJsonManager->PtrTryGetFloat(handle, path, &value)) {
		return 0;
	}

	cell_t* addr;
	pContext->LocalToPhysAddr(params[3], &addr);
	*addr = sp_ftoc(static_cast<float>(value));

	return 1;
}

static cell_t json_ptr_try_get_int(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	int value;
	if (!g_pJsonManager->PtrTryGetInt(handle, path, &value)) {
		return 0;
	}

	cell_t* addr;
	pContext->LocalToPhysAddr(params[3], &addr);
	*addr = value;

	return 1;
}

static cell_t json_ptr_try_get_integer64(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	std::variant<int64_t, uint64_t> value;
	if (!g_pJsonManager->PtrTryGetInt64(handle, path, &value)) {
		return 0;
	}

	size_t maxlen = static_cast<size_t>(params[4]);
	char result[JSON_INT64_BUFFER_SIZE];
	if (std::holds_alternative<uint64_t>(value)) {
		snprintf(result, sizeof(result), "%" PRIu64, std::get<uint64_t>(value));
	} else {
		snprintf(result, sizeof(result), "%" PRId64, std::get<int64_t>(value));
	}
	pContext->StringToLocalUTF8(params[3], maxlen, result, nullptr);
	return 1;
}

static cell_t json_ptr_try_get_str(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	char* path;
	pContext->LocalToString(params[2], &path);

	const char* str = nullptr;
	size_t len = 0;

	if (!g_pJsonManager->PtrTryGetString(handle, path, &str, &len)) {
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
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	const char* key = nullptr;
	JsonValue* pJSONValue = nullptr;

	if (!g_pJsonManager->ObjectForeachNext(handle, &key, nullptr, &pJSONValue)) {
		return false;
	}

	pContext->StringToLocalUTF8(params[2], params[3], key, nullptr);

	return CreateAndAssignHandle(pContext, pJSONValue, params[4], "JSON object value");
}

static cell_t json_arr_foreach(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	size_t index = 0;
	JsonValue* pJSONValue = nullptr;

	if (!g_pJsonManager->ArrayForeachNext(handle, &index, &pJSONValue)) {
		return false;
	}

	cell_t* indexPtr;
	pContext->LocalToPhysAddr(params[2], &indexPtr);
	*indexPtr = static_cast<cell_t>(index);

	return CreateAndAssignHandle(pContext, pJSONValue, params[3], "JSON array value");
}

static cell_t json_obj_foreach_key(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	const char* key = nullptr;

	if (!g_pJsonManager->ObjectForeachKeyNext(handle, &key, nullptr)) {
		return false;
	}

	pContext->StringToLocalUTF8(params[2], params[3], key, nullptr);

	return true;
}

static cell_t json_arr_foreach_index(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	size_t index = 0;

	if (!g_pJsonManager->ArrayForeachIndexNext(handle, &index)) {
		return false;
	}

	cell_t* indexPtr;
	pContext->LocalToPhysAddr(params[2], &indexPtr);
	*indexPtr = static_cast<cell_t>(index);

	return true;
}

static cell_t json_arr_sort(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot sort an immutable JSON array");
	}

	JSON_SORT_ORDER sort_mode = static_cast<JSON_SORT_ORDER>(params[2]);
	if (sort_mode < JSON_SORT_ASC || sort_mode > JSON_SORT_RANDOM) {
		return pContext->ThrowNativeError("Invalid sort mode: %d (expected 0=ascending, 1=descending, 2=random)", sort_mode);
	}

	return g_pJsonManager->ArraySort(handle, sort_mode);
}

static cell_t json_obj_sort(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Cannot sort an immutable JSON object");
	}

	JSON_SORT_ORDER sort_mode = static_cast<JSON_SORT_ORDER>(params[2]);
	if (sort_mode < JSON_SORT_ASC || sort_mode > JSON_SORT_RANDOM) {
		return pContext->ThrowNativeError("Invalid sort mode: %d (expected 0=ascending, 1=descending, 2=random)", sort_mode);
	}

	return g_pJsonManager->ObjectSort(handle, sort_mode);
}

static cell_t json_doc_to_mutable(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (handle->IsMutable()) {
		return pContext->ThrowNativeError("Document is already mutable");
	}

	JsonValue* pJSONValue = g_pJsonManager->ToMutable(handle);

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Failed to convert to mutable document");
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "mutable JSON document");
}

static cell_t json_doc_to_immutable(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);

	if (!handle) return 0;

	if (!handle->IsMutable()) {
		return pContext->ThrowNativeError("Document is already immutable");
	}

	JsonValue* pJSONValue = g_pJsonManager->ToImmutable(handle);

	if (!pJSONValue) {
		return pContext->ThrowNativeError("Failed to convert to immutable document");
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "immutable JSON document");
}

static cell_t json_arr_iter_init(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	JsonArrIter* iter = g_pJsonManager->ArrIterWith(handle);
	return CreateAndReturnArrIterHandle(pContext, iter, "array iterator");
}

static cell_t json_arr_iter_next(IPluginContext* pContext, const cell_t* params)
{
	JsonArrIter* iter = g_pJsonManager->GetArrIterFromHandle(pContext, params[1]);
	if (!iter) return 0;

	JsonValue* val = g_pJsonManager->ArrIterNext(iter);
	if (!val) return 0;

	return CreateAndReturnHandle(pContext, val, "array iterator value");
}

static cell_t json_arr_iter_has_next(IPluginContext* pContext, const cell_t* params)
{
	JsonArrIter* iter = g_pJsonManager->GetArrIterFromHandle(pContext, params[1]);
	if (!iter) return 0;

	return g_pJsonManager->ArrIterHasNext(iter);
}

static cell_t json_arr_iter_get_index(IPluginContext* pContext, const cell_t* params)
{
	JsonArrIter* iter = g_pJsonManager->GetArrIterFromHandle(pContext, params[1]);
	if (!iter) return 0;

	size_t index = g_pJsonManager->ArrIterGetIndex(iter);
	if (index == SIZE_MAX) {
		return -1;
	}

	return static_cast<cell_t>(index);
}

static cell_t json_arr_iter_remove(IPluginContext* pContext, const cell_t* params)
{
	JsonArrIter* iter = g_pJsonManager->GetArrIterFromHandle(pContext, params[1]);
	if (!iter) return 0;

	if (!iter->IsMutable()) {
		return pContext->ThrowNativeError("Cannot remove from immutable array iterator");
	}

	void* removed = g_pJsonManager->ArrIterRemove(iter);
	return removed != nullptr;
}

static cell_t json_obj_iter_init(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	JsonObjIter* iter = g_pJsonManager->ObjIterWith(handle);
	return CreateAndReturnObjIterHandle(pContext, iter, "object iterator");
}

static cell_t json_obj_iter_next(IPluginContext* pContext, const cell_t* params)
{
	JsonObjIter* iter = g_pJsonManager->GetObjIterFromHandle(pContext, params[1]);
	if (!iter) return 0;

	void* key = g_pJsonManager->ObjIterNext(iter);
	if (!key) return 0;

	const char* key_str;
	if (iter->IsMutable()) {
		key_str = yyjson_mut_get_str(reinterpret_cast<yyjson_mut_val*>(key));
	} else {
		key_str = yyjson_get_str(reinterpret_cast<yyjson_val*>(key));
	}

	pContext->StringToLocalUTF8(params[2], params[3], key_str, nullptr);
	return 1;
}

static cell_t json_obj_iter_has_next(IPluginContext* pContext, const cell_t* params)
{
	JsonObjIter* iter = g_pJsonManager->GetObjIterFromHandle(pContext, params[1]);
	if (!iter) return 0;

	return g_pJsonManager->ObjIterHasNext(iter);
}

static cell_t json_obj_iter_get_val(IPluginContext* pContext, const cell_t* params)
{
	JsonObjIter* iter = g_pJsonManager->GetObjIterFromHandle(pContext, params[1]);
	if (!iter) return 0;

	void* key = nullptr;
	if (iter->IsMutable()) {
		key = iter->m_currentKey;
	} else {
		key = iter->m_currentKey;
	}

	if (!key) {
		return pContext->ThrowNativeError("Iterator not positioned at a valid key (call Next() first)");
	}

	JsonValue* val = g_pJsonManager->ObjIterGetVal(iter, key);
	if (!val) {
		return pContext->ThrowNativeError("Failed to get value from iterator");
	}

	return CreateAndReturnHandle(pContext, val, "object iterator value");
}

static cell_t json_obj_iter_get(IPluginContext* pContext, const cell_t* params)
{
	JsonObjIter* iter = g_pJsonManager->GetObjIterFromHandle(pContext, params[1]);
	if (!iter) return 0;

	char* key;
	pContext->LocalToString(params[2], &key);

	JsonValue* val = g_pJsonManager->ObjIterGet(iter, key);
	if (!val) {
		return 0;
	}

	return CreateAndReturnHandle(pContext, val, "object iterator value");
}

static cell_t json_obj_iter_get_index(IPluginContext* pContext, const cell_t* params)
{
	JsonObjIter* iter = g_pJsonManager->GetObjIterFromHandle(pContext, params[1]);
	if (!iter) return 0;

	size_t index = g_pJsonManager->ObjIterGetIndex(iter);
	if (index == SIZE_MAX) {
		return -1;
	}

	return static_cast<cell_t>(index);
}

static cell_t json_obj_iter_remove(IPluginContext* pContext, const cell_t* params)
{
	JsonObjIter* iter = g_pJsonManager->GetObjIterFromHandle(pContext, params[1]);
	if (!iter) return 0;

	if (!iter->IsMutable()) {
		return pContext->ThrowNativeError("Cannot remove from immutable object iterator");
	}

	void* removed = g_pJsonManager->ObjIterRemove(iter);
	return removed != nullptr;
}

static cell_t json_read_number(IPluginContext* pContext, const cell_t* params)
{
	char* dat;
	pContext->LocalToString(params[1], &dat);
	yyjson_read_flag read_flg = static_cast<yyjson_read_flag>(params[2]);

	char error[JSON_ERROR_BUFFER_SIZE];
	size_t consumed = 0;
	JsonValue* pJSONValue = g_pJsonManager->ReadNumber(dat, read_flg, error, sizeof(error), &consumed);

	if (!pJSONValue) {
		return pContext->ThrowNativeError("%s", error);
	}

	cell_t* consumedPtr = nullptr;
	if (params[4] != 0) {
		pContext->LocalToPhysAddr(params[4], &consumedPtr);
		if (consumedPtr) {
			*consumedPtr = static_cast<cell_t>(consumed);
		}
	}

	return CreateAndReturnHandle(pContext, pJSONValue, "read number");
}

static cell_t json_write_number(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	size_t buffer_size = static_cast<size_t>(params[3]);
	char* temp_buffer = (char*)malloc(buffer_size);
	if (!temp_buffer) {
		return pContext->ThrowNativeError("Failed to allocate buffer");
	}

	size_t written = 0;
	if (!g_pJsonManager->WriteNumber(handle, temp_buffer, buffer_size, &written)) {
		free(temp_buffer);
		return pContext->ThrowNativeError("Failed to write number or buffer too small");
	}

	pContext->StringToLocalUTF8(params[2], buffer_size, temp_buffer, nullptr);
	free(temp_buffer);

	cell_t* writtenPtr = nullptr;
	if (params[4] != 0) {
		pContext->LocalToPhysAddr(params[4], &writtenPtr);
		if (writtenPtr) {
			*writtenPtr = static_cast<cell_t>(written);
		}
	}

	return 1;
}

static cell_t json_set_fp_to_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	bool flt = params[2] != 0;
	if (!g_pJsonManager->SetFpToFloat(handle, flt)) {
		return pContext->ThrowNativeError("Failed to set floating-point format to float (value is not a floating-point number)");
	}

	return 1;
}

static cell_t json_set_fp_to_fixed(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	int prec = params[2];
	if (!g_pJsonManager->SetFpToFixed(handle, prec)) {
		return pContext->ThrowNativeError("Failed to set floating-point format to fixed (value is not a floating-point number or precision out of range 1-15)");
	}

	return 1;
}

static cell_t json_set_bool(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	bool value = params[2] != 0;
	if (!g_pJsonManager->SetBool(handle, value)) {
		return pContext->ThrowNativeError("Failed to set value to boolean (value is object or array)");
	}

	return 1;
}

static cell_t json_set_int(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	int value = params[2];
	if (!g_pJsonManager->SetInt(handle, value)) {
		return pContext->ThrowNativeError("Failed to set value to integer (value is object or array)");
	}

	return 1;
}

static cell_t json_set_int64(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	char* str;
	pContext->LocalToString(params[2], &str);

	std::variant<int64_t, uint64_t> variant_value;
	char error[JSON_ERROR_BUFFER_SIZE];
	if (!g_pJsonManager->ParseInt64Variant(str, &variant_value, error, sizeof(error))) {
		return pContext->ThrowNativeError("%s", error);
	}

	if (!g_pJsonManager->SetInt64(handle, variant_value)) {
		return pContext->ThrowNativeError("Failed to set value to int64 (value is object or array)");
	}

	return 1;
}

static cell_t json_set_float(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	float value = sp_ctof(params[2]);
	if (!g_pJsonManager->SetFloat(handle, value)) {
		return pContext->ThrowNativeError("Failed to set value to float (value is object or array)");
	}

	return 1;
}

static cell_t json_set_string(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	char* value;
	pContext->LocalToString(params[2], &value);

	if (!g_pJsonManager->SetString(handle, value)) {
		return pContext->ThrowNativeError("Failed to set value to string (value is object or array)");
	}

	return 1;
}

static cell_t json_set_null(IPluginContext* pContext, const cell_t* params)
{
	JsonValue* handle = g_pJsonManager->GetFromHandle(pContext, params[1]);
	if (!handle) return 0;

	if (!g_pJsonManager->SetNull(handle)) {
		return pContext->ThrowNativeError("Failed to set value to null (value is object or array)");
	}

	return 1;
}

const sp_nativeinfo_t g_JsonNatives[] =
{
	// JSONObject
	{"JSONObject.JSONObject", json_obj_init},
	{"JSONObject.FromStrings", json_obj_init_with_str},
	{"JSONObject.Size.get", json_obj_get_size},
	{"JSONObject.Get", json_obj_get_val},
	{"JSONObject.GetBool", json_obj_get_bool},
	{"JSONObject.GetFloat", json_obj_get_float},
	{"JSONObject.GetInt", json_obj_get_int},
	{"JSONObject.GetInt64", json_obj_get_integer64},
	{"JSONObject.GetString", json_obj_get_str},
	{"JSONObject.IsNull", json_obj_is_null},
	{"JSONObject.GetKey", json_obj_get_key},
	{"JSONObject.GetValueAt", json_obj_get_val_at},
	{"JSONObject.HasKey", json_obj_has_key},
	{"JSONObject.RenameKey", json_obj_rename_key},
	{"JSONObject.Set", json_obj_set_val},
	{"JSONObject.SetBool", json_obj_set_bool},
	{"JSONObject.SetFloat", json_obj_set_float},
	{"JSONObject.SetInt", json_obj_set_int},
	{"JSONObject.SetInt64", json_obj_set_integer64},
	{"JSONObject.SetNull", json_obj_set_null},
	{"JSONObject.SetString", json_obj_set_str},
	{"JSONObject.Remove", json_obj_remove},
	{"JSONObject.Clear", json_obj_clear},
	{"JSONObject.FromString", json_obj_parse_str},
	{"JSONObject.FromFile", json_obj_parse_file},
	{"JSONObject.Sort", json_obj_sort},

	// JSONArray
	{"JSONArray.JSONArray", json_arr_init},
	{"JSONArray.FromStrings", json_arr_init_with_str},
	{"JSONArray.FromInt", json_arr_init_with_int32},
	{"JSONArray.FromInt64", json_arr_init_with_int64},
	{"JSONArray.FromBool", json_arr_init_with_bool},
	{"JSONArray.FromFloat", json_arr_init_with_float},
	{"JSONArray.Length.get", json_arr_get_size},
	{"JSONArray.Get", json_arr_get_val},
	{"JSONArray.First.get", json_arr_get_first},
	{"JSONArray.Last.get", json_arr_get_last},
	{"JSONArray.GetBool", json_arr_get_bool},
	{"JSONArray.GetFloat", json_arr_get_float},
	{"JSONArray.GetInt", json_arr_get_integer},
	{"JSONArray.GetInt64", json_arr_get_integer64},
	{"JSONArray.GetString", json_arr_get_str},
	{"JSONArray.IsNull", json_arr_is_null},
	{"JSONArray.Set", json_arr_replace_val},
	{"JSONArray.SetBool", json_arr_replace_bool},
	{"JSONArray.SetFloat", json_arr_replace_float},
	{"JSONArray.SetInt", json_arr_replace_integer},
	{"JSONArray.SetInt64", json_arr_replace_integer64},
	{"JSONArray.SetNull", json_arr_replace_null},
	{"JSONArray.SetString", json_arr_replace_str},
	{"JSONArray.Push", json_arr_append_val},
	{"JSONArray.PushBool", json_arr_append_bool},
	{"JSONArray.PushFloat", json_arr_append_float},
	{"JSONArray.PushInt", json_arr_append_int},
	{"JSONArray.PushInt64", json_arr_append_integer64},
	{"JSONArray.PushNull", json_arr_append_null},
	{"JSONArray.PushString", json_arr_append_str},
	{"JSONArray.Insert", json_arr_insert},
	{"JSONArray.InsertBool", json_arr_insert_bool},
	{"JSONArray.InsertInt", json_arr_insert_int},
	{"JSONArray.InsertInt64", json_arr_insert_int64},
	{"JSONArray.InsertFloat", json_arr_insert_float},
	{"JSONArray.InsertString", json_arr_insert_str},
	{"JSONArray.InsertNull", json_arr_insert_null},
	{"JSONArray.Prepend", json_arr_prepend},
	{"JSONArray.PrependBool", json_arr_prepend_bool},
	{"JSONArray.PrependInt", json_arr_prepend_int},
	{"JSONArray.PrependInt64", json_arr_prepend_int64},
	{"JSONArray.PrependFloat", json_arr_prepend_float},
	{"JSONArray.PrependString", json_arr_prepend_str},
	{"JSONArray.PrependNull", json_arr_prepend_null},
	{"JSONArray.Remove", json_arr_remove},
	{"JSONArray.RemoveFirst", json_arr_remove_first},
	{"JSONArray.RemoveLast", json_arr_remove_last},
	{"JSONArray.RemoveRange", json_arr_remove_range},
	{"JSONArray.Clear", json_arr_clear},
	{"JSONArray.FromString", json_arr_parse_str},
	{"JSONArray.FromFile", json_arr_parse_file},
	{"JSONArray.IndexOfBool", json_arr_index_of_bool},
	{"JSONArray.IndexOfString", json_arr_index_of_str},
	{"JSONArray.IndexOfInt", json_arr_index_of_int},
	{"JSONArray.IndexOfInt64", json_arr_index_of_integer64},
	{"JSONArray.IndexOfUint64", json_arr_index_of_uint64},
	{"JSONArray.IndexOfFloat", json_arr_index_of_float},
	{"JSONArray.Sort", json_arr_sort},

	// JSON UTILITY
	{"JSON.ToString", json_doc_write_to_str},
	{"JSON.ToFile", json_doc_write_to_file},
	{"JSON.Parse", json_doc_parse},
	{"JSON.Equals", json_doc_equals},
	{"JSON.EqualsStr", json_equals_str},
	{"JSON.DeepCopy", json_doc_copy_deep},
	{"JSON.GetTypeDesc", json_get_type_desc},
	{"JSON.GetSerializedSize", json_get_serialized_size},
	{"JSON.ReadSize.get", json_get_read_size},
	{"JSON.Type.get", json_get_type},
	{"JSON.SubType.get", json_get_subtype},
	{"JSON.IsArray.get", json_is_array},
	{"JSON.IsObject.get", json_is_object},
	{"JSON.IsInt.get", json_is_int},
	{"JSON.IsUint.get", json_is_uint},
	{"JSON.IsSint.get", json_is_sint},
	{"JSON.IsNum.get", json_is_num},
	{"JSON.IsBool.get", json_is_bool},
	{"JSON.IsTrue.get", json_is_true},
	{"JSON.IsFalse.get", json_is_false},
	{"JSON.IsFloat.get", json_is_float},
	{"JSON.IsStr.get", json_is_str},
	{"JSON.IsNull.get", json_is_null},
	{"JSON.IsCtn.get", json_is_ctn},
	{"JSON.IsMutable.get", json_is_mutable},
	{"JSON.IsImmutable.get", json_is_immutable},
	{"JSON.ForeachObject", json_obj_foreach},
	{"JSON.ForeachArray", json_arr_foreach},
	{"JSON.ForeachKey", json_obj_foreach_key},
	{"JSON.ForeachIndex", json_arr_foreach_index},
	{"JSON.ToMutable", json_doc_to_mutable},
	{"JSON.ToImmutable", json_doc_to_immutable},
	{"JSON.ReadNumber", json_read_number},
	{"JSON.WriteNumber", json_write_number},
	{"JSON.SetFpToFloat", json_set_fp_to_float},
	{"JSON.SetFpToFixed", json_set_fp_to_fixed},

	// JSON CREATE/GET/SET
	{"JSON.Pack", json_pack},
	{"JSON.CreateBool", json_create_bool},
	{"JSON.CreateFloat", json_create_float},
	{"JSON.CreateInt", json_create_int},
	{"JSON.CreateInt64", json_create_integer64},
	{"JSON.CreateNull", json_create_null},
	{"JSON.CreateString", json_create_str},
	{"JSON.GetBool", json_get_bool},
	{"JSON.GetFloat", json_get_float},
	{"JSON.GetInt", json_get_int},
	{"JSON.GetInt64", json_get_integer64},
	{"JSON.GetString", json_get_str},
	{"JSON.SetBool", json_set_bool},
	{"JSON.SetInt", json_set_int},
	{"JSON.SetInt64", json_set_int64},
	{"JSON.SetFloat", json_set_float},
	{"JSON.SetString", json_set_string},
	{"JSON.SetNull", json_set_null},

	// JSON POINTER
	{"JSON.PtrGet", json_ptr_get_val},
	{"JSON.PtrGetBool", json_ptr_get_bool},
	{"JSON.PtrGetFloat", json_ptr_get_float},
	{"JSON.PtrGetInt", json_ptr_get_int},
	{"JSON.PtrGetInt64", json_ptr_get_integer64},
	{"JSON.PtrGetString", json_ptr_get_str},
	{"JSON.PtrGetIsNull", json_ptr_get_is_null},
	{"JSON.PtrGetLength", json_ptr_get_length},
	{"JSON.PtrSet", json_ptr_set_val},
	{"JSON.PtrSetBool", json_ptr_set_bool},
	{"JSON.PtrSetFloat", json_ptr_set_float},
	{"JSON.PtrSetInt", json_ptr_set_int},
	{"JSON.PtrSetInt64", json_ptr_set_integer64},
	{"JSON.PtrSetString", json_ptr_set_str},
	{"JSON.PtrSetNull", json_ptr_set_null},
	{"JSON.PtrAdd", json_ptr_add_val},
	{"JSON.PtrAddBool", json_ptr_add_bool},
	{"JSON.PtrAddFloat", json_ptr_add_float},
	{"JSON.PtrAddInt", json_ptr_add_int},
	{"JSON.PtrAddInt64", json_ptr_add_integer64},
	{"JSON.PtrAddString", json_ptr_add_str},
	{"JSON.PtrAddNull", json_ptr_add_null},
	{"JSON.PtrRemove", json_ptr_remove_val},
	{"JSON.PtrTryGetVal", json_ptr_try_get_val},
	{"JSON.PtrTryGetBool", json_ptr_try_get_bool},
	{"JSON.PtrTryGetFloat", json_ptr_try_get_float},
	{"JSON.PtrTryGetInt", json_ptr_try_get_int},
	{"JSON.PtrTryGetInt64", json_ptr_try_get_integer64},
	{"JSON.PtrTryGetString", json_ptr_try_get_str},

	// JSONArrIter
	{"JSONArrIter.JSONArrIter", json_arr_iter_init},
	{"JSONArrIter.Next.get", json_arr_iter_next},
	{"JSONArrIter.HasNext.get", json_arr_iter_has_next},
	{"JSONArrIter.Index.get", json_arr_iter_get_index},
	{"JSONArrIter.Remove", json_arr_iter_remove},

	// JSONObjIter
	{"JSONObjIter.JSONObjIter", json_obj_iter_init},
	{"JSONObjIter.Next", json_obj_iter_next},
	{"JSONObjIter.HasNext.get", json_obj_iter_has_next},
	{"JSONObjIter.Value.get", json_obj_iter_get_val},
	{"JSONObjIter.Get", json_obj_iter_get},
	{"JSONObjIter.Index.get", json_obj_iter_get_index},
	{"JSONObjIter.Remove", json_obj_iter_remove},

	{nullptr, nullptr}
};