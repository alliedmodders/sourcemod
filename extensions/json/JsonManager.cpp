#include "JsonManager.h"
#include "extension.h"

static inline void ReadInt64FromVal(yyjson_val* val, std::variant<int64_t, uint64_t>* out_value) {
	if (yyjson_is_uint(val)) {
		*out_value = yyjson_get_uint(val);
	} else {
		*out_value = yyjson_get_sint(val);
	}
}

static inline void ReadInt64FromMutVal(yyjson_mut_val* val, std::variant<int64_t, uint64_t>* out_value) {
	if (yyjson_mut_is_uint(val)) {
		*out_value = yyjson_mut_get_uint(val);
	} else {
		*out_value = yyjson_mut_get_sint(val);
	}
}

// Set error message safely
static inline void SetErrorSafe(char* error, size_t error_size, const char* format, ...) {
	if (!error || error_size == 0) return;

	va_list args;
	va_start(args, format);
	int needed = vsnprintf(error, error_size, format, args);
	va_end(args);

	if (needed >= static_cast<int>(error_size)) {
		error[error_size - 1] = '\0';
	}
}

std::unique_ptr<JsonValue> JsonManager::CreateWrapper() {
	return std::make_unique<JsonValue>();
}

std::shared_ptr<yyjson_mut_doc> JsonManager::WrapDocument(yyjson_mut_doc* doc) {
	return std::shared_ptr<yyjson_mut_doc>(doc, [](yyjson_mut_doc*){});
}

std::shared_ptr<yyjson_mut_doc> JsonManager::CopyDocument(yyjson_doc* doc) {
	return WrapDocument(yyjson_doc_mut_copy(doc, nullptr));
}

std::shared_ptr<yyjson_mut_doc> JsonManager::CreateDocument() {
	return WrapDocument(yyjson_mut_doc_new(nullptr));
}

std::shared_ptr<yyjson_doc> JsonManager::WrapImmutableDocument(yyjson_doc* doc) {
	return std::shared_ptr<yyjson_doc>(doc, [](yyjson_doc*){});
}

JsonManager::JsonManager(): m_randomGenerator(m_randomDevice()) {}

JsonManager::~JsonManager() {}

JsonValue* JsonManager::ParseJSON(const char* json_str, bool is_file, bool is_mutable,
	yyjson_read_flag read_flg, char* error, size_t error_size)
{
	if (!json_str) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid JSON string");
		}
		return nullptr;
	}

	yyjson_read_err readError;
	yyjson_doc* idoc;
	auto pJSONValue = CreateWrapper();

	if (is_file) {
		char realpath[PLATFORM_MAX_PATH];
		smutils->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", json_str);
		idoc = yyjson_read_file(realpath, read_flg, nullptr, &readError);
	} else {
		idoc = yyjson_read_opts(const_cast<char*>(json_str), strlen(json_str), read_flg, nullptr, &readError);
	}

	if (!idoc || readError.code) {
		if (error && error_size > 0) {
			if (is_file) {
				SetErrorSafe(error, error_size, "Failed to parse JSON file: %s (error code: %u, msg: %s, position: %zu)",
					json_str, readError.code, readError.msg, readError.pos);
			} else {
				SetErrorSafe(error, error_size, "Failed to parse JSON str: %s (error code: %u, position: %zu)",
					readError.msg, readError.code, readError.pos);
			}
		}
		if (idoc) {
			yyjson_doc_free(idoc);
		}
		return nullptr;
	}

	pJSONValue->m_readSize = yyjson_doc_get_read_size(idoc);

	if (is_mutable) {
		pJSONValue->m_pDocument_mut = CopyDocument(idoc);
		pJSONValue->m_pVal_mut = yyjson_mut_doc_get_root(pJSONValue->m_pDocument_mut.get());
		yyjson_doc_free(idoc);
	} else {
		pJSONValue->m_pDocument = WrapImmutableDocument(idoc);
		pJSONValue->m_pVal = yyjson_doc_get_root(idoc);
	}

	return pJSONValue.release();
}

bool JsonManager::WriteToString(JsonValue* handle, char* buffer, size_t buffer_size,
	yyjson_write_flag write_flg, size_t* out_size)
{
	if (!handle || !buffer || buffer_size == 0) {
		return false;
	}

	size_t json_size;
	char* json_str;

	if (handle->IsMutable()) {
		json_str = yyjson_mut_val_write(handle->m_pVal_mut, write_flg, &json_size);
	} else {
		json_str = yyjson_val_write(handle->m_pVal, write_flg, &json_size);
	}

	if (!json_str) {
		return false;
	}

	size_t needed_size = json_size + 1;
	if (needed_size > buffer_size) {
		free(json_str);
		return false;
	}

	memcpy(buffer, json_str, json_size);
	buffer[json_size] = '\0';
	free(json_str);

	if (out_size) {
		*out_size = needed_size;
	}

	return true;
}

bool JsonManager::WriteToFile(JsonValue* handle, const char* path, yyjson_write_flag write_flg,
	char* error, size_t error_size)
{
	if (!handle || !path) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters");
		}
		return false;
	}

	char realpath[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);

	yyjson_write_err writeError;
	bool is_success;

	if (handle->IsMutable()) {
		is_success = yyjson_mut_write_file(realpath, handle->m_pDocument_mut.get(), write_flg, nullptr, &writeError);
	} else {
		is_success = yyjson_write_file(realpath, handle->m_pDocument.get(), write_flg, nullptr, &writeError);
	}

	if (writeError.code && error && error_size > 0) {
		SetErrorSafe(error, error_size, "Failed to write JSON to file: %s (error code: %u)", writeError.msg, writeError.code);
	}

	return is_success;
}

bool JsonManager::Equals(JsonValue* handle1, JsonValue* handle2)
{
	if (!handle1 || !handle2) {
		return false;
	}

	if (handle1->IsMutable() && handle2->IsMutable()) {
		return yyjson_mut_equals(handle1->m_pVal_mut, handle2->m_pVal_mut);
	}

	if (!handle1->IsMutable() && !handle2->IsMutable()) {
		auto doc1_mut = CopyDocument(handle1->m_pDocument.get());
		auto doc2_mut = CopyDocument(handle2->m_pDocument.get());

		if (!doc1_mut || !doc2_mut) {
			return false;
		}

		yyjson_mut_val* val1_mut = yyjson_mut_doc_get_root(doc1_mut.get());
		yyjson_mut_val* val2_mut = yyjson_mut_doc_get_root(doc2_mut.get());

		if (!val1_mut || !val2_mut) {
			return false;
		}

		return yyjson_mut_equals(val1_mut, val2_mut);
	}

	JsonValue* immutable = handle1->IsMutable() ? handle2 : handle1;
	JsonValue* mutable_doc = handle1->IsMutable() ? handle1 : handle2;

	auto doc_mut = CopyDocument(immutable->m_pDocument.get());
	if (!doc_mut) {
		return false;
	}

	yyjson_mut_val* val_mut = yyjson_mut_doc_get_root(doc_mut.get());
	if (!val_mut) {
		return false;
	}

	return yyjson_mut_equals(mutable_doc->m_pVal_mut, val_mut);
}

bool JsonManager::EqualsStr(JsonValue* handle, const char* str)
{
	if (!handle || !str) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_equals_str(handle->m_pVal_mut, str);
	} else {
		return yyjson_equals_str(handle->m_pVal, str);
	}
}

JsonValue* JsonManager::DeepCopy(JsonValue* targetDoc, JsonValue* sourceValue)
{
	if (!targetDoc || !sourceValue) {
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();

	if (targetDoc->IsMutable()) {
		pJSONValue->m_pDocument_mut = CreateDocument();

		yyjson_mut_val* val_copy = nullptr;
		if (sourceValue->IsMutable()) {
			val_copy = yyjson_mut_val_mut_copy(pJSONValue->m_pDocument_mut.get(), sourceValue->m_pVal_mut);
		} else {
			val_copy = yyjson_val_mut_copy(pJSONValue->m_pDocument_mut.get(), sourceValue->m_pVal);
		}

		if (!val_copy) {
			return nullptr;
		}

		yyjson_mut_doc_set_root(pJSONValue->m_pDocument_mut.get(), val_copy);
		pJSONValue->m_pVal_mut = val_copy;
	} else {
		yyjson_mut_doc* temp_doc = yyjson_mut_doc_new(nullptr);
		if (!temp_doc) {
			return nullptr;
		}

		yyjson_mut_val* temp_val = nullptr;
		if (sourceValue->IsMutable()) {
			temp_val = yyjson_mut_val_mut_copy(temp_doc, sourceValue->m_pVal_mut);
		} else {
			temp_val = yyjson_val_mut_copy(temp_doc, sourceValue->m_pVal);
		}

		if (!temp_val) {
			yyjson_mut_doc_free(temp_doc);
			return nullptr;
		}

		yyjson_mut_doc_set_root(temp_doc, temp_val);

		yyjson_doc* doc = yyjson_mut_doc_imut_copy(temp_doc, nullptr);
		yyjson_mut_doc_free(temp_doc);

		if (!doc) {
			return nullptr;
		}

		pJSONValue->m_pDocument = WrapImmutableDocument(doc);
		pJSONValue->m_pVal = yyjson_doc_get_root(doc);
	}

	return pJSONValue.release();
}

const char* JsonManager::GetTypeDesc(JsonValue* handle)
{
	if (!handle) {
		return "invalid";
	}

	if (handle->IsMutable()) {
		return yyjson_mut_get_type_desc(handle->m_pVal_mut);
	} else {
		return yyjson_get_type_desc(handle->m_pVal);
	}
}

size_t JsonManager::GetSerializedSize(JsonValue* handle, yyjson_write_flag write_flg)
{
	if (!handle) {
		return 0;
	}

	size_t json_size;
	char* json_str;

	if (handle->IsMutable()) {
		json_str = yyjson_mut_val_write(handle->m_pVal_mut, write_flg, &json_size);
	} else {
		json_str = yyjson_val_write(handle->m_pVal, write_flg, &json_size);
	}

	if (json_str) {
		free(json_str);
		return json_size + 1;
	}

	return 0;
}

JsonValue* JsonManager::ToMutable(JsonValue* handle)
{
	if (!handle || handle->IsMutable()) {
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CopyDocument(handle->m_pDocument.get());
	pJSONValue->m_pVal_mut = yyjson_mut_doc_get_root(pJSONValue->m_pDocument_mut.get());

	return pJSONValue.release();
}

JsonValue* JsonManager::ToImmutable(JsonValue* handle)
{
	if (!handle || !handle->IsMutable()) {
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();
	yyjson_doc* mdoc = yyjson_mut_doc_imut_copy(handle->m_pDocument_mut.get(), nullptr);
	pJSONValue->m_pDocument = WrapImmutableDocument(mdoc);
	pJSONValue->m_pVal = yyjson_doc_get_root(pJSONValue->m_pDocument.get());

	return pJSONValue.release();
}

yyjson_type JsonManager::GetType(JsonValue* handle)
{
	if (!handle) {
		return YYJSON_TYPE_NONE;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_get_type(handle->m_pVal_mut);
	} else {
		return yyjson_get_type(handle->m_pVal);
	}
}

yyjson_subtype JsonManager::GetSubtype(JsonValue* handle)
{
	if (!handle) {
		return YYJSON_SUBTYPE_NONE;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_get_subtype(handle->m_pVal_mut);
	} else {
		return yyjson_get_subtype(handle->m_pVal);
	}
}

bool JsonManager::IsArray(JsonValue* handle)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_is_arr(handle->m_pVal_mut);
	} else {
		return yyjson_is_arr(handle->m_pVal);
	}
}

bool JsonManager::IsObject(JsonValue* handle)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_is_obj(handle->m_pVal_mut);
	} else {
		return yyjson_is_obj(handle->m_pVal);
	}
}

bool JsonManager::IsInt(JsonValue* handle)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_is_int(handle->m_pVal_mut);
	} else {
		return yyjson_is_int(handle->m_pVal);
	}
}

bool JsonManager::IsUint(JsonValue* handle)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_is_uint(handle->m_pVal_mut);
	} else {
		return yyjson_is_uint(handle->m_pVal);
	}
}

bool JsonManager::IsSint(JsonValue* handle)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_is_sint(handle->m_pVal_mut);
	} else {
		return yyjson_is_sint(handle->m_pVal);
	}
}

bool JsonManager::IsNum(JsonValue* handle)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_is_num(handle->m_pVal_mut);
	} else {
		return yyjson_is_num(handle->m_pVal);
	}
}

bool JsonManager::IsBool(JsonValue* handle)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_is_bool(handle->m_pVal_mut);
	} else {
		return yyjson_is_bool(handle->m_pVal);
	}
}

bool JsonManager::IsTrue(JsonValue* handle)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_is_true(handle->m_pVal_mut);
	} else {
		return yyjson_is_true(handle->m_pVal);
	}
}

bool JsonManager::IsFalse(JsonValue* handle)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_is_false(handle->m_pVal_mut);
	} else {
		return yyjson_is_false(handle->m_pVal);
	}
}

bool JsonManager::IsFloat(JsonValue* handle)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_is_real(handle->m_pVal_mut);
	} else {
		return yyjson_is_real(handle->m_pVal);
	}
}

bool JsonManager::IsStr(JsonValue* handle)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_is_str(handle->m_pVal_mut);
	} else {
		return yyjson_is_str(handle->m_pVal);
	}
}

bool JsonManager::IsNull(JsonValue* handle)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_is_null(handle->m_pVal_mut);
	} else {
		return yyjson_is_null(handle->m_pVal);
	}
}

bool JsonManager::IsCtn(JsonValue* handle)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_is_ctn(handle->m_pVal_mut);
	} else {
		return yyjson_is_ctn(handle->m_pVal);
	}
}

bool JsonManager::IsMutable(JsonValue* handle)
{
	if (!handle) {
		return false;
	}

	return handle->IsMutable();
}

bool JsonManager::IsImmutable(JsonValue* handle)
{
	if (!handle) {
		return false;
	}

	return handle->IsImmutable();
}

size_t JsonManager::GetReadSize(JsonValue* handle)
{
	if (!handle) {
		return 0;
	}

	if (handle->m_readSize == 0) {
		return 0;
	}

	return handle->m_readSize + 1;
}

JsonValue* JsonManager::ObjectInit()
{
	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CreateDocument();
	pJSONValue->m_pVal_mut = yyjson_mut_obj(pJSONValue->m_pDocument_mut.get());
	yyjson_mut_doc_set_root(pJSONValue->m_pDocument_mut.get(), pJSONValue->m_pVal_mut);

	return pJSONValue.release();
}

JsonValue* JsonManager::ObjectInitWithStrings(const char** pairs, size_t count)
{
	if (!pairs || count == 0) {
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CreateDocument();

	pJSONValue->m_pVal_mut = yyjson_mut_obj_with_kv(
		pJSONValue->m_pDocument_mut.get(),
		pairs,
		count
	);

	if (!pJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pJSONValue.release();
}

JsonValue* JsonManager::ObjectParseString(const char* str, yyjson_read_flag read_flg,
	char* error, size_t error_size)
{
	if (!str) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid string");
		}
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();

	yyjson_read_err readError;
	yyjson_doc* idoc = yyjson_read_opts(const_cast<char*>(str), strlen(str), read_flg, nullptr, &readError);

	if (!idoc || readError.code) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Failed to parse JSON str: %s (error code: %u, position: %zu)",
				readError.msg, readError.code, readError.pos);
		}
		if (idoc) {
			yyjson_doc_free(idoc);
		}
		return nullptr;
	}

	yyjson_val* root = yyjson_doc_get_root(idoc);

	if (!yyjson_is_obj(root)) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Root value is not an object (got %s)", yyjson_get_type_desc(root));
		}
		yyjson_doc_free(idoc);
		return nullptr;
	}

	pJSONValue->m_readSize = yyjson_doc_get_read_size(idoc);
	pJSONValue->m_pDocument = WrapImmutableDocument(idoc);
	pJSONValue->m_pVal = root;

	return pJSONValue.release();
}

JsonValue* JsonManager::ObjectParseFile(const char* path, yyjson_read_flag read_flg,
	char* error, size_t error_size)
{
	if (!path) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid path");
		}
		return nullptr;
	}

	char realpath[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);
	auto pJSONValue = CreateWrapper();

	yyjson_read_err readError;
	yyjson_doc* idoc = yyjson_read_file(realpath, read_flg, nullptr, &readError);

	if (!idoc || readError.code) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Failed to parse JSON file: %s (error code: %u, msg: %s, position: %zu)",
				realpath, readError.code, readError.msg, readError.pos);
		}
		if (idoc) {
			yyjson_doc_free(idoc);
		}
		return nullptr;
	}

	yyjson_val* root = yyjson_doc_get_root(idoc);

	if (!yyjson_is_obj(root)) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Root value in file is not an object (got %s)", yyjson_get_type_desc(root));
		}
		yyjson_doc_free(idoc);
		return nullptr;
	}

	pJSONValue->m_readSize = yyjson_doc_get_read_size(idoc);
	pJSONValue->m_pDocument = WrapImmutableDocument(idoc);
	pJSONValue->m_pVal = root;

	return pJSONValue.release();
}

size_t JsonManager::ObjectGetSize(JsonValue* handle)
{
	if (!handle) {
		return 0;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_obj_size(handle->m_pVal_mut);
	} else {
		return yyjson_obj_size(handle->m_pVal);
	}
}

bool JsonManager::ObjectGetKey(JsonValue* handle, size_t index, const char** out_key)
{
	if (!handle || !out_key) {
		return false;
	}

	if (handle->IsMutable()) {
		size_t obj_size = yyjson_mut_obj_size(handle->m_pVal_mut);
		if (index >= obj_size) {
			return false;
		}

		yyjson_mut_obj_iter iter;
		yyjson_mut_obj_iter_init(handle->m_pVal_mut, &iter);

		for (size_t i = 0; i < index; i++) {
			yyjson_mut_obj_iter_next(&iter);
		}

		yyjson_mut_val* key = yyjson_mut_obj_iter_next(&iter);
		if (!key) {
			return false;
		}

		*out_key = yyjson_mut_get_str(key);
		return true;
	} else {
		size_t obj_size = yyjson_obj_size(handle->m_pVal);
		if (index >= obj_size) {
			return false;
		}

		yyjson_obj_iter iter;
		yyjson_obj_iter_init(handle->m_pVal, &iter);

		for (size_t i = 0; i < index; i++) {
			yyjson_obj_iter_next(&iter);
		}

		yyjson_val* key = yyjson_obj_iter_next(&iter);
		if (!key) {
			return false;
		}

		*out_key = yyjson_get_str(key);
		return true;
	}
}

JsonValue* JsonManager::ObjectGetValueAt(JsonValue* handle, size_t index)
{
	if (!handle) {
		return nullptr;
	}

	size_t obj_size = handle->IsMutable() ? yyjson_mut_obj_size(handle->m_pVal_mut) : yyjson_obj_size(handle->m_pVal);

	if (index >= obj_size) {
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();

	if (handle->IsMutable()) {
		yyjson_mut_obj_iter iter;
		yyjson_mut_obj_iter_init(handle->m_pVal_mut, &iter);

		for (size_t i = 0; i < index; i++) {
			yyjson_mut_obj_iter_next(&iter);
		}

		yyjson_mut_val* key = yyjson_mut_obj_iter_next(&iter);
		if (!key) {
			return nullptr;
		}

		pJSONValue->m_pDocument_mut = handle->m_pDocument_mut;
		pJSONValue->m_pVal_mut = yyjson_mut_obj_iter_get_val(key);
	} else {
		yyjson_obj_iter iter;
		yyjson_obj_iter_init(handle->m_pVal, &iter);

		yyjson_val* key = nullptr;
		for (size_t i = 0; i <= index; i++) {
			key = yyjson_obj_iter_next(&iter);
			if (!key) {
				return nullptr;
			}
		}

		pJSONValue->m_pDocument = handle->m_pDocument;
		pJSONValue->m_pVal = yyjson_obj_iter_get_val(key);
	}

	return pJSONValue.release();
}

JsonValue* JsonManager::ObjectGet(JsonValue* handle, const char* key)
{
	if (!handle || !key) {
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_obj_get(handle->m_pVal_mut, key);
		if (!val) {
			return nullptr;
		}

		pJSONValue->m_pDocument_mut = handle->m_pDocument_mut;
		pJSONValue->m_pVal_mut = val;
	} else {
		yyjson_val* val = yyjson_obj_get(handle->m_pVal, key);
		if (!val) {
			return nullptr;
		}

		pJSONValue->m_pDocument = handle->m_pDocument;
		pJSONValue->m_pVal = val;
	}

	return pJSONValue.release();
}

bool JsonManager::ObjectGetBool(JsonValue* handle, const char* key, bool* out_value)
{
	if (!handle || !key || !out_value) {
		return false;
	}

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_obj_get(handle->m_pVal_mut, key);
		if (!val || !yyjson_mut_is_bool(val)) {
			return false;
		}

		*out_value = yyjson_mut_get_bool(val);
		return true;
	} else {
		yyjson_val* val = yyjson_obj_get(handle->m_pVal, key);
		if (!val || !yyjson_is_bool(val)) {
			return false;
		}

		*out_value = yyjson_get_bool(val);
		return true;
	}
}

bool JsonManager::ObjectGetFloat(JsonValue* handle, const char* key, double* out_value)
{
	if (!handle || !key || !out_value) {
		return false;
	}

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_obj_get(handle->m_pVal_mut, key);
		if (!val || !yyjson_mut_is_real(val)) {
			return false;
		}

		*out_value = yyjson_mut_get_real(val);
		return true;
	} else {
		yyjson_val* val = yyjson_obj_get(handle->m_pVal, key);
		if (!val || !yyjson_is_real(val)) {
			return false;
		}

		*out_value = yyjson_get_real(val);
		return true;
	}
}

bool JsonManager::ObjectGetInt(JsonValue* handle, const char* key, int* out_value)
{
	if (!handle || !key || !out_value) {
		return false;
	}

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_obj_get(handle->m_pVal_mut, key);
		if (!val || !yyjson_mut_is_int(val)) {
			return false;
		}

		*out_value = yyjson_mut_get_int(val);
		return true;
	} else {
		yyjson_val* val = yyjson_obj_get(handle->m_pVal, key);
		if (!val || !yyjson_is_int(val)) {
			return false;
		}

		*out_value = yyjson_get_int(val);
		return true;
	}
}

bool JsonManager::ObjectGetInt64(JsonValue* handle, const char* key, std::variant<int64_t, uint64_t>* out_value)
{
	if (!handle || !key || !out_value) {
		return false;
	}

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_obj_get(handle->m_pVal_mut, key);
		if (!val || !yyjson_mut_is_int(val)) {
			return false;
		}

		ReadInt64FromMutVal(val, out_value);
		return true;
	} else {
		yyjson_val* val = yyjson_obj_get(handle->m_pVal, key);
		if (!val || !yyjson_is_int(val)) {
			return false;
		}

		ReadInt64FromVal(val, out_value);
		return true;
	}
}

bool JsonManager::ObjectGetString(JsonValue* handle, const char* key, const char** out_str, size_t* out_len)
{
	if (!handle || !key || !out_str) {
		return false;
	}

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_obj_get(handle->m_pVal_mut, key);
		if (!val || !yyjson_mut_is_str(val)) {
			return false;
		}

		*out_str = yyjson_mut_get_str(val);
		if (out_len) {
			*out_len = yyjson_mut_get_len(val);
		}
		return true;
	} else {
		yyjson_val* val = yyjson_obj_get(handle->m_pVal, key);
		if (!val || !yyjson_is_str(val)) {
			return false;
		}

		*out_str = yyjson_get_str(val);
		if (out_len) {
			*out_len = yyjson_get_len(val);
		}
		return true;
	}
}

bool JsonManager::ObjectIsNull(JsonValue* handle, const char* key, bool* out_is_null)
{
	if (!handle || !key || !out_is_null) {
		return false;
	}

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_obj_get(handle->m_pVal_mut, key);
		if (!val) {
			return false;
		}

		*out_is_null = yyjson_mut_is_null(val);
		return true;
	} else {
		yyjson_val* val = yyjson_obj_get(handle->m_pVal, key);
		if (!val) {
			return false;
		}

		*out_is_null = yyjson_is_null(val);
		return true;
	}
}

bool JsonManager::ObjectHasKey(JsonValue* handle, const char* key, bool use_pointer)
{
	if (!handle || !key) {
		return false;
	}

	if (handle->IsMutable()) {
		if (use_pointer) {
			return yyjson_mut_doc_ptr_get(handle->m_pDocument_mut.get(), key) != nullptr;
		} else {
			yyjson_mut_obj_iter iter = yyjson_mut_obj_iter_with(handle->m_pVal_mut);
			return yyjson_mut_obj_iter_get(&iter, key) != nullptr;
		}
	} else {
		if (use_pointer) {
			return yyjson_doc_ptr_get(handle->m_pDocument.get(), key) != nullptr;
		} else {
			yyjson_obj_iter iter = yyjson_obj_iter_with(handle->m_pVal);
			return yyjson_obj_iter_get(&iter, key) != nullptr;
		}
	}
}

bool JsonManager::ObjectRenameKey(JsonValue* handle, const char* old_key, const char* new_key, bool allow_duplicate)
{
	if (!handle || !handle->IsMutable() || !old_key || !new_key) {
		return false;
	}

	if (!yyjson_mut_obj_get(handle->m_pVal_mut, old_key)) {
		return false;
	}

	if (!allow_duplicate && yyjson_mut_obj_get(handle->m_pVal_mut, new_key)) {
		return false;
	}

	return yyjson_mut_obj_rename_key(handle->m_pDocument_mut.get(), handle->m_pVal_mut, old_key, new_key);
}

bool JsonManager::ObjectSet(JsonValue* handle, const char* key, JsonValue* value)
{
	if (!handle || !handle->IsMutable() || !key || !value) {
		return false;
	}

	yyjson_mut_val* val_copy = nullptr;
	if (value->IsMutable()) {
		val_copy = yyjson_mut_val_mut_copy(handle->m_pDocument_mut.get(), value->m_pVal_mut);
	} else {
		val_copy = yyjson_val_mut_copy(handle->m_pDocument_mut.get(), value->m_pVal);
	}

	if (!val_copy) {
		return false;
	}

	return yyjson_mut_obj_put(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), key), val_copy);
}

bool JsonManager::ObjectSetBool(JsonValue* handle, const char* key, bool value)
{
	if (!handle || !handle->IsMutable() || !key) {
		return false;
	}

	return yyjson_mut_obj_put(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), key), yyjson_mut_bool(handle->m_pDocument_mut.get(), value));
}

bool JsonManager::ObjectSetFloat(JsonValue* handle, const char* key, double value)
{
	if (!handle || !handle->IsMutable() || !key) {
		return false;
	}

	return yyjson_mut_obj_put(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), key), yyjson_mut_real(handle->m_pDocument_mut.get(), value));
}

bool JsonManager::ObjectSetInt(JsonValue* handle, const char* key, int value)
{
	if (!handle || !handle->IsMutable() || !key) {
		return false;
	}

	return yyjson_mut_obj_put(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), key), yyjson_mut_int(handle->m_pDocument_mut.get(), value));
}

bool JsonManager::ObjectSetInt64(JsonValue* handle, const char* key, std::variant<int64_t, uint64_t> value)
{
	if (!handle || !handle->IsMutable() || !key) {
		return false;
	}

	return std::visit([&](auto&& val) -> bool {
		using T = std::decay_t<decltype(val)>;
		if constexpr (std::is_same_v<T, int64_t>) {
			return yyjson_mut_obj_put(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), key), yyjson_mut_sint(handle->m_pDocument_mut.get(), val));
		} else if constexpr (std::is_same_v<T, uint64_t>) {
			return yyjson_mut_obj_put(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), key), yyjson_mut_uint(handle->m_pDocument_mut.get(), val));
		}
		return false;
	}, value);
}

bool JsonManager::ObjectSetNull(JsonValue* handle, const char* key)
{
	if (!handle || !handle->IsMutable() || !key) {
		return false;
	}

	return yyjson_mut_obj_put(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), key), yyjson_mut_null(handle->m_pDocument_mut.get()));
}

bool JsonManager::ObjectSetString(JsonValue* handle, const char* key, const char* value)
{
	if (!handle || !handle->IsMutable() || !key || !value) {
		return false;
	}

	return yyjson_mut_obj_put(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), key), yyjson_mut_strcpy(handle->m_pDocument_mut.get(), value));
}

bool JsonManager::ObjectRemove(JsonValue* handle, const char* key)
{
	if (!handle || !handle->IsMutable() || !key) {
		return false;
	}

	return yyjson_mut_obj_remove_key(handle->m_pVal_mut, key) != nullptr;
}

bool JsonManager::ObjectClear(JsonValue* handle)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_obj_clear(handle->m_pVal_mut);
}

bool JsonManager::ObjectSort(JsonValue* handle, JSON_SORT_ORDER sort_mode)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	if (!yyjson_mut_is_obj(handle->m_pVal_mut)) {
		return false;
	}

	if (sort_mode < JSON_SORT_ASC || sort_mode > JSON_SORT_RANDOM) {
		return false;
	}

	size_t obj_size = yyjson_mut_obj_size(handle->m_pVal_mut);
	if (obj_size <= 1) return true;

	struct KeyValuePair {
		yyjson_mut_val* key;
		const char* key_str;
		size_t key_len;
		yyjson_mut_val* val;
	};
	std::vector<KeyValuePair> pairs;
	pairs.reserve(obj_size);

	size_t idx, max;
	yyjson_mut_val *key, *val;
	yyjson_mut_obj_foreach(handle->m_pVal_mut, idx, max, key, val) {
		const char* key_str = yyjson_mut_get_str(key);
		size_t key_len = yyjson_mut_get_len(key);
		pairs.push_back({key, key_str, key_len, val});
	}

	if (sort_mode == JSON_SORT_RANDOM) {
		std::shuffle(pairs.begin(), pairs.end(), m_randomGenerator);
	}
	else {
		auto compare = [sort_mode](const KeyValuePair& a, const KeyValuePair& b) {
			size_t min_len = a.key_len < b.key_len ? a.key_len : b.key_len;
			int cmp = memcmp(a.key_str, b.key_str, min_len);
			if (cmp == 0) {
				cmp = (a.key_len < b.key_len) ? -1 : (a.key_len > b.key_len ? 1 : 0);
			}
			return sort_mode == JSON_SORT_ASC ? cmp < 0 : cmp > 0;
		};

		std::sort(pairs.begin(), pairs.end(), compare);
	}

	yyjson_mut_obj_clear(handle->m_pVal_mut);

	for (const auto& pair : pairs) {
		yyjson_mut_obj_add(handle->m_pVal_mut, pair.key, pair.val);
	}

	return true;
}

JsonValue* JsonManager::ArrayInit()
{
	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CreateDocument();
	pJSONValue->m_pVal_mut = yyjson_mut_arr(pJSONValue->m_pDocument_mut.get());
	yyjson_mut_doc_set_root(pJSONValue->m_pDocument_mut.get(), pJSONValue->m_pVal_mut);

	return pJSONValue.release();
}

JsonValue* JsonManager::ArrayInitWithStrings(const char** strings, size_t count)
{
	if (!strings) {
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CreateDocument();

	pJSONValue->m_pVal_mut = yyjson_mut_arr_with_strcpy(
		pJSONValue->m_pDocument_mut.get(),
		strings,
		count
	);

	if (!pJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pJSONValue.release();
}

JsonValue* JsonManager::ArrayInitWithInt32(const int32_t* values, size_t count)
{
	if (!values) {
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CreateDocument();

	pJSONValue->m_pVal_mut = yyjson_mut_arr_with_sint32(
		pJSONValue->m_pDocument_mut.get(),
		values,
		count
	);

	if (!pJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pJSONValue.release();
}

JsonValue* JsonManager::ArrayInitWithInt64(const char** values, size_t count, char* error, size_t error_size)
{
	if (!values) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid values parameter");
		}
		return nullptr;
	}

	if (count == 0) {
		auto pJSONValue = CreateWrapper();
		pJSONValue->m_pDocument_mut = CreateDocument();
		pJSONValue->m_pVal_mut = yyjson_mut_arr(pJSONValue->m_pDocument_mut.get());
		return pJSONValue.release();
	}

	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CreateDocument();
	auto doc = pJSONValue->m_pDocument_mut.get();

	pJSONValue->m_pVal_mut = yyjson_mut_arr(doc);
	if (!pJSONValue->m_pVal_mut) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Failed to create array");
		}
		return nullptr;
	}

	for (size_t i = 0; i < count; i++) {
		std::variant<int64_t, uint64_t> variant_value;
		if (!ParseInt64Variant(values[i], &variant_value, error, error_size)) {
			return nullptr;
		}

		yyjson_mut_val* val = nullptr;
		std::visit([&](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, int64_t>) {
				val = yyjson_mut_sint(doc, arg);
			} else if constexpr (std::is_same_v<T, uint64_t>) {
				val = yyjson_mut_uint(doc, arg);
			}
		}, variant_value);

		if (!val || !yyjson_mut_arr_append(pJSONValue->m_pVal_mut, val)) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to append value at index %zu", i);
			}
			return nullptr;
		}
	}

	return pJSONValue.release();
}

JsonValue* JsonManager::ArrayInitWithBool(const bool* values, size_t count)
{
	if (!values) {
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CreateDocument();

	pJSONValue->m_pVal_mut = yyjson_mut_arr_with_bool(
		pJSONValue->m_pDocument_mut.get(),
		values,
		count
	);

	if (!pJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pJSONValue.release();
}

JsonValue* JsonManager::ArrayInitWithFloat(const double* values, size_t count)
{
	if (!values) {
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CreateDocument();

	pJSONValue->m_pVal_mut = yyjson_mut_arr_with_real(
		pJSONValue->m_pDocument_mut.get(),
		values,
		count
	);

	if (!pJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pJSONValue.release();
}

JsonValue* JsonManager::ArrayParseString(const char* str, yyjson_read_flag read_flg,
	char* error, size_t error_size)
{
	if (!str) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid string");
		}
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();

	yyjson_read_err readError;
	yyjson_doc* idoc = yyjson_read_opts(const_cast<char*>(str), strlen(str), read_flg, nullptr, &readError);

	if (!idoc || readError.code) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Failed to parse JSON string: %s (error code: %u, position: %zu)",
				readError.msg, readError.code, readError.pos);
		}
		if (idoc) {
			yyjson_doc_free(idoc);
		}
		return nullptr;
	}

	yyjson_val* root = yyjson_doc_get_root(idoc);

	if (!yyjson_is_arr(root)) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Root value is not an array (got %s)", yyjson_get_type_desc(root));
		}
		yyjson_doc_free(idoc);
		return nullptr;
	}

	pJSONValue->m_readSize = yyjson_doc_get_read_size(idoc);
	pJSONValue->m_pDocument = WrapImmutableDocument(idoc);
	pJSONValue->m_pVal = root;

	return pJSONValue.release();
}

JsonValue* JsonManager::ArrayParseFile(const char* path, yyjson_read_flag read_flg,
	char* error, size_t error_size)
{
	if (!path) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid path");
		}
		return nullptr;
	}

	char realpath[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);
	auto pJSONValue = CreateWrapper();

	yyjson_read_err readError;
	yyjson_doc* idoc = yyjson_read_file(realpath, read_flg, nullptr, &readError);

	if (!idoc || readError.code) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Failed to parse JSON file: %s (error code: %u, msg: %s, position: %zu)",
				realpath, readError.code, readError.msg, readError.pos);
		}
		if (idoc) {
			yyjson_doc_free(idoc);
		}
		return nullptr;
	}

	yyjson_val* root = yyjson_doc_get_root(idoc);

	if (!yyjson_is_arr(root)) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Root value in file is not an array (got %s)", yyjson_get_type_desc(root));
		}
		yyjson_doc_free(idoc);
		return nullptr;
	}

	pJSONValue->m_readSize = yyjson_doc_get_read_size(idoc);
	pJSONValue->m_pDocument = WrapImmutableDocument(idoc);
	pJSONValue->m_pVal = root;

	return pJSONValue.release();
}

size_t JsonManager::ArrayGetSize(JsonValue* handle)
{
	if (!handle) {
		return 0;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_arr_size(handle->m_pVal_mut);
	} else {
		return yyjson_arr_size(handle->m_pVal);
	}
}

JsonValue* JsonManager::ArrayGet(JsonValue* handle, size_t index)
{
	if (!handle) {
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();

	if (handle->IsMutable()) {
		size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
		if (index >= arr_size) {
			return nullptr;
		}

		yyjson_mut_val* val = yyjson_mut_arr_get(handle->m_pVal_mut, index);
		if (!val) {
			return nullptr;
		}

		pJSONValue->m_pDocument_mut = handle->m_pDocument_mut;
		pJSONValue->m_pVal_mut = val;
	} else {
		size_t arr_size = yyjson_arr_size(handle->m_pVal);
		if (index >= arr_size) {
			return nullptr;
		}

		yyjson_val* val = yyjson_arr_get(handle->m_pVal, index);
		if (!val) {
			return nullptr;
		}

		pJSONValue->m_pDocument = handle->m_pDocument;
		pJSONValue->m_pVal = val;
	}

	return pJSONValue.release();
}

JsonValue* JsonManager::ArrayGetFirst(JsonValue* handle)
{
	if (!handle) {
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();

	if (handle->IsMutable()) {
		size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
		if (arr_size == 0) {
			return nullptr;
		}

		yyjson_mut_val* val = yyjson_mut_arr_get_first(handle->m_pVal_mut);
		if (!val) {
			return nullptr;
		}

		pJSONValue->m_pDocument_mut = handle->m_pDocument_mut;
		pJSONValue->m_pVal_mut = val;
	} else {
		size_t arr_size = yyjson_arr_size(handle->m_pVal);
		if (arr_size == 0) {
			return nullptr;
		}

		yyjson_val* val = yyjson_arr_get_first(handle->m_pVal);
		if (!val) {
			return nullptr;
		}

		pJSONValue->m_pDocument = handle->m_pDocument;
		pJSONValue->m_pVal = val;
	}

	return pJSONValue.release();
}

JsonValue* JsonManager::ArrayGetLast(JsonValue* handle)
{
	if (!handle) {
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();

	if (handle->IsMutable()) {
		size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
		if (arr_size == 0) {
			return nullptr;
		}

		yyjson_mut_val* val = yyjson_mut_arr_get_last(handle->m_pVal_mut);
		if (!val) {
			return nullptr;
		}

		pJSONValue->m_pDocument_mut = handle->m_pDocument_mut;
		pJSONValue->m_pVal_mut = val;
	} else {
		size_t arr_size = yyjson_arr_size(handle->m_pVal);
		if (arr_size == 0) {
			return nullptr;
		}

		yyjson_val* val = yyjson_arr_get_last(handle->m_pVal);
		if (!val) {
			return nullptr;
		}

		pJSONValue->m_pDocument = handle->m_pDocument;
		pJSONValue->m_pVal = val;
	}

	return pJSONValue.release();
}

bool JsonManager::ArrayGetBool(JsonValue* handle, size_t index, bool* out_value)
{
	if (!handle || !out_value) {
		return false;
	}

	if (handle->IsMutable()) {
		size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
		if (index >= arr_size) {
			return false;
		}

		yyjson_mut_val* val = yyjson_mut_arr_get(handle->m_pVal_mut, index);
		if (!yyjson_mut_is_bool(val)) {
			return false;
		}

		*out_value = yyjson_mut_get_bool(val);
		return true;
	} else {
		size_t arr_size = yyjson_arr_size(handle->m_pVal);
		if (index >= arr_size) {
			return false;
		}

		yyjson_val* val = yyjson_arr_get(handle->m_pVal, index);
		if (!yyjson_is_bool(val)) {
			return false;
		}

		*out_value = yyjson_get_bool(val);
		return true;
	}
}

bool JsonManager::ArrayGetFloat(JsonValue* handle, size_t index, double* out_value)
{
	if (!handle || !out_value) {
		return false;
	}

	if (handle->IsMutable()) {
		size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
		if (index >= arr_size) {
			return false;
		}

		yyjson_mut_val* val = yyjson_mut_arr_get(handle->m_pVal_mut, index);
		if (!yyjson_mut_is_real(val)) {
			return false;
		}

		*out_value = yyjson_mut_get_real(val);
		return true;
	} else {
		size_t arr_size = yyjson_arr_size(handle->m_pVal);
		if (index >= arr_size) {
			return false;
		}

		yyjson_val* val = yyjson_arr_get(handle->m_pVal, index);
		if (!yyjson_is_real(val)) {
			return false;
		}

		*out_value = yyjson_get_real(val);
		return true;
	}
}

bool JsonManager::ArrayGetInt(JsonValue* handle, size_t index, int* out_value)
{
	if (!handle || !out_value) {
		return false;
	}

	if (handle->IsMutable()) {
		size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
		if (index >= arr_size) {
			return false;
		}

		yyjson_mut_val* val = yyjson_mut_arr_get(handle->m_pVal_mut, index);
		if (!yyjson_mut_is_int(val)) {
			return false;
		}

		*out_value = yyjson_mut_get_int(val);
		return true;
	} else {
		size_t arr_size = yyjson_arr_size(handle->m_pVal);
		if (index >= arr_size) {
			return false;
		}

		yyjson_val* val = yyjson_arr_get(handle->m_pVal, index);
		if (!yyjson_is_int(val)) {
			return false;
		}

		*out_value = yyjson_get_int(val);
		return true;
	}
}

bool JsonManager::ArrayGetInt64(JsonValue* handle, size_t index, std::variant<int64_t, uint64_t>* out_value)
{
	if (!handle || !out_value) {
		return false;
	}

	if (handle->IsMutable()) {
		size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
		if (index >= arr_size) {
			return false;
		}

		yyjson_mut_val* val = yyjson_mut_arr_get(handle->m_pVal_mut, index);
		if (!yyjson_mut_is_int(val)) {
			return false;
		}

		ReadInt64FromMutVal(val, out_value);
		return true;
	} else {
		size_t arr_size = yyjson_arr_size(handle->m_pVal);
		if (index >= arr_size) {
			return false;
		}

		yyjson_val* val = yyjson_arr_get(handle->m_pVal, index);
		if (!yyjson_is_int(val)) {
			return false;
		}

		ReadInt64FromVal(val, out_value);
		return true;
	}
}

bool JsonManager::ArrayGetString(JsonValue* handle, size_t index, const char** out_str, size_t* out_len)
{
	if (!handle || !out_str) {
		return false;
	}

	if (handle->IsMutable()) {
		size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
		if (index >= arr_size) {
			return false;
		}

		yyjson_mut_val* val = yyjson_mut_arr_get(handle->m_pVal_mut, index);
		if (!yyjson_mut_is_str(val)) {
			return false;
		}

		*out_str = yyjson_mut_get_str(val);
		if (out_len) {
			*out_len = yyjson_mut_get_len(val);
		}
		return true;
	} else {
		size_t arr_size = yyjson_arr_size(handle->m_pVal);
		if (index >= arr_size) {
			return false;
		}

		yyjson_val* val = yyjson_arr_get(handle->m_pVal, index);
		if (!yyjson_is_str(val)) {
			return false;
		}

		*out_str = yyjson_get_str(val);
		if (out_len) {
			*out_len = yyjson_get_len(val);
		}
		return true;
	}
}

bool JsonManager::ArrayIsNull(JsonValue* handle, size_t index)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
		if (index >= arr_size) {
			return false;
		}

		yyjson_mut_val* val = yyjson_mut_arr_get(handle->m_pVal_mut, index);
		return yyjson_mut_is_null(val);
	} else {
		size_t arr_size = yyjson_arr_size(handle->m_pVal);
		if (index >= arr_size) {
			return false;
		}

		yyjson_val* val = yyjson_arr_get(handle->m_pVal, index);
		return yyjson_is_null(val);
	}
}

bool JsonManager::ArrayReplace(JsonValue* handle, size_t index, JsonValue* value)
{
	if (!handle || !handle->IsMutable() || !value) {
		return false;
	}

	size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
	if (index >= arr_size) {
		return false;
	}

	yyjson_mut_val* val_copy = nullptr;
	if (value->IsMutable()) {
		val_copy = yyjson_mut_val_mut_copy(handle->m_pDocument_mut.get(), value->m_pVal_mut);
	} else {
		val_copy = yyjson_val_mut_copy(handle->m_pDocument_mut.get(), value->m_pVal);
	}

	if (!val_copy) {
		return false;
	}

	return yyjson_mut_arr_replace(handle->m_pVal_mut, index, val_copy) != nullptr;
}

bool JsonManager::ArrayReplaceBool(JsonValue* handle, size_t index, bool value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
	if (index >= arr_size) {
		return false;
	}

	return yyjson_mut_arr_replace(handle->m_pVal_mut, index, yyjson_mut_bool(handle->m_pDocument_mut.get(), value)) != nullptr;
}

bool JsonManager::ArrayReplaceFloat(JsonValue* handle, size_t index, double value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
	if (index >= arr_size) {
		return false;
	}

	return yyjson_mut_arr_replace(handle->m_pVal_mut, index, yyjson_mut_real(handle->m_pDocument_mut.get(), value)) != nullptr;
}

bool JsonManager::ArrayReplaceInt(JsonValue* handle, size_t index, int value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
	if (index >= arr_size) {
		return false;
	}

	return yyjson_mut_arr_replace(handle->m_pVal_mut, index, yyjson_mut_int(handle->m_pDocument_mut.get(), value)) != nullptr;
}

bool JsonManager::ArrayReplaceInt64(JsonValue* handle, size_t index, std::variant<int64_t, uint64_t> value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
	if (index >= arr_size) {
		return false;
	}

	return std::visit([&](auto&& val) -> bool {
		using T = std::decay_t<decltype(val)>;
		if constexpr (std::is_same_v<T, int64_t>) {
			return yyjson_mut_arr_replace(handle->m_pVal_mut, index, yyjson_mut_sint(handle->m_pDocument_mut.get(), val)) != nullptr;
		} else if constexpr (std::is_same_v<T, uint64_t>) {
			return yyjson_mut_arr_replace(handle->m_pVal_mut, index, yyjson_mut_uint(handle->m_pDocument_mut.get(), val)) != nullptr;
		}
		return false;
	}, value);
}

bool JsonManager::ArrayReplaceNull(JsonValue* handle, size_t index)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
	if (index >= arr_size) {
		return false;
	}

	return yyjson_mut_arr_replace(handle->m_pVal_mut, index, yyjson_mut_null(handle->m_pDocument_mut.get())) != nullptr;
}

bool JsonManager::ArrayReplaceString(JsonValue* handle, size_t index, const char* value)
{
	if (!handle || !handle->IsMutable() || !value) {
		return false;
	}

	size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
	if (index >= arr_size) {
		return false;
	}

	return yyjson_mut_arr_replace(handle->m_pVal_mut, index, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), value)) != nullptr;
}

bool JsonManager::ArrayAppend(JsonValue* handle, JsonValue* value)
{
	if (!handle || !handle->IsMutable() || !value) {
		return false;
	}

	yyjson_mut_val* val_copy = nullptr;
	if (value->IsMutable()) {
		val_copy = yyjson_mut_val_mut_copy(handle->m_pDocument_mut.get(), value->m_pVal_mut);
	} else {
		val_copy = yyjson_val_mut_copy(handle->m_pDocument_mut.get(), value->m_pVal);
	}

	if (!val_copy) {
		return false;
	}

	return yyjson_mut_arr_append(handle->m_pVal_mut, val_copy);
}

bool JsonManager::ArrayAppendBool(JsonValue* handle, bool value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_append(handle->m_pVal_mut, yyjson_mut_bool(handle->m_pDocument_mut.get(), value));
}

bool JsonManager::ArrayAppendFloat(JsonValue* handle, double value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_append(handle->m_pVal_mut, yyjson_mut_real(handle->m_pDocument_mut.get(), value));
}

bool JsonManager::ArrayAppendInt(JsonValue* handle, int value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_append(handle->m_pVal_mut, yyjson_mut_int(handle->m_pDocument_mut.get(), value));
}

bool JsonManager::ArrayAppendInt64(JsonValue* handle, std::variant<int64_t, uint64_t> value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return std::visit([&](auto&& val) -> bool {
		using T = std::decay_t<decltype(val)>;
		if constexpr (std::is_same_v<T, int64_t>) {
			return yyjson_mut_arr_append(handle->m_pVal_mut, yyjson_mut_sint(handle->m_pDocument_mut.get(), val));
		} else if constexpr (std::is_same_v<T, uint64_t>) {
			return yyjson_mut_arr_append(handle->m_pVal_mut, yyjson_mut_uint(handle->m_pDocument_mut.get(), val));
		}
		return false;
	}, value);
}

bool JsonManager::ArrayAppendNull(JsonValue* handle)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_append(handle->m_pVal_mut, yyjson_mut_null(handle->m_pDocument_mut.get()));
}

bool JsonManager::ArrayAppendString(JsonValue* handle, const char* value)
{
	if (!handle || !handle->IsMutable() || !value) {
		return false;
	}

	return yyjson_mut_arr_append(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), value));
}

bool JsonManager::ArrayInsert(JsonValue* handle, size_t index, JsonValue* value)
{
	if (!handle || !handle->IsMutable() || !value) {
		return false;
	}

	return yyjson_mut_arr_insert(handle->m_pVal_mut, value->m_pVal_mut, index);
}

bool JsonManager::ArrayInsertBool(JsonValue* handle, size_t index, bool value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_insert(handle->m_pVal_mut, yyjson_mut_bool(handle->m_pDocument_mut.get(), value), index);
}

bool JsonManager::ArrayInsertInt(JsonValue* handle, size_t index, int value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_insert(handle->m_pVal_mut, yyjson_mut_sint(handle->m_pDocument_mut.get(), value), index);
}

bool JsonManager::ArrayInsertInt64(JsonValue* handle, size_t index, std::variant<int64_t, uint64_t> value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	yyjson_mut_val* val = nullptr;
	std::visit([&](auto&& arg) {
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, int64_t>) {
			val = yyjson_mut_sint(handle->m_pDocument_mut.get(), arg);
		} else if constexpr (std::is_same_v<T, uint64_t>) {
			val = yyjson_mut_uint(handle->m_pDocument_mut.get(), arg);
		}
	}, value);

	if (!val) {
		return false;
	}

	return yyjson_mut_arr_insert(handle->m_pVal_mut, val, index);
}

bool JsonManager::ArrayInsertFloat(JsonValue* handle, size_t index, double value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_insert(handle->m_pVal_mut, yyjson_mut_real(handle->m_pDocument_mut.get(), value), index);
}

bool JsonManager::ArrayInsertString(JsonValue* handle, size_t index, const char* value)
{
	if (!handle || !handle->IsMutable() || !value) {
		return false;
	}

	return yyjson_mut_arr_insert(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), value), index);
}

bool JsonManager::ArrayInsertNull(JsonValue* handle, size_t index)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_insert(handle->m_pVal_mut, yyjson_mut_null(handle->m_pDocument_mut.get()), index);
}

bool JsonManager::ArrayPrepend(JsonValue* handle, JsonValue* value)
{
	if (!handle || !handle->IsMutable() || !value) {
		return false;
	}

	return yyjson_mut_arr_prepend(handle->m_pVal_mut, value->m_pVal_mut);
}

bool JsonManager::ArrayPrependBool(JsonValue* handle, bool value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_prepend(handle->m_pVal_mut, yyjson_mut_bool(handle->m_pDocument_mut.get(), value));
}

bool JsonManager::ArrayPrependInt(JsonValue* handle, int value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_prepend(handle->m_pVal_mut, yyjson_mut_sint(handle->m_pDocument_mut.get(), value));
}

bool JsonManager::ArrayPrependInt64(JsonValue* handle, std::variant<int64_t, uint64_t> value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	yyjson_mut_val* val = nullptr;
	std::visit([&](auto&& arg) {
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, int64_t>) {
			val = yyjson_mut_sint(handle->m_pDocument_mut.get(), arg);
		} else if constexpr (std::is_same_v<T, uint64_t>) {
			val = yyjson_mut_uint(handle->m_pDocument_mut.get(), arg);
		}
	}, value);

	if (!val) {
		return false;
	}

	return yyjson_mut_arr_prepend(handle->m_pVal_mut, val);
}

bool JsonManager::ArrayPrependFloat(JsonValue* handle, double value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_prepend(handle->m_pVal_mut, yyjson_mut_real(handle->m_pDocument_mut.get(), value));
}

bool JsonManager::ArrayPrependString(JsonValue* handle, const char* value)
{
	if (!handle || !handle->IsMutable() || !value) {
		return false;
	}

	return yyjson_mut_arr_prepend(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), value));
}

bool JsonManager::ArrayPrependNull(JsonValue* handle)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_prepend(handle->m_pVal_mut, yyjson_mut_null(handle->m_pDocument_mut.get()));
}

bool JsonManager::ArrayRemove(JsonValue* handle, size_t index)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
	if (index >= arr_size) {
		return false;
	}

	return yyjson_mut_arr_remove(handle->m_pVal_mut, index) != nullptr;
}

bool JsonManager::ArrayRemoveFirst(JsonValue* handle)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	if (yyjson_mut_arr_size(handle->m_pVal_mut) == 0) {
		return false;
	}

	return yyjson_mut_arr_remove_first(handle->m_pVal_mut) != nullptr;
}

bool JsonManager::ArrayRemoveLast(JsonValue* handle)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	if (yyjson_mut_arr_size(handle->m_pVal_mut) == 0) {
		return false;
	}

	return yyjson_mut_arr_remove_last(handle->m_pVal_mut) != nullptr;
}

bool JsonManager::ArrayRemoveRange(JsonValue* handle, size_t start_index, size_t end_index)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);

	if (start_index >= arr_size || end_index > arr_size || start_index > end_index) {
		return false;
	}

	return yyjson_mut_arr_remove_range(handle->m_pVal_mut, start_index, end_index);
}

bool JsonManager::ArrayClear(JsonValue* handle)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_clear(handle->m_pVal_mut);
}

int JsonManager::ArrayIndexOfBool(JsonValue* handle, bool search_value)
{
	if (!handle) {
		return -1;
	}

	if (handle->IsMutable()) {
		size_t idx, max;
		yyjson_mut_val *val;
		yyjson_mut_arr_foreach(handle->m_pVal_mut, idx, max, val) {
			if (yyjson_mut_is_bool(val) && yyjson_mut_get_bool(val) == search_value) {
				return static_cast<int>(idx);
			}
		}
	} else {
		size_t idx, max;
		yyjson_val *val;
		yyjson_arr_foreach(handle->m_pVal, idx, max, val) {
			if (yyjson_is_bool(val) && yyjson_get_bool(val) == search_value) {
				return static_cast<int>(idx);
			}
		}
	}

	return -1;
}

int JsonManager::ArrayIndexOfString(JsonValue* handle, const char* search_value)
{
	if (!handle || !search_value) {
		return -1;
	}

	if (handle->IsMutable()) {
		size_t idx, max;
		yyjson_mut_val *val;
		yyjson_mut_arr_foreach(handle->m_pVal_mut, idx, max, val) {
			if (yyjson_mut_is_str(val) && strcmp(yyjson_mut_get_str(val), search_value) == 0) {
				return static_cast<int>(idx);
			}
		}
	} else {
		size_t idx, max;
		yyjson_val *val;
		yyjson_arr_foreach(handle->m_pVal, idx, max, val) {
			if (yyjson_is_str(val) && strcmp(yyjson_get_str(val), search_value) == 0) {
				return static_cast<int>(idx);
			}
		}
	}

	return -1;
}

int JsonManager::ArrayIndexOfInt(JsonValue* handle, int search_value)
{
	if (!handle) {
		return -1;
	}

	if (handle->IsMutable()) {
		size_t idx, max;
		yyjson_mut_val *val;
		yyjson_mut_arr_foreach(handle->m_pVal_mut, idx, max, val) {
			if (yyjson_mut_is_int(val) && yyjson_mut_get_int(val) == search_value) {
				return static_cast<int>(idx);
			}
		}
	} else {
		size_t idx, max;
		yyjson_val *val;
		yyjson_arr_foreach(handle->m_pVal, idx, max, val) {
			if (yyjson_is_int(val) && yyjson_get_int(val) == search_value) {
				return static_cast<int>(idx);
			}
		}
	}

	return -1;
}

int JsonManager::ArrayIndexOfInt64(JsonValue* handle, int64_t search_value)
{
	if (!handle) {
		return -1;
	}

	if (handle->IsMutable()) {
		size_t idx, max;
		yyjson_mut_val *val;
		yyjson_mut_arr_foreach(handle->m_pVal_mut, idx, max, val) {
			if (yyjson_mut_is_int(val) && yyjson_mut_get_sint(val) == search_value) {
				return static_cast<int>(idx);
			}
		}
	} else {
		size_t idx, max;
		yyjson_val *val;
		yyjson_arr_foreach(handle->m_pVal, idx, max, val) {
			if (yyjson_is_int(val) && yyjson_get_sint(val) == search_value) {
				return static_cast<int>(idx);
			}
		}
	}

	return -1;
}

int JsonManager::ArrayIndexOfUint64(JsonValue* handle, uint64_t search_value)
{
	if (!handle) {
		return -1;
	}

	if (handle->IsMutable()) {
		size_t idx, max;
		yyjson_mut_val *val;
		yyjson_mut_arr_foreach(handle->m_pVal_mut, idx, max, val) {
			if (yyjson_mut_is_int(val) && yyjson_mut_get_uint(val) == search_value) {
				return static_cast<int>(idx);
			}
		}
	} else {
		size_t idx, max;
		yyjson_val *val;
		yyjson_arr_foreach(handle->m_pVal, idx, max, val) {
			if (yyjson_is_int(val) && yyjson_get_uint(val) == search_value) {
				return static_cast<int>(idx);
			}
		}
	}

	return -1;
}

int JsonManager::ArrayIndexOfFloat(JsonValue* handle, double search_value)
{
	if (!handle) {
		return -1;
	}

	if (handle->IsMutable()) {
		size_t idx, max;
		yyjson_mut_val *val;
		yyjson_mut_arr_foreach(handle->m_pVal_mut, idx, max, val) {
			if (yyjson_mut_is_real(val)) {
				double val_num = yyjson_mut_get_real(val);
				if (yyjson_equals_fp(val_num, search_value)) {
					return static_cast<int>(idx);
				}
			}
		}
	} else {
		size_t idx, max;
		yyjson_val *val;
		yyjson_arr_foreach(handle->m_pVal, idx, max, val) {
			if (yyjson_is_real(val)) {
				double val_num = yyjson_get_real(val);
				if (yyjson_equals_fp(val_num, search_value)) {
					return static_cast<int>(idx);
				}
			}
		}
	}

	return -1;
}

bool JsonManager::ArraySort(JsonValue* handle, JSON_SORT_ORDER sort_mode)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	if (!yyjson_mut_is_arr(handle->m_pVal_mut)) {
		return false;
	}

	if (sort_mode < JSON_SORT_ASC || sort_mode > JSON_SORT_RANDOM) {
		return false;
	}

	size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
	if (arr_size <= 1) return true;

	struct ValueInfo {
		yyjson_mut_val* val;
		uint8_t type;
		uint8_t subtype;  // 0=float, 1=signed int, 2=unsigned int
	};

	std::vector<ValueInfo> values;
	values.reserve(arr_size);

	size_t idx, max;
	yyjson_mut_val *val;
	yyjson_mut_arr_foreach(handle->m_pVal_mut, idx, max, val) {
		uint8_t type = yyjson_mut_get_type(val);
		uint8_t subtype = 0;

		if (type == YYJSON_TYPE_NUM) {
			if (yyjson_mut_is_int(val)) {
				subtype = yyjson_mut_is_sint(val) ? 1 : 2;
			}
		}

		values.push_back({val, type, subtype});
	}

	if (sort_mode == JSON_SORT_RANDOM) {
		std::shuffle(values.begin(), values.end(), m_randomGenerator);
	}
	else {
		auto compare = [sort_mode](const ValueInfo& a, const ValueInfo& b) {
			if (a.val == b.val) return false;

			if (a.type != b.type) {
				return sort_mode == JSON_SORT_ASC ? a.type < b.type : a.type > b.type;
			}

			switch (a.type) {
			case YYJSON_TYPE_STR: {
				const char* str_a = yyjson_mut_get_str(a.val);
				const char* str_b = yyjson_mut_get_str(b.val);
				int cmp = strcmp(str_a, str_b);
				return sort_mode == JSON_SORT_ASC ? cmp < 0 : cmp > 0;
			}
			case YYJSON_TYPE_NUM: {
				if (a.subtype > 0 && b.subtype > 0) {
					if (a.subtype == 1 && b.subtype == 1) {
						int64_t num_a = yyjson_mut_get_sint(a.val);
						int64_t num_b = yyjson_mut_get_sint(b.val);
						return sort_mode == JSON_SORT_ASC ? num_a < num_b : num_a > num_b;
					}
					else if (a.subtype == 2 && b.subtype == 2) {
						uint64_t num_a = yyjson_mut_get_uint(a.val);
						uint64_t num_b = yyjson_mut_get_uint(b.val);
						return sort_mode == JSON_SORT_ASC ? num_a < num_b : num_a > num_b;
					}
					else {
						int64_t signed_val;
						uint64_t unsigned_val;
						bool a_is_signed = (a.subtype == 1);

						if (a_is_signed) {
							signed_val = yyjson_mut_get_sint(a.val);
							unsigned_val = yyjson_mut_get_uint(b.val);

							if (signed_val < 0) {
								return sort_mode == JSON_SORT_ASC;
							}
							uint64_t a_as_unsigned = static_cast<uint64_t>(signed_val);
							return sort_mode == JSON_SORT_ASC ?
								a_as_unsigned < unsigned_val :
								a_as_unsigned > unsigned_val;
						} else {
							unsigned_val = yyjson_mut_get_uint(a.val);
							signed_val = yyjson_mut_get_sint(b.val);

							if (signed_val < 0) {
								return sort_mode == JSON_SORT_DESC;
							}
							uint64_t b_as_unsigned = static_cast<uint64_t>(signed_val);
							return sort_mode == JSON_SORT_ASC ?
								unsigned_val < b_as_unsigned :
								unsigned_val > b_as_unsigned;
						}
					}
				}
				double num_a = yyjson_mut_get_num(a.val);
				double num_b = yyjson_mut_get_num(b.val);
				return sort_mode == JSON_SORT_ASC ? num_a < num_b : num_a > num_b;
			}
			case YYJSON_TYPE_BOOL: {
				bool val_a = yyjson_mut_get_bool(a.val);
				bool val_b = yyjson_mut_get_bool(b.val);
				return sort_mode == JSON_SORT_ASC ? val_a < val_b : val_a > val_b;
			}
			default:
				return false;
			}
		};

		std::sort(values.begin(), values.end(), compare);
	}

	yyjson_mut_arr_clear(handle->m_pVal_mut);
	for (const auto& info : values) {
		yyjson_mut_arr_append(handle->m_pVal_mut, info.val);
	}

	return true;
}

const char* JsonManager::SkipSeparators(const char* ptr)
{
	while (*ptr && (isspace(*ptr) || *ptr == ':' || *ptr == ',')) {
		ptr++;
	}
	return ptr;
}

yyjson_mut_val* JsonManager::PackImpl(yyjson_mut_doc* doc, const char* format,
                                          IPackParamProvider* provider,
                                          char* error, size_t error_size,
                                          const char** out_end_ptr)
{
	if (!doc || !format || !*format) {
		SetErrorSafe(error, error_size, "Invalid argument(s)");
		return nullptr;
	}

	yyjson_mut_val* root = nullptr;
	const char* ptr = format;

	bool is_obj = false;
	if (*ptr == '{') {
		root = yyjson_mut_obj(doc);
		is_obj = true;
		ptr = SkipSeparators(ptr + 1);
	} else if (*ptr == '[') {
		root = yyjson_mut_arr(doc);
		ptr = SkipSeparators(ptr + 1);
	} else {
		SetErrorSafe(error, error_size, "Invalid format string: expected '{' or '['");
		return nullptr;
	}

	if (!root) {
		SetErrorSafe(error, error_size, "Failed to create root object/array");
		return nullptr;
	}

	yyjson_mut_val* key_val = nullptr;
	yyjson_mut_val* val = nullptr;

	while (*ptr && *ptr != '}' && *ptr != ']') {
		if (is_obj) {
			if (*ptr != 's') {
				SetErrorSafe(error, error_size, "Object key must be string, got '%c'", *ptr);
				return nullptr;
			}
		}
		switch (*ptr) {
			case 's': {
				if (is_obj) {
					const char* key;
					if (!provider->GetNextString(&key)) {
						SetErrorSafe(error, error_size, "Invalid string key");
						return nullptr;
					}
					key_val = yyjson_mut_strcpy(doc, key);
					if (!key_val) {
						SetErrorSafe(error, error_size, "Failed to create key");
						return nullptr;
					}

					ptr = SkipSeparators(ptr + 1);
					if (*ptr != 's' && *ptr != 'i' && *ptr != 'f' && *ptr != 'b' &&
						*ptr != 'n' && *ptr != '{' && *ptr != '[') {
							SetErrorSafe(error, error_size, "Invalid value type after key");
						return nullptr;
					}

					if (*ptr == '{' || *ptr == '[') {
						val = PackImpl(doc, ptr, provider, error, error_size, &ptr);
						if (!val) {
							return nullptr;
						}
					} else {
						switch (*ptr) {
							case 's': {
								const char* val_str;
								if (!provider->GetNextString(&val_str)) {
									SetErrorSafe(error, error_size, "Invalid string value");
									return nullptr;
								}
								val = yyjson_mut_strcpy(doc, val_str);
								ptr++;
								break;
							}
							case 'i': {
								int val_int;
								if (!provider->GetNextInt(&val_int)) {
									SetErrorSafe(error, error_size, "Invalid integer value");
									return nullptr;
								}
								val = yyjson_mut_int(doc, val_int);
								ptr++;
								break;
							}
							case 'f': {
								float val_float;
								if (!provider->GetNextFloat(&val_float)) {
									SetErrorSafe(error, error_size, "Invalid float value");
									return nullptr;
								}
								val = yyjson_mut_real(doc, val_float);
								ptr++;
								break;
							}
							case 'b': {
								bool val_bool;
								if (!provider->GetNextBool(&val_bool)) {
									SetErrorSafe(error, error_size, "Invalid boolean value");
									return nullptr;
								}
								val = yyjson_mut_bool(doc, val_bool);
								ptr++;
								break;
							}
							case 'n': {
								val = yyjson_mut_null(doc);
								ptr++;
								break;
							}
						}
					}

					if (!val) {
						SetErrorSafe(error, error_size, "Failed to create value");
						return nullptr;
					}

					if (!yyjson_mut_obj_add(root, key_val, val)) {
						SetErrorSafe(error, error_size, "Failed to add value to object");
						return nullptr;
					}
				} else {
					const char* val_str;
					if (!provider->GetNextString(&val_str)) {
						SetErrorSafe(error, error_size, "Invalid string value");
						return nullptr;
					}
					if (!yyjson_mut_arr_add_strcpy(doc, root, val_str)) {
						SetErrorSafe(error, error_size, "Failed to add string to array");
						return nullptr;
					}
					ptr++;
				}
				break;
			}
			case 'i': {
				int val_int;
				if (!provider->GetNextInt(&val_int)) {
					SetErrorSafe(error, error_size, "Invalid integer value");
					return nullptr;
				}
				if (!yyjson_mut_arr_add_int(doc, root, val_int)) {
					SetErrorSafe(error, error_size, "Failed to add integer to array");
					return nullptr;
				}
				ptr++;
				break;
			}
			case 'b': {
				bool val_bool;
				if (!provider->GetNextBool(&val_bool)) {
					SetErrorSafe(error, error_size, "Invalid boolean value");
					return nullptr;
				}
				if (!yyjson_mut_arr_add_bool(doc, root, val_bool)) {
					SetErrorSafe(error, error_size, "Failed to add boolean to array");
					return nullptr;
				}
				ptr++;
				break;
			}
			case 'n': {
				if (!yyjson_mut_arr_add_null(doc, root)) {
					SetErrorSafe(error, error_size, "Failed to add null to array");
					return nullptr;
				}
				ptr++;
				break;
			}
			case 'f': {
				float val_float;
				if (!provider->GetNextFloat(&val_float)) {
					SetErrorSafe(error, error_size, "Invalid float value");
					return nullptr;
				}
				if (!yyjson_mut_arr_add_real(doc, root, val_float)) {
					SetErrorSafe(error, error_size, "Failed to add float to array");
					return nullptr;
				}
				ptr++;
				break;
			}
			case '{':
			case '[': {
				val = PackImpl(doc, ptr, provider, error, error_size, &ptr);
				if (!val) {
					return nullptr;
				}
				if (!yyjson_mut_arr_append(root, val)) {
					SetErrorSafe(error, error_size, "Failed to add nested value to array");
					return nullptr;
				}
				break;
			}
			default: {
				SetErrorSafe(error, error_size, "Invalid format character: %c", *ptr);
				return nullptr;
			}
		}
		ptr = SkipSeparators(ptr);
	}

	if (*ptr != (is_obj ? '}' : ']')) {
		SetErrorSafe(error, error_size, "Unexpected end of format string");
		return nullptr;
	}

	if (out_end_ptr) {
		*out_end_ptr = ptr + 1;
	}

	return root;
}

JsonValue* JsonManager::Pack(const char* format, IPackParamProvider* param_provider, char* error, size_t error_size)
{
	if (!format || !param_provider) {
		SetErrorSafe(error, error_size, "Invalid arguments");
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CreateDocument();

	if (!pJSONValue->m_pDocument_mut) {
		SetErrorSafe(error, error_size, "Failed to create document");
		return nullptr;
	}

	const char* end_ptr = nullptr;
	pJSONValue->m_pVal_mut = PackImpl(pJSONValue->m_pDocument_mut.get(), format,
	                                       param_provider, error, error_size, &end_ptr);

	if (!pJSONValue->m_pVal_mut) {
		return nullptr;
	}

	yyjson_mut_doc_set_root(pJSONValue->m_pDocument_mut.get(), pJSONValue->m_pVal_mut);

	return pJSONValue.release();
}

JsonValue* JsonManager::CreateBool(bool value)
{
	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CreateDocument();
	pJSONValue->m_pVal_mut = yyjson_mut_bool(pJSONValue->m_pDocument_mut.get(), value);

	if (!pJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pJSONValue.release();
}

JsonValue* JsonManager::CreateFloat(double value)
{
	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CreateDocument();
	pJSONValue->m_pVal_mut = yyjson_mut_real(pJSONValue->m_pDocument_mut.get(), value);

	if (!pJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pJSONValue.release();
}

JsonValue* JsonManager::CreateInt(int value)
{
	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CreateDocument();
	pJSONValue->m_pVal_mut = yyjson_mut_int(pJSONValue->m_pDocument_mut.get(), value);

	if (!pJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pJSONValue.release();
}

JsonValue* JsonManager::CreateInt64(std::variant<int64_t, uint64_t> value)
{
	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CreateDocument();

	std::visit([&](auto&& val) {
		using T = std::decay_t<decltype(val)>;
		if constexpr (std::is_same_v<T, int64_t>) {
			pJSONValue->m_pVal_mut = yyjson_mut_sint(pJSONValue->m_pDocument_mut.get(), val);
		} else if constexpr (std::is_same_v<T, uint64_t>) {
			pJSONValue->m_pVal_mut = yyjson_mut_uint(pJSONValue->m_pDocument_mut.get(), val);
		}
	}, value);

	if (!pJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pJSONValue.release();
}

JsonValue* JsonManager::CreateNull()
{
	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CreateDocument();
	pJSONValue->m_pVal_mut = yyjson_mut_null(pJSONValue->m_pDocument_mut.get());

	if (!pJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pJSONValue.release();
}

JsonValue* JsonManager::CreateString(const char* value)
{
	if (!value) {
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CreateDocument();
	pJSONValue->m_pVal_mut = yyjson_mut_strcpy(pJSONValue->m_pDocument_mut.get(), value);

	if (!pJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pJSONValue.release();
}

bool JsonManager::GetBool(JsonValue* handle, bool* out_value)
{
	if (!handle || !out_value) {
		return false;
	}

	if (handle->IsMutable()) {
		if (!yyjson_mut_is_bool(handle->m_pVal_mut)) {
			return false;
		}
		*out_value = yyjson_mut_get_bool(handle->m_pVal_mut);
		return true;
	} else {
		if (!yyjson_is_bool(handle->m_pVal)) {
			return false;
		}
		*out_value = yyjson_get_bool(handle->m_pVal);
		return true;
	}
}

bool JsonManager::GetFloat(JsonValue* handle, double* out_value)
{
	if (!handle || !out_value) {
		return false;
	}

	if (handle->IsMutable()) {
		if (!yyjson_mut_is_real(handle->m_pVal_mut)) {
			return false;
		}
		*out_value = yyjson_mut_get_real(handle->m_pVal_mut);
		return true;
	} else {
		if (!yyjson_is_real(handle->m_pVal)) {
			return false;
		}
		*out_value = yyjson_get_real(handle->m_pVal);
		return true;
	}
}

bool JsonManager::GetInt(JsonValue* handle, int* out_value)
{
	if (!handle || !out_value) {
		return false;
	}

	if (handle->IsMutable()) {
		if (!yyjson_mut_is_int(handle->m_pVal_mut)) {
			return false;
		}
		*out_value = yyjson_mut_get_int(handle->m_pVal_mut);
		return true;
	} else {
		if (!yyjson_is_int(handle->m_pVal)) {
			return false;
		}
		*out_value = yyjson_get_int(handle->m_pVal);
		return true;
	}
}

bool JsonManager::GetInt64(JsonValue* handle, std::variant<int64_t, uint64_t>* out_value)
{
	if (!handle || !out_value) {
		return false;
	}

	if (handle->IsMutable()) {
		if (!yyjson_mut_is_int(handle->m_pVal_mut)) {
			return false;
		}
		ReadInt64FromMutVal(handle->m_pVal_mut, out_value);
		return true;
	} else {
		if (!yyjson_is_int(handle->m_pVal)) {
			return false;
		}
		ReadInt64FromVal(handle->m_pVal, out_value);
		return true;
	}
}

bool JsonManager::GetString(JsonValue* handle, const char** out_str, size_t* out_len)
{
	if (!handle || !out_str) {
		return false;
	}

	if (handle->IsMutable()) {
		if (!yyjson_mut_is_str(handle->m_pVal_mut)) {
			return false;
		}
		*out_str = yyjson_mut_get_str(handle->m_pVal_mut);
		if (out_len) {
			*out_len = yyjson_mut_get_len(handle->m_pVal_mut);
		}
		return true;
	} else {
		if (!yyjson_is_str(handle->m_pVal)) {
			return false;
		}
		*out_str = yyjson_get_str(handle->m_pVal);
		if (out_len) {
			*out_len = yyjson_get_len(handle->m_pVal);
		}
		return true;
	}
}

JsonValue* JsonManager::PtrGet(JsonValue* handle, const char* path, char* error, size_t error_size)
{
	if (!handle || !path) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters");
		}
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();
	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (!val || ptrGetError.code) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)",
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return nullptr;
		}

		pJSONValue->m_pDocument_mut = handle->m_pDocument_mut;
		pJSONValue->m_pVal_mut = val;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (!val || ptrGetError.code) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)",
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return nullptr;
		}

		pJSONValue->m_pDocument = handle->m_pDocument;
		pJSONValue->m_pVal = val;
	}

	return pJSONValue.release();
}

bool JsonManager::PtrGetBool(JsonValue* handle, const char* path, bool* out_value, char* error, size_t error_size)
{
	if (!handle || !path || !out_value) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters");
		}
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)",
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_mut_is_bool(val)) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Type mismatch at path '%s': expected boolean value, got %s", path, yyjson_mut_get_type_desc(val));
			}
			return false;
		}

		*out_value = yyjson_mut_get_bool(val);
		return true;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)",
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_is_bool(val)) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Type mismatch at path '%s': expected boolean value, got %s", path, yyjson_get_type_desc(val));
			}
			return false;
		}

		*out_value = yyjson_get_bool(val);
		return true;
	}
}

bool JsonManager::PtrGetFloat(JsonValue* handle, const char* path, double* out_value, char* error, size_t error_size)
{
	if (!handle || !path || !out_value) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters");
		}
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)",
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_mut_is_real(val)) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Type mismatch at path '%s': expected float value, got %s", path, yyjson_mut_get_type_desc(val));
			}
			return false;
		}

		*out_value = yyjson_mut_get_real(val);
		return true;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)",
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_is_real(val)) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Type mismatch at path '%s': expected float value, got %s", path, yyjson_get_type_desc(val));
			}
			return false;
		}

		*out_value = yyjson_get_real(val);
		return true;
	}
}

bool JsonManager::PtrGetInt(JsonValue* handle, const char* path, int* out_value, char* error, size_t error_size)
{
	if (!handle || !path || !out_value) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters");
		}
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)",
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_mut_is_int(val)) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Type mismatch at path '%s': expected integer value, got %s", path, yyjson_mut_get_type_desc(val));
			}
			return false;
		}

		*out_value = yyjson_mut_get_int(val);
		return true;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)",
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_is_int(val)) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Type mismatch at path '%s': expected integer value, got %s", path, yyjson_get_type_desc(val));
			}
			return false;
		}

		*out_value = yyjson_get_int(val);
		return true;
	}
}

bool JsonManager::PtrGetInt64(JsonValue* handle, const char* path, std::variant<int64_t, uint64_t>* out_value, char* error, size_t error_size)
{
	if (!handle || !path || !out_value) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters");
		}
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (!val || ptrGetError.code) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)",
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_mut_is_int(val)) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Type mismatch at path '%s': expected integer64 value, got %s", path, yyjson_mut_get_type_desc(val));
			}
			return false;
		}

		ReadInt64FromMutVal(val, out_value);
		return true;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (!val || ptrGetError.code) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)",
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_is_int(val)) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Type mismatch at path '%s': expected integer64 value, got %s", path, yyjson_get_type_desc(val));
			}
			return false;
		}

		ReadInt64FromVal(val, out_value);
		return true;
	}
}

bool JsonManager::PtrGetString(JsonValue* handle, const char* path, const char** out_str, size_t* out_len, char* error, size_t error_size)
{
	if (!handle || !path || !out_str) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters");
		}
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)",
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_mut_is_str(val)) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Type mismatch at path '%s': expected string value, got %s", path, yyjson_mut_get_type_desc(val));
			}
			return false;
		}

		*out_str = yyjson_mut_get_str(val);
		if (out_len) {
			*out_len = yyjson_mut_get_len(val);
		}
		return true;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)",
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_is_str(val)) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Type mismatch at path '%s': expected string value, got %s", path, yyjson_get_type_desc(val));
			}
			return false;
		}

		*out_str = yyjson_get_str(val);
		if (out_len) {
			*out_len = yyjson_get_len(val);
		}
		return true;
	}
}

bool JsonManager::PtrGetIsNull(JsonValue* handle, const char* path, bool* out_is_null, char* error, size_t error_size)
{
	if (!handle || !path || !out_is_null) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters");
		}
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)",
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		*out_is_null = yyjson_mut_is_null(val);
		return true;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)",
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		*out_is_null = yyjson_is_null(val);
		return true;
	}
}

bool JsonManager::PtrGetLength(JsonValue* handle, const char* path, size_t* out_len, char* error, size_t error_size)
{
	if (!handle || !path || !out_len) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters");
		}
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)",
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (yyjson_mut_is_str(val)) {
			*out_len = yyjson_mut_get_len(val) + 1;
		} else {
			*out_len = yyjson_mut_get_len(val);
		}
		return true;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				SetErrorSafe(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)",
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (yyjson_is_str(val)) {
			*out_len = yyjson_get_len(val) + 1;
		} else {
			*out_len = yyjson_get_len(val);
		}
		return true;
	}
}

bool JsonManager::PtrSet(JsonValue* handle, const char* path, JsonValue* value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path || !value) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_mut_val* val_copy = nullptr;
	if (value->IsMutable()) {
		val_copy = yyjson_mut_val_mut_copy(handle->m_pDocument_mut.get(), value->m_pVal_mut);
	} else {
		val_copy = yyjson_val_mut_copy(handle->m_pDocument_mut.get(), value->m_pVal);
	}

	if (!val_copy) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Failed to copy JSON value");
		}
		return false;
	}

	yyjson_ptr_err ptrSetError;
	bool success = yyjson_mut_doc_ptr_setx(handle->m_pDocument_mut.get(), path, strlen(path), val_copy, true, nullptr, &ptrSetError);

	if (ptrSetError.code && error && error_size > 0) {
		SetErrorSafe(error, error_size, "Failed to set JSON pointer: %s (error code: %u, position: %zu, path: %s)",
			ptrSetError.msg, ptrSetError.code, ptrSetError.pos, path);
	}

	return success;
}

bool JsonManager::PtrSetBool(JsonValue* handle, const char* path, bool value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrSetError;
	bool success = yyjson_mut_doc_ptr_setx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_bool(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrSetError);

	if (ptrSetError.code && error && error_size > 0) {
		SetErrorSafe(error, error_size, "Failed to set JSON pointer: %s (error code: %u, position: %zu, path: %s)",
			ptrSetError.msg, ptrSetError.code, ptrSetError.pos, path);
	}

	return success;
}

bool JsonManager::PtrSetFloat(JsonValue* handle, const char* path, double value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrSetError;
	bool success = yyjson_mut_doc_ptr_setx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_real(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrSetError);

	if (ptrSetError.code && error && error_size > 0) {
		SetErrorSafe(error, error_size, "Failed to set JSON pointer: %s (error code: %u, position: %zu, path: %s)",
			ptrSetError.msg, ptrSetError.code, ptrSetError.pos, path);
	}

	return success;
}

bool JsonManager::PtrSetInt(JsonValue* handle, const char* path, int value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrSetError;
	bool success = yyjson_mut_doc_ptr_setx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_int(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrSetError);

	if (ptrSetError.code && error && error_size > 0) {
		SetErrorSafe(error, error_size, "Failed to set JSON pointer: %s (error code: %u, position: %zu, path: %s)",
			ptrSetError.msg, ptrSetError.code, ptrSetError.pos, path);
	}

	return success;
}

bool JsonManager::PtrSetInt64(JsonValue* handle, const char* path, std::variant<int64_t, uint64_t> value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrSetError;
	bool success = std::visit([&](auto&& val) -> bool {
		using T = std::decay_t<decltype(val)>;
		if constexpr (std::is_same_v<T, int64_t>) {
			return yyjson_mut_doc_ptr_setx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_sint(handle->m_pDocument_mut.get(), val), true, nullptr, &ptrSetError);
		} else if constexpr (std::is_same_v<T, uint64_t>) {
			return yyjson_mut_doc_ptr_setx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_uint(handle->m_pDocument_mut.get(), val), true, nullptr, &ptrSetError);
		}
		return false;
	}, value);

	if (!success && ptrSetError.code && error && error_size > 0) {
		SetErrorSafe(error, error_size, "Failed to set JSON pointer: %s (error code: %u, position: %zu, path: %s)",
			ptrSetError.msg, ptrSetError.code, ptrSetError.pos, path);
	}

	return success;
}

bool JsonManager::PtrSetString(JsonValue* handle, const char* path, const char* value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path || !value) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrSetError;
	bool success = yyjson_mut_doc_ptr_setx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_strcpy(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrSetError);

	if (ptrSetError.code && error && error_size > 0) {
		SetErrorSafe(error, error_size, "Failed to set JSON pointer: %s (error code: %u, position: %zu, path: %s)",
			ptrSetError.msg, ptrSetError.code, ptrSetError.pos, path);
	}

	return success;
}

bool JsonManager::PtrSetNull(JsonValue* handle, const char* path, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrSetError;
	bool success = yyjson_mut_doc_ptr_setx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_null(handle->m_pDocument_mut.get()), true, nullptr, &ptrSetError);

	if (ptrSetError.code && error && error_size > 0) {
		SetErrorSafe(error, error_size, "Failed to set JSON pointer: %s (error code: %u, position: %zu, path: %s)",
			ptrSetError.msg, ptrSetError.code, ptrSetError.pos, path);
	}

	return success;
}

bool JsonManager::PtrAdd(JsonValue* handle, const char* path, JsonValue* value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path || !value) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_mut_val* val_copy = nullptr;
	if (value->IsMutable()) {
		val_copy = yyjson_mut_val_mut_copy(handle->m_pDocument_mut.get(), value->m_pVal_mut);
	} else {
		val_copy = yyjson_val_mut_copy(handle->m_pDocument_mut.get(), value->m_pVal);
	}

	if (!val_copy) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Failed to copy JSON value");
		}
		return false;
	}

	yyjson_ptr_err ptrAddError;
	bool success = yyjson_mut_doc_ptr_addx(handle->m_pDocument_mut.get(), path, strlen(path), val_copy, true, nullptr, &ptrAddError);

	if (ptrAddError.code && error && error_size > 0) {
		SetErrorSafe(error, error_size, "Failed to add JSON pointer: %s (error code: %u, position: %zu, path: %s)",
			ptrAddError.msg, ptrAddError.code, ptrAddError.pos, path);
	}

	return success;
}

bool JsonManager::PtrAddBool(JsonValue* handle, const char* path, bool value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrAddError;
	bool success = yyjson_mut_doc_ptr_addx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_bool(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrAddError);

	if (ptrAddError.code && error && error_size > 0) {
		SetErrorSafe(error, error_size, "Failed to add JSON pointer: %s (error code: %u, position: %zu, path: %s)",
			ptrAddError.msg, ptrAddError.code, ptrAddError.pos, path);
	}

	return success;
}

bool JsonManager::PtrAddFloat(JsonValue* handle, const char* path, double value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrAddError;
	bool success = yyjson_mut_doc_ptr_addx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_real(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrAddError);

	if (ptrAddError.code && error && error_size > 0) {
		SetErrorSafe(error, error_size, "Failed to add JSON pointer: %s (error code: %u, position: %zu, path: %s)",
			ptrAddError.msg, ptrAddError.code, ptrAddError.pos, path);
	}

	return success;
}

bool JsonManager::PtrAddInt(JsonValue* handle, const char* path, int value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrAddError;
	bool success = yyjson_mut_doc_ptr_addx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_int(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrAddError);

	if (ptrAddError.code && error && error_size > 0) {
		SetErrorSafe(error, error_size, "Failed to add JSON pointer: %s (error code: %u, position: %zu, path: %s)",
			ptrAddError.msg, ptrAddError.code, ptrAddError.pos, path);
	}

	return success;
}

bool JsonManager::PtrAddInt64(JsonValue* handle, const char* path, std::variant<int64_t, uint64_t> value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrAddError;
	bool success = std::visit([&](auto&& val) -> bool {
		using T = std::decay_t<decltype(val)>;
		if constexpr (std::is_same_v<T, int64_t>) {
			return yyjson_mut_doc_ptr_addx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_sint(handle->m_pDocument_mut.get(), val), true, nullptr, &ptrAddError);
		} else if constexpr (std::is_same_v<T, uint64_t>) {
			return yyjson_mut_doc_ptr_addx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_uint(handle->m_pDocument_mut.get(), val), true, nullptr, &ptrAddError);
		}
		return false;
	}, value);

	if (ptrAddError.code && error && error_size > 0) {
		SetErrorSafe(error, error_size, "Failed to add JSON pointer: %s (error code: %u, position: %zu, path: %s)",
			ptrAddError.msg, ptrAddError.code, ptrAddError.pos, path);
	}

	return success;
}

bool JsonManager::PtrAddString(JsonValue* handle, const char* path, const char* value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path || !value) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrAddError;
	bool success = yyjson_mut_doc_ptr_addx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_strcpy(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrAddError);

	if (ptrAddError.code && error && error_size > 0) {
		SetErrorSafe(error, error_size, "Failed to add JSON pointer: %s (error code: %u, position: %zu, path: %s)",
			ptrAddError.msg, ptrAddError.code, ptrAddError.pos, path);
	}

	return success;
}

bool JsonManager::PtrAddNull(JsonValue* handle, const char* path, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrAddError;
	bool success = yyjson_mut_doc_ptr_addx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_null(handle->m_pDocument_mut.get()), true, nullptr, &ptrAddError);

	if (ptrAddError.code && error && error_size > 0) {
		SetErrorSafe(error, error_size, "Failed to add JSON pointer: %s (error code: %u, position: %zu, path: %s)",
			ptrAddError.msg, ptrAddError.code, ptrAddError.pos, path);
	}

	return success;
}

bool JsonManager::PtrRemove(JsonValue* handle, const char* path, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrRemoveError;
	bool success = yyjson_mut_doc_ptr_removex(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrRemoveError) != nullptr;

	if (ptrRemoveError.code && error && error_size > 0) {
		SetErrorSafe(error, error_size, "Failed to remove JSON pointer: %s (error code: %u, position: %zu, path: %s)",
			ptrRemoveError.msg, ptrRemoveError.code, ptrRemoveError.pos, path);
	}

	return success;
}

JsonManager::PtrGetValueResult JsonManager::PtrGetValueInternal(JsonValue* handle, const char* path)
{
	PtrGetValueResult result;
	result.success = false;

	if (!handle || !path) {
		return result;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		result.mut_val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);
		if (result.mut_val && !ptrGetError.code) {
			result.success = true;
		}
	} else {
		result.imm_val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);
		if (result.imm_val && !ptrGetError.code) {
			result.success = true;
		}
	}

	return result;
}

JsonValue* JsonManager::PtrTryGet(JsonValue* handle, const char* path)
{
	if (!handle || !path) {
			return nullptr;
		}

	auto result = PtrGetValueInternal(handle, path);
	if (!result.success) {
			return nullptr;
		}

	auto pJSONValue = CreateWrapper();
	if (handle->IsMutable()) {
		pJSONValue->m_pDocument_mut = handle->m_pDocument_mut;
		pJSONValue->m_pVal_mut = result.mut_val;
	} else {
		pJSONValue->m_pDocument = handle->m_pDocument;
		pJSONValue->m_pVal = result.imm_val;
	}

	return pJSONValue.release();
}

bool JsonManager::PtrTryGetBool(JsonValue* handle, const char* path, bool* out_value)
{
	if (!handle || !path || !out_value) {
		return false;
	}

	auto result = PtrGetValueInternal(handle, path);
	if (!result.success) {
		return false;
	}

	if (handle->IsMutable()) {
		if (!yyjson_mut_is_bool(result.mut_val)) {
			return false;
		}
		*out_value = yyjson_mut_get_bool(result.mut_val);
		return true;
	} else {
		if (!yyjson_is_bool(result.imm_val)) {
			return false;
		}
		*out_value = yyjson_get_bool(result.imm_val);
		return true;
	}
}

bool JsonManager::PtrTryGetFloat(JsonValue* handle, const char* path, double* out_value)
{
	if (!handle || !path || !out_value) {
		return false;
	}

	auto result = PtrGetValueInternal(handle, path);
	if (!result.success) {
		return false;
	}

	if (handle->IsMutable()) {
		if (!yyjson_mut_is_num(result.mut_val)) {
			return false;
		}
		*out_value = yyjson_mut_get_num(result.mut_val);
		return true;
	} else {
		if (!yyjson_is_num(result.imm_val)) {
			return false;
		}
		*out_value = yyjson_get_num(result.imm_val);
		return true;
	}
}

bool JsonManager::PtrTryGetInt(JsonValue* handle, const char* path, int* out_value)
{
	if (!handle || !path || !out_value) {
		return false;
	}

	auto result = PtrGetValueInternal(handle, path);
	if (!result.success) {
		return false;
	}

	if (handle->IsMutable()) {
		if (!yyjson_mut_is_int(result.mut_val)) {
			return false;
		}
		*out_value = yyjson_mut_get_int(result.mut_val);
		return true;
	} else {
		if (!yyjson_is_int(result.imm_val)) {
			return false;
		}
		*out_value = yyjson_get_int(result.imm_val);
		return true;
	}
}

bool JsonManager::PtrTryGetInt64(JsonValue* handle, const char* path, std::variant<int64_t, uint64_t>* out_value)
{
	if (!handle || !path || !out_value) {
		return false;
	}

	auto result = PtrGetValueInternal(handle, path);
	if (!result.success) {
		return false;
	}

	if (handle->IsMutable()) {
		if (!yyjson_mut_is_int(result.mut_val)) {
			return false;
		}
		ReadInt64FromMutVal(result.mut_val, out_value);
		return true;
	} else {
		if (!yyjson_is_int(result.imm_val)) {
			return false;
		}
		ReadInt64FromVal(result.imm_val, out_value);
		return true;
	}
}

bool JsonManager::PtrTryGetString(JsonValue* handle, const char* path, const char** out_str, size_t* out_len)
{
	if (!handle || !path || !out_str) {
			return false;
		}

	auto result = PtrGetValueInternal(handle, path);
	if (!result.success) {
		return false;
	}

	if (handle->IsMutable()) {
		if (!yyjson_mut_is_str(result.mut_val)) {
			return false;
		}
		*out_str = yyjson_mut_get_str(result.mut_val);
		if (out_len) {
			*out_len = yyjson_mut_get_len(result.mut_val);
		}
		return true;
	} else {
		if (!yyjson_is_str(result.imm_val)) {
			return false;
		}
		*out_str = yyjson_get_str(result.imm_val);
		if (out_len) {
			*out_len = yyjson_get_len(result.imm_val);
		}
		return true;
	}
}

bool JsonManager::ObjectForeachNext(JsonValue* handle, const char** out_key,
                                       size_t* out_key_len, JsonValue** out_value)
{
	if (!handle || !IsObject(handle)) {
		return false;
	}

	if (IsMutable(handle)) {
		if (!handle->m_iterInitialized) {
			if (!yyjson_mut_obj_iter_init(handle->m_pVal_mut, &handle->m_iterObj)) {
				return false;
			}
			handle->m_iterInitialized = true;
		}

		yyjson_mut_val* key = yyjson_mut_obj_iter_next(&handle->m_iterObj);
		if (key) {
			yyjson_mut_val* val = yyjson_mut_obj_iter_get_val(key);

			*out_key = yyjson_mut_get_str(key);
			if (out_key_len) {
				*out_key_len = yyjson_mut_get_len(key);
			}

			auto pWrapper = CreateWrapper();
			pWrapper->m_pDocument_mut = handle->m_pDocument_mut;
			pWrapper->m_pVal_mut = val;
			*out_value = pWrapper.release();

			return true;
		}
	} else {
		if (!handle->m_iterInitialized) {
			if (!yyjson_obj_iter_init(handle->m_pVal, &handle->m_iterObjImm)) {
				return false;
			}
			handle->m_iterInitialized = true;
		}

		yyjson_val* key = yyjson_obj_iter_next(&handle->m_iterObjImm);
		if (key) {
			yyjson_val* val = yyjson_obj_iter_get_val(key);

			*out_key = yyjson_get_str(key);
			if (out_key_len) {
				*out_key_len = yyjson_get_len(key);
			}

			auto pWrapper = CreateWrapper();
			pWrapper->m_pDocument = handle->m_pDocument;
			pWrapper->m_pVal = val;
			*out_value = pWrapper.release();

			return true;
		}
	}

	handle->ResetObjectIterator();
	return false;
}

bool JsonManager::ArrayForeachNext(JsonValue* handle, size_t* out_index,
                                      JsonValue** out_value)
{
	if (!handle || !IsArray(handle)) {
		return false;
	}

	if (IsMutable(handle)) {
		if (!handle->m_iterInitialized) {
			if (!yyjson_mut_arr_iter_init(handle->m_pVal_mut, &handle->m_iterArr)) {
				return false;
			}
			handle->m_iterInitialized = true;
		}

		yyjson_mut_val* val = yyjson_mut_arr_iter_next(&handle->m_iterArr);
		if (val) {
			*out_index = handle->m_arrayIndex;

			auto pWrapper = CreateWrapper();
			pWrapper->m_pDocument_mut = handle->m_pDocument_mut;
			pWrapper->m_pVal_mut = val;
			*out_value = pWrapper.release();

			handle->m_arrayIndex++;
			return true;
		}
	} else {
		if (!handle->m_iterInitialized) {
			if (!yyjson_arr_iter_init(handle->m_pVal, &handle->m_iterArrImm)) {
				return false;
			}
			handle->m_iterInitialized = true;
		}

		yyjson_val* val = yyjson_arr_iter_next(&handle->m_iterArrImm);
		if (val) {
			*out_index = handle->m_arrayIndex;

			auto pWrapper = CreateWrapper();
			pWrapper->m_pDocument = handle->m_pDocument;
			pWrapper->m_pVal = val;
			*out_value = pWrapper.release();

			handle->m_arrayIndex++;
			return true;
		}
	}

	handle->ResetArrayIterator();
	return false;
}

bool JsonManager::ObjectForeachKeyNext(JsonValue* handle, const char** out_key,
                                          size_t* out_key_len)
{
	if (!handle || !IsObject(handle)) {
		return false;
	}

	if (IsMutable(handle)) {
		if (!handle->m_iterInitialized) {
			if (!yyjson_mut_obj_iter_init(handle->m_pVal_mut, &handle->m_iterObj)) {
				return false;
			}
			handle->m_iterInitialized = true;
		}

		yyjson_mut_val* key = yyjson_mut_obj_iter_next(&handle->m_iterObj);
		if (key) {
			*out_key = yyjson_mut_get_str(key);
			if (out_key_len) {
				*out_key_len = yyjson_mut_get_len(key);
			}
			return true;
		}
	} else {
		if (!handle->m_iterInitialized) {
			if (!yyjson_obj_iter_init(handle->m_pVal, &handle->m_iterObjImm)) {
				return false;
			}
			handle->m_iterInitialized = true;
		}

		yyjson_val* key = yyjson_obj_iter_next(&handle->m_iterObjImm);
		if (key) {
			*out_key = yyjson_get_str(key);
			if (out_key_len) {
				*out_key_len = yyjson_get_len(key);
			}
			return true;
		}
	}

	handle->ResetObjectIterator();
	return false;
}

bool JsonManager::ArrayForeachIndexNext(JsonValue* handle, size_t* out_index)
{
	if (!handle || !IsArray(handle)) {
		return false;
	}

	if (IsMutable(handle)) {
		if (!handle->m_iterInitialized) {
			if (!yyjson_mut_arr_iter_init(handle->m_pVal_mut, &handle->m_iterArr)) {
				return false;
			}
			handle->m_iterInitialized = true;
		}

		yyjson_mut_val* val = yyjson_mut_arr_iter_next(&handle->m_iterArr);
		if (val) {
			*out_index = handle->m_arrayIndex;
			handle->m_arrayIndex++;
			return true;
		}
	} else {
		if (!handle->m_iterInitialized) {
			if (!yyjson_arr_iter_init(handle->m_pVal, &handle->m_iterArrImm)) {
				return false;
			}
			handle->m_iterInitialized = true;
		}

		yyjson_val* val = yyjson_arr_iter_next(&handle->m_iterArrImm);
		if (val) {
			*out_index = handle->m_arrayIndex;
			handle->m_arrayIndex++;
			return true;
		}
	}

	handle->ResetArrayIterator();
	return false;
}

void JsonManager::Release(JsonValue* value)
{
	if (value) {
		delete value;
	}
}

HandleType_t JsonManager::GetHandleType()
{
  return g_JsonType;
}

JsonValue* JsonManager::GetFromHandle(IPluginContext* pContext, Handle_t handle)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	JsonValue* pJSONValue;
	if ((err = handlesys->ReadHandle(handle, g_JsonType, &sec, (void**)&pJSONValue)) != HandleError_None)
	{
		pContext->ReportError("Invalid JSON handle %x (error %d)", handle, err);
		return nullptr;
	}

	return pJSONValue;
}

JsonArrIter* JsonManager::ArrIterInit(JsonValue* handle)
{
	return ArrIterWith(handle);
}

JsonArrIter* JsonManager::ArrIterWith(JsonValue* handle)
{
	if (!handle || !IsArray(handle)) {
		return nullptr;
	}

	auto iter = new JsonArrIter();
	iter->m_isMutable = handle->IsMutable();
	iter->m_pDocument_mut = handle->m_pDocument_mut;
	iter->m_pDocument = handle->m_pDocument;

	if (handle->IsMutable()) {
		if (!yyjson_mut_arr_iter_init(handle->m_pVal_mut, &iter->m_iterMut)) {
			delete iter;
			return nullptr;
		}
	} else {
		if (!yyjson_arr_iter_init(handle->m_pVal, &iter->m_iterImm)) {
			delete iter;
			return nullptr;
		}
	}

	iter->m_initialized = true;
	return iter;
}

JsonValue* JsonManager::ArrIterNext(JsonArrIter* iter)
{
	if (!iter || !iter->m_initialized) {
		return nullptr;
	}

	JsonValue* val = nullptr;

	if (iter->m_isMutable) {
		yyjson_mut_val* raw_val = yyjson_mut_arr_iter_next(&iter->m_iterMut);
		if (!raw_val) {
			return nullptr;
		}

		auto pWrapper = CreateWrapper();
		pWrapper->m_pDocument_mut = iter->m_pDocument_mut;
		pWrapper->m_pVal_mut = raw_val;
		val = pWrapper.release();
	} else {
		yyjson_val* raw_val = yyjson_arr_iter_next(&iter->m_iterImm);
		if (!raw_val) {
			return nullptr;
		}

		auto pWrapper = CreateWrapper();
		pWrapper->m_pDocument = iter->m_pDocument;
		pWrapper->m_pVal = raw_val;
		val = pWrapper.release();
	}

	return val;
}

bool JsonManager::ArrIterHasNext(JsonArrIter* iter)
{
	if (!iter || !iter->m_initialized) {
		return false;
	}

	if (iter->m_isMutable) {
		return yyjson_mut_arr_iter_has_next(&iter->m_iterMut);
	} else {
		return yyjson_arr_iter_has_next(&iter->m_iterImm);
	}
}

size_t JsonManager::ArrIterGetIndex(JsonArrIter* iter)
{
	if (!iter || !iter->m_initialized) {
		return SIZE_MAX;
	}

	if (iter->m_isMutable) {
		if (iter->m_iterMut.idx == 0) {
			return SIZE_MAX;
		}
		return iter->m_iterMut.idx - 1;
	} else {
		if (iter->m_iterImm.idx == 0) {
			return SIZE_MAX;
		}
		return iter->m_iterImm.idx - 1;
	}
}

void* JsonManager::ArrIterRemove(JsonArrIter* iter)
{
	if (!iter || !iter->m_isMutable) {
		return nullptr;
	}

	return yyjson_mut_arr_iter_remove(&iter->m_iterMut);
}

JsonObjIter* JsonManager::ObjIterInit(JsonValue* handle)
{
	return ObjIterWith(handle);
}

JsonObjIter* JsonManager::ObjIterWith(JsonValue* handle)
{
	if (!handle || !IsObject(handle)) {
		return nullptr;
	}

	auto iter = new JsonObjIter();
	iter->m_isMutable = handle->IsMutable();
	iter->m_pDocument_mut = handle->m_pDocument_mut;
	iter->m_pDocument = handle->m_pDocument;

	if (handle->IsMutable()) {
		if (!yyjson_mut_obj_iter_init(handle->m_pVal_mut, &iter->m_iterMut)) {
			delete iter;
			return nullptr;
		}
	} else {
		if (!yyjson_obj_iter_init(handle->m_pVal, &iter->m_iterImm)) {
			delete iter;
			return nullptr;
		}
	}

	iter->m_initialized = true;
	return iter;
}

void* JsonManager::ObjIterNext(JsonObjIter* iter)
{
	if (!iter || !iter->m_initialized) {
		return nullptr;
	}

	if (iter->m_isMutable) {
		yyjson_mut_val* current_key = yyjson_mut_obj_iter_next(&iter->m_iterMut);
		if (!current_key) {
			return nullptr;
		}
		iter->m_currentKey = current_key;
		return current_key;
	} else {
		void* key = yyjson_obj_iter_next(&iter->m_iterImm);
		iter->m_currentKey = key;
		return key;
	}
}

bool JsonManager::ObjIterHasNext(JsonObjIter* iter)
{
	if (!iter || !iter->m_initialized) {
		return false;
	}

	if (iter->m_isMutable) {
		return yyjson_mut_obj_iter_has_next(&iter->m_iterMut);
	} else {
		return yyjson_obj_iter_has_next(&iter->m_iterImm);
	}
}

JsonValue* JsonManager::ObjIterGetVal(JsonObjIter* iter, void* key)
{
	if (!iter || !iter->m_initialized || !key) {
		return nullptr;
	}

	auto pWrapper = CreateWrapper();

	if (iter->m_isMutable) {
		yyjson_mut_val* val = yyjson_mut_obj_iter_get_val(reinterpret_cast<yyjson_mut_val*>(key));
		if (!val) {
			return nullptr;
		}
		pWrapper->m_pDocument_mut = iter->m_pDocument_mut;
		pWrapper->m_pVal_mut = val;
	} else {
		yyjson_val* val = yyjson_obj_iter_get_val(reinterpret_cast<yyjson_val*>(key));
		if (!val) {
			return nullptr;
		}
		pWrapper->m_pDocument = iter->m_pDocument;
		pWrapper->m_pVal = val;
	}

	return pWrapper.release();
}

JsonValue* JsonManager::ObjIterGet(JsonObjIter* iter, const char* key)
{
	if (!iter || !iter->m_initialized || !key) {
		return nullptr;
	}

	auto pWrapper = CreateWrapper();

	if (iter->m_isMutable) {
		yyjson_mut_val* val = yyjson_mut_obj_iter_get(&iter->m_iterMut, key);
		if (!val) {
			return nullptr;
		}
		pWrapper->m_pDocument_mut = iter->m_pDocument_mut;
		pWrapper->m_pVal_mut = val;
	} else {
		yyjson_val* val = yyjson_obj_iter_get(&iter->m_iterImm, key);
		if (!val) {
			return nullptr;
		}
		pWrapper->m_pDocument = iter->m_pDocument;
		pWrapper->m_pVal = val;
	}

	return pWrapper.release();
}

size_t JsonManager::ObjIterGetIndex(JsonObjIter* iter)
{
	if (!iter || !iter->m_initialized) {
		return SIZE_MAX;
	}
	if (iter->m_isMutable) {
		if (iter->m_currentKey == nullptr) {
			return SIZE_MAX;
		}
		if (iter->m_iterMut.idx >= iter->m_iterMut.max) {
			return iter->m_iterMut.max - 1;
		}
		return iter->m_iterMut.idx - 1;
	} else {
		if (iter->m_iterImm.idx == 0) {
			return SIZE_MAX;
		}
		if (iter->m_iterImm.idx >= iter->m_iterImm.max) {
			return iter->m_iterImm.max - 1;
		}
		return iter->m_iterImm.idx - 1;
	}
}

void* JsonManager::ObjIterRemove(JsonObjIter* iter)
{
	if (!iter || !iter->m_isMutable) {
		return nullptr;
	}

	return yyjson_mut_obj_iter_remove(&iter->m_iterMut);
}

void JsonManager::ReleaseArrIter(JsonArrIter* iter)
{
	if (iter) {
		delete iter;
	}
}

void JsonManager::ReleaseObjIter(JsonObjIter* iter)
{
	if (iter) {
		delete iter;
	}
}

HandleType_t JsonManager::GetArrIterHandleType()
{
	return g_ArrIterType;
}

HandleType_t JsonManager::GetObjIterHandleType()
{
	return g_ObjIterType;
}

JsonArrIter* JsonManager::GetArrIterFromHandle(IPluginContext* pContext, Handle_t handle)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	JsonArrIter* pIter;
	if ((err = handlesys->ReadHandle(handle, g_ArrIterType, &sec, (void**)&pIter)) != HandleError_None)
	{
		pContext->ReportError("Invalid JSONArrIter handle %x (error %d)", handle, err);
		return nullptr;
	}

	return pIter;
}

JsonObjIter* JsonManager::GetObjIterFromHandle(IPluginContext* pContext, Handle_t handle)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	JsonObjIter* pIter;
	if ((err = handlesys->ReadHandle(handle, g_ObjIterType, &sec, (void**)&pIter)) != HandleError_None)
	{
		pContext->ReportError("Invalid JSONObjIter handle %x (error %d)", handle, err);
		return nullptr;
	}

	return pIter;
}

JsonValue* JsonManager::ReadNumber(const char* dat, uint32_t read_flg, char* error, size_t error_size, size_t* out_consumed)
{
	if (!dat) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid input data");
		}
		return nullptr;
	}

	auto pJSONValue = CreateWrapper();
	pJSONValue->m_pDocument_mut = CreateDocument();

	yyjson_mut_val* val = yyjson_mut_int(pJSONValue->m_pDocument_mut.get(), 0);
	if (!val) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Failed to create number value");
		}
		return nullptr;
	}

	yyjson_read_err readError;
	const char* end_ptr = yyjson_mut_read_number(dat, val,
		static_cast<yyjson_read_flag>(read_flg), nullptr, &readError);

	if (!end_ptr || readError.code) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Failed to read number: %s (error code: %u, position: %zu)",
				readError.msg, readError.code, readError.pos);
		}
		return nullptr;
	}

	if (out_consumed) {
		*out_consumed = end_ptr - dat;
	}

	pJSONValue->m_pVal_mut = val;
	yyjson_mut_doc_set_root(pJSONValue->m_pDocument_mut.get(), val);

	return pJSONValue.release();
}

bool JsonManager::WriteNumber(JsonValue* handle, char* buffer, size_t buffer_size, size_t* out_written)
{
	if (!handle || !buffer || buffer_size == 0) {
		return false;
	}

	if (!IsNum(handle)) {
		return false;
	}

	char* result = nullptr;
	if (handle->IsMutable()) {
		result = yyjson_mut_write_number(handle->m_pVal_mut, buffer);
	} else {
		result = yyjson_write_number(handle->m_pVal, buffer);
	}

	if (!result) {
		return false;
	}

	size_t written = result - buffer;
	if (written >= buffer_size) {
		return false;
	}

	if (out_written) {
		*out_written = written;
	}

	return true;
}

bool JsonManager::SetFpToFloat(JsonValue* handle, bool flt)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_set_fp_to_float(handle->m_pVal_mut, flt);
	} else {
		return yyjson_set_fp_to_float(handle->m_pVal, flt);
	}
}

bool JsonManager::SetFpToFixed(JsonValue* handle, int prec)
{
	if (!handle) {
		return false;
	}

	if (prec < 1 || prec > 15) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_set_fp_to_fixed(handle->m_pVal_mut, prec);
	} else {
		return yyjson_set_fp_to_fixed(handle->m_pVal, prec);
	}
}

bool JsonManager::SetBool(JsonValue* handle, bool value)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_set_bool(handle->m_pVal_mut, value);
	} else {
		return yyjson_set_bool(handle->m_pVal, value);
	}
}

bool JsonManager::SetInt(JsonValue* handle, int value)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_set_int(handle->m_pVal_mut, value);
	} else {
		return yyjson_set_int(handle->m_pVal, value);
	}
}

bool JsonManager::SetInt64(JsonValue* handle, std::variant<int64_t, uint64_t> value)
{
	if (!handle) {
		return false;
	}

	return std::visit([&](auto&& val) -> bool {
		using T = std::decay_t<decltype(val)>;
		if (handle->IsMutable()) {
			if constexpr (std::is_same_v<T, int64_t>) {
				return yyjson_mut_set_sint(handle->m_pVal_mut, val);
			} else if constexpr (std::is_same_v<T, uint64_t>) {
				return yyjson_mut_set_uint(handle->m_pVal_mut, val);
			}
		} else {
			if constexpr (std::is_same_v<T, int64_t>) {
				return yyjson_set_sint(handle->m_pVal, val);
			} else if constexpr (std::is_same_v<T, uint64_t>) {
				return yyjson_set_uint(handle->m_pVal, val);
			}
		}
		return false;
	}, value);
}

bool JsonManager::SetFloat(JsonValue* handle, double value)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_set_real(handle->m_pVal_mut, value);
	} else {
		return yyjson_set_real(handle->m_pVal, value);
	}
}

bool JsonManager::SetString(JsonValue* handle, const char* value)
{
	if (!handle || !value) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_set_str(handle->m_pVal_mut, value);
	} else {
		return yyjson_set_str(handle->m_pVal, value);
	}
}

bool JsonManager::SetNull(JsonValue* handle)
{
	if (!handle) {
		return false;
	}

	if (handle->IsMutable()) {
		return yyjson_mut_set_null(handle->m_pVal_mut);
	} else {
		return yyjson_set_null(handle->m_pVal);
	}
}

bool JsonManager::ParseInt64Variant(const char* value, std::variant<int64_t, uint64_t>* out_value, char* error, size_t error_size)
{
	if (!value || !*value) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Empty integer64 value");
		}
		return false;
	}

	if (!out_value) {
		if (error && error_size > 0) {
			SetErrorSafe(error, error_size, "Invalid output parameter");
		}
		return false;
	}

	std::string_view sv(value);
	bool is_negative = (sv[0] == '-');

	if (is_negative) {
		int64_t signed_val;
		auto result = std::from_chars(sv.data(), sv.data() + sv.size(), signed_val);

		if (result.ec == std::errc{} && result.ptr == sv.data() + sv.size()) {
			*out_value = signed_val;
			return true;
		}

		if (error && error_size > 0) {
			if (result.ec == std::errc::result_out_of_range) {
				SetErrorSafe(error, error_size, "Integer64 value out of range: %s", value);
			} else {
				SetErrorSafe(error, error_size, "Invalid integer64 value: %s", value);
			}
		}
		return false;
	}

	int64_t signed_val;
	auto result = std::from_chars(sv.data(), sv.data() + sv.size(), signed_val);

	if (result.ec == std::errc{} && result.ptr == sv.data() + sv.size()) {
		*out_value = signed_val;
		return true;
	}

	if (result.ec == std::errc::result_out_of_range) {
		uint64_t unsigned_val;
		auto unsigned_result = std::from_chars(sv.data(), sv.data() + sv.size(), unsigned_val);

		if (unsigned_result.ec == std::errc{} && unsigned_result.ptr == sv.data() + sv.size()) {
			*out_value = unsigned_val;
			return true;
		}

		if (error && error_size > 0) {
			if (unsigned_result.ec == std::errc::result_out_of_range) {
				SetErrorSafe(error, error_size, "Integer64 value out of range: %s", value);
			} else {
				SetErrorSafe(error, error_size, "Invalid integer64 value: %s", value);
			}
		}
		return false;
	}

	if (error && error_size > 0) {
		SetErrorSafe(error, error_size, "Invalid integer64 value: %s", value);
	}
	return false;
}