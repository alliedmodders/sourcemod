#include "YYJSONManager.h"
#include "extension.h"

std::unique_ptr<YYJSONValue> YYJSONManager::CreateWrapper() {
	return std::make_unique<YYJSONValue>();
}

std::shared_ptr<yyjson_mut_doc> YYJSONManager::WrapDocument(yyjson_mut_doc* doc) {
	return std::shared_ptr<yyjson_mut_doc>(doc, [](yyjson_mut_doc*){});
}

std::shared_ptr<yyjson_mut_doc> YYJSONManager::CopyDocument(yyjson_doc* doc) {
	return WrapDocument(yyjson_doc_mut_copy(doc, nullptr));
}

std::shared_ptr<yyjson_mut_doc> YYJSONManager::CreateDocument() {
	return WrapDocument(yyjson_mut_doc_new(nullptr));
}

std::shared_ptr<yyjson_doc> YYJSONManager::WrapImmutableDocument(yyjson_doc* doc) {
	return std::shared_ptr<yyjson_doc>(doc, [](yyjson_doc*){});
}

YYJSONManager::YYJSONManager(): m_randomGenerator(m_randomDevice()) {}

YYJSONManager::~YYJSONManager() {}

YYJSONValue* YYJSONManager::ParseJSON(const char* json_str, bool is_file, bool is_mutable, 
	yyjson_read_flag read_flg, char* error, size_t error_size)
{
	if (!json_str) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid JSON string");
		}
		return nullptr;
	}

	yyjson_read_err readError;
	yyjson_doc* idoc;
	auto pYYJSONValue = CreateWrapper();

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
				snprintf(error, error_size, "Failed to parse JSON file: %s (error code: %u, msg: %s, position: %zu)",
					json_str, readError.code, readError.msg, readError.pos);
			} else {
				snprintf(error, error_size, "Failed to parse JSON str: %s (error code: %u, position: %zu)",
					readError.msg, readError.code, readError.pos);
			}
		}
		if (idoc) {
			yyjson_doc_free(idoc);
		}
		return nullptr;
	}

	pYYJSONValue->m_readSize = yyjson_doc_get_read_size(idoc);

	if (is_mutable) {
		pYYJSONValue->m_pDocument_mut = CopyDocument(idoc);
		pYYJSONValue->m_pVal_mut = yyjson_mut_doc_get_root(pYYJSONValue->m_pDocument_mut.get());
		yyjson_doc_free(idoc);
	} else {
		pYYJSONValue->m_pDocument = WrapImmutableDocument(idoc);
		pYYJSONValue->m_pVal = yyjson_doc_get_root(idoc);
	}

	return pYYJSONValue.release();
}

bool YYJSONManager::WriteToString(YYJSONValue* handle, char* buffer, size_t buffer_size, 
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

bool YYJSONManager::WriteToFile(YYJSONValue* handle, const char* path, yyjson_write_flag write_flg,
	char* error, size_t error_size)
{
	if (!handle || !path) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters");
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
		snprintf(error, error_size, "Failed to write JSON to file: %s (error code: %u)", writeError.msg, writeError.code);
	}

	return is_success;
}

bool YYJSONManager::Equals(YYJSONValue* handle1, YYJSONValue* handle2)
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

	YYJSONValue* immutable = handle1->IsMutable() ? handle2 : handle1;
	YYJSONValue* mutable_doc = handle1->IsMutable() ? handle1 : handle2;

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

YYJSONValue* YYJSONManager::DeepCopy(YYJSONValue* targetDoc, YYJSONValue* sourceValue)
{
	if (!targetDoc || !sourceValue) {
		return nullptr;
	}

	auto pYYJSONValue = CreateWrapper();

	if (targetDoc->IsMutable()) {
		pYYJSONValue->m_pDocument_mut = CreateDocument();

		yyjson_mut_val* val_copy = nullptr;
		if (sourceValue->IsMutable()) {
			val_copy = yyjson_mut_val_mut_copy(pYYJSONValue->m_pDocument_mut.get(), sourceValue->m_pVal_mut);
		} else {
			val_copy = yyjson_val_mut_copy(pYYJSONValue->m_pDocument_mut.get(), sourceValue->m_pVal);
		}

		if (!val_copy) {
			return nullptr;
		}

		yyjson_mut_doc_set_root(pYYJSONValue->m_pDocument_mut.get(), val_copy);
		pYYJSONValue->m_pVal_mut = val_copy;
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

		pYYJSONValue->m_pDocument = WrapImmutableDocument(doc);
		pYYJSONValue->m_pVal = yyjson_doc_get_root(doc);
	}

	return pYYJSONValue.release();
}

const char* YYJSONManager::GetTypeDesc(YYJSONValue* handle)
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

size_t YYJSONManager::GetSerializedSize(YYJSONValue* handle, yyjson_write_flag write_flg)
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

YYJSONValue* YYJSONManager::ToMutable(YYJSONValue* handle)
{
	if (!handle || handle->IsMutable()) {
		return nullptr;
	}

	auto pYYJSONValue = CreateWrapper();
	pYYJSONValue->m_pDocument_mut = CopyDocument(handle->m_pDocument.get());
	pYYJSONValue->m_pVal_mut = yyjson_mut_doc_get_root(pYYJSONValue->m_pDocument_mut.get());

	return pYYJSONValue.release();
}

YYJSONValue* YYJSONManager::ToImmutable(YYJSONValue* handle)
{
	if (!handle || !handle->IsMutable()) {
		return nullptr;
	}

	auto pYYJSONValue = CreateWrapper();
	yyjson_doc* mdoc = yyjson_mut_doc_imut_copy(handle->m_pDocument_mut.get(), nullptr);
	pYYJSONValue->m_pDocument = WrapImmutableDocument(mdoc);
	pYYJSONValue->m_pVal = yyjson_doc_get_root(pYYJSONValue->m_pDocument.get());

	return pYYJSONValue.release();
}

yyjson_type YYJSONManager::GetType(YYJSONValue* handle)
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

yyjson_subtype YYJSONManager::GetSubtype(YYJSONValue* handle)
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

bool YYJSONManager::IsArray(YYJSONValue* handle)
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

bool YYJSONManager::IsObject(YYJSONValue* handle)
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

bool YYJSONManager::IsInt(YYJSONValue* handle)
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

bool YYJSONManager::IsUint(YYJSONValue* handle)
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

bool YYJSONManager::IsSint(YYJSONValue* handle)
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

bool YYJSONManager::IsNum(YYJSONValue* handle)
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

bool YYJSONManager::IsBool(YYJSONValue* handle)
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

bool YYJSONManager::IsTrue(YYJSONValue* handle)
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

bool YYJSONManager::IsFalse(YYJSONValue* handle)
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

bool YYJSONManager::IsFloat(YYJSONValue* handle)
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

bool YYJSONManager::IsStr(YYJSONValue* handle)
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

bool YYJSONManager::IsNull(YYJSONValue* handle)
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

bool YYJSONManager::IsCtn(YYJSONValue* handle)
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

bool YYJSONManager::IsMutable(YYJSONValue* handle)
{
	if (!handle) {
		return false;
	}

	return handle->IsMutable();
}

bool YYJSONManager::IsImmutable(YYJSONValue* handle)
{
	if (!handle) {
		return false;
	}

	return handle->IsImmutable();
}

size_t YYJSONManager::GetReadSize(YYJSONValue* handle)
{
	if (!handle) {
		return 0;
	}

	// this not happen in normal case, but it's possible if the document is not from parsing.
	if (handle->m_readSize == 0) {
		return 0;
	}

	return handle->m_readSize + 1;
}

YYJSONValue* YYJSONManager::ObjectInit()
{
	auto pYYJSONValue = CreateWrapper();
	pYYJSONValue->m_pDocument_mut = CreateDocument();
	pYYJSONValue->m_pVal_mut = yyjson_mut_obj(pYYJSONValue->m_pDocument_mut.get());
	yyjson_mut_doc_set_root(pYYJSONValue->m_pDocument_mut.get(), pYYJSONValue->m_pVal_mut);

	return pYYJSONValue.release();
}

YYJSONValue* YYJSONManager::ObjectInitWithStrings(const char** pairs, size_t count)
{
	if (!pairs || count == 0) {
		return nullptr;
	}

	auto pYYJSONValue = CreateWrapper();
	pYYJSONValue->m_pDocument_mut = CreateDocument();

	pYYJSONValue->m_pVal_mut = yyjson_mut_obj_with_kv(
		pYYJSONValue->m_pDocument_mut.get(),
		pairs,
		count
	);

	if (!pYYJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pYYJSONValue.release();
}

YYJSONValue* YYJSONManager::ObjectParseString(const char* str, yyjson_read_flag read_flg,
	char* error, size_t error_size)
{
	if (!str) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid string");
		}
		return nullptr;
	}

	auto pYYJSONValue = CreateWrapper();

	yyjson_read_err readError;
	yyjson_doc* idoc = yyjson_read_opts(const_cast<char*>(str), strlen(str), read_flg, nullptr, &readError);

	if (!idoc || readError.code) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Failed to parse JSON str: %s (error code: %u, position: %zu)",
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
			snprintf(error, error_size, "Root value is not an object (got %s)", yyjson_get_type_desc(root));
		}
		yyjson_doc_free(idoc);
		return nullptr;
	}

	pYYJSONValue->m_readSize = yyjson_doc_get_read_size(idoc);
	pYYJSONValue->m_pDocument = WrapImmutableDocument(idoc);
	pYYJSONValue->m_pVal = root;

	return pYYJSONValue.release();
}

YYJSONValue* YYJSONManager::ObjectParseFile(const char* path, yyjson_read_flag read_flg,
	char* error, size_t error_size)
{
	if (!path) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid path");
		}
		return nullptr;
	}

	char realpath[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);
	auto pYYJSONValue = CreateWrapper();

	yyjson_read_err readError;
	yyjson_doc* idoc = yyjson_read_file(realpath, read_flg, nullptr, &readError);

	if (!idoc || readError.code) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Failed to parse JSON file: %s (error code: %u, msg: %s, position: %zu)",
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
			snprintf(error, error_size, "Root value in file is not an object (got %s)", yyjson_get_type_desc(root));
		}
		yyjson_doc_free(idoc);
		return nullptr;
	}

	pYYJSONValue->m_readSize = yyjson_doc_get_read_size(idoc);
	pYYJSONValue->m_pDocument = WrapImmutableDocument(idoc);
	pYYJSONValue->m_pVal = root;

	return pYYJSONValue.release();
}

size_t YYJSONManager::ObjectGetSize(YYJSONValue* handle)
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

bool YYJSONManager::ObjectGetKey(YYJSONValue* handle, size_t index, const char** out_key)
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

YYJSONValue* YYJSONManager::ObjectGetValueAt(YYJSONValue* handle, size_t index)
{
	if (!handle) {
		return nullptr;
	}

	auto pYYJSONValue = CreateWrapper();

	if (handle->IsMutable()) {
		size_t obj_size = yyjson_mut_obj_size(handle->m_pVal_mut);
		if (index >= obj_size) {
			return nullptr;
		}

		yyjson_mut_obj_iter iter;
		yyjson_mut_obj_iter_init(handle->m_pVal_mut, &iter);

		for (size_t i = 0; i < index; i++) {
			yyjson_mut_obj_iter_next(&iter);
		}

		yyjson_mut_val* key = yyjson_mut_obj_iter_next(&iter);
		if (!key) {
			return nullptr;
		}

		pYYJSONValue->m_pDocument_mut = handle->m_pDocument_mut;
		pYYJSONValue->m_pVal_mut = yyjson_mut_obj_iter_get_val(key);
	} else {
		size_t obj_size = yyjson_obj_size(handle->m_pVal);
		if (index >= obj_size) {
			return nullptr;
		}

		yyjson_obj_iter iter;
		yyjson_obj_iter_init(handle->m_pVal, &iter);

		for (size_t i = 0; i < index; i++) {
			yyjson_obj_iter_next(&iter);
		}

		yyjson_val* key = yyjson_obj_iter_next(&iter);
		if (!key) {
			return nullptr;
		}

		pYYJSONValue->m_pDocument = handle->m_pDocument;
		pYYJSONValue->m_pVal = yyjson_obj_iter_get_val(key);
	}

	return pYYJSONValue.release();
}

YYJSONValue* YYJSONManager::ObjectGet(YYJSONValue* handle, const char* key)
{
	if (!handle || !key) {
		return nullptr;
	}

	auto pYYJSONValue = CreateWrapper();

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_obj_get(handle->m_pVal_mut, key);
		if (!val) {
			return nullptr;
		}

		pYYJSONValue->m_pDocument_mut = handle->m_pDocument_mut;
		pYYJSONValue->m_pVal_mut = val;
	} else {
		yyjson_val* val = yyjson_obj_get(handle->m_pVal, key);
		if (!val) {
			return nullptr;
		}

		pYYJSONValue->m_pDocument = handle->m_pDocument;
		pYYJSONValue->m_pVal = val;
	}

	return pYYJSONValue.release();
}

bool YYJSONManager::ObjectGetBool(YYJSONValue* handle, const char* key, bool* out_value)
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

bool YYJSONManager::ObjectGetFloat(YYJSONValue* handle, const char* key, double* out_value)
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

bool YYJSONManager::ObjectGetInt(YYJSONValue* handle, const char* key, int* out_value)
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

bool YYJSONManager::ObjectGetInt64(YYJSONValue* handle, const char* key, int64_t* out_value)
{
	if (!handle || !key || !out_value) {
		return false;
	}

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_obj_get(handle->m_pVal_mut, key);
		if (!val || !yyjson_mut_is_int(val)) {
			return false;
		}

		*out_value = yyjson_mut_get_sint(val);
		return true;
	} else {
		yyjson_val* val = yyjson_obj_get(handle->m_pVal, key);
		if (!val || !yyjson_is_int(val)) {
			return false;
		}

		*out_value = yyjson_get_sint(val);
		return true;
	}
}

bool YYJSONManager::ObjectGetString(YYJSONValue* handle, const char* key, const char** out_str, size_t* out_len)
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

bool YYJSONManager::ObjectIsNull(YYJSONValue* handle, const char* key, bool* out_is_null)
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

bool YYJSONManager::ObjectHasKey(YYJSONValue* handle, const char* key, bool use_pointer)
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

bool YYJSONManager::ObjectRenameKey(YYJSONValue* handle, const char* old_key, const char* new_key, bool allow_duplicate)
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

bool YYJSONManager::ObjectSet(YYJSONValue* handle, const char* key, YYJSONValue* value)
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

bool YYJSONManager::ObjectSetBool(YYJSONValue* handle, const char* key, bool value)
{
	if (!handle || !handle->IsMutable() || !key) {
		return false;
	}

	return yyjson_mut_obj_put(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), key), yyjson_mut_bool(handle->m_pDocument_mut.get(), value));
}

bool YYJSONManager::ObjectSetFloat(YYJSONValue* handle, const char* key, double value)
{
	if (!handle || !handle->IsMutable() || !key) {
		return false;
	}

	return yyjson_mut_obj_put(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), key), yyjson_mut_real(handle->m_pDocument_mut.get(), value));
}

bool YYJSONManager::ObjectSetInt(YYJSONValue* handle, const char* key, int value)
{
	if (!handle || !handle->IsMutable() || !key) {
		return false;
	}

	return yyjson_mut_obj_put(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), key), yyjson_mut_int(handle->m_pDocument_mut.get(), value));
}

bool YYJSONManager::ObjectSetInt64(YYJSONValue* handle, const char* key, int64_t value)
{
	if (!handle || !handle->IsMutable() || !key) {
		return false;
	}

	return yyjson_mut_obj_put(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), key), yyjson_mut_sint(handle->m_pDocument_mut.get(), value));
}

bool YYJSONManager::ObjectSetNull(YYJSONValue* handle, const char* key)
{
	if (!handle || !handle->IsMutable() || !key) {
		return false;
	}

	return yyjson_mut_obj_put(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), key), yyjson_mut_null(handle->m_pDocument_mut.get()));
}

bool YYJSONManager::ObjectSetString(YYJSONValue* handle, const char* key, const char* value)
{
	if (!handle || !handle->IsMutable() || !key || !value) {
		return false;
	}

	return yyjson_mut_obj_put(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), key), yyjson_mut_strcpy(handle->m_pDocument_mut.get(), value));
}

bool YYJSONManager::ObjectRemove(YYJSONValue* handle, const char* key)
{
	if (!handle || !handle->IsMutable() || !key) {
		return false;
	}

	return yyjson_mut_obj_remove_key(handle->m_pVal_mut, key) != nullptr;
}

bool YYJSONManager::ObjectClear(YYJSONValue* handle)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_obj_clear(handle->m_pVal_mut);
}

bool YYJSONManager::ObjectSort(YYJSONValue* handle, YYJSON_SORT_ORDER sort_mode)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	if (!yyjson_mut_is_obj(handle->m_pVal_mut)) {
		return false;
	}

	if (sort_mode < YYJSON_SORT_ASC || sort_mode > YYJSON_SORT_RANDOM) {
		return false;
	}

	size_t obj_size = yyjson_mut_obj_size(handle->m_pVal_mut);
	if (obj_size <= 1) return true;

	static std::vector<std::pair<yyjson_mut_val*, yyjson_mut_val*>> pairs;
	pairs.clear();
	pairs.reserve(obj_size);

	size_t idx, max;
	yyjson_mut_val *key, *val;
	yyjson_mut_obj_foreach(handle->m_pVal_mut, idx, max, key, val) {
		pairs.emplace_back(key, val);
	}

	if (sort_mode == YYJSON_SORT_RANDOM) {
		std::shuffle(pairs.begin(), pairs.end(), m_randomGenerator);
	}
	else {
		auto compare = [sort_mode](const auto& a, const auto& b) {
			const char* key_a = yyjson_mut_get_str(a.first);
			const char* key_b = yyjson_mut_get_str(b.first);
			int cmp = strcmp(key_a, key_b);
			return sort_mode == YYJSON_SORT_ASC ? cmp < 0 : cmp > 0;
		};

		std::sort(pairs.begin(), pairs.end(), compare);
	}

	yyjson_mut_obj_clear(handle->m_pVal_mut);
	for (const auto& pair : pairs) {
		yyjson_mut_obj_add(handle->m_pVal_mut, pair.first, pair.second);
	}

	return true;
}

YYJSONValue* YYJSONManager::ArrayInit()
{
	auto pYYJSONValue = CreateWrapper();
	pYYJSONValue->m_pDocument_mut = CreateDocument();
	pYYJSONValue->m_pVal_mut = yyjson_mut_arr(pYYJSONValue->m_pDocument_mut.get());
	yyjson_mut_doc_set_root(pYYJSONValue->m_pDocument_mut.get(), pYYJSONValue->m_pVal_mut);

	return pYYJSONValue.release();
}

YYJSONValue* YYJSONManager::ArrayInitWithStrings(const char** strings, size_t count)
{
	if (!strings) {
		return nullptr;
	}

	auto pYYJSONValue = CreateWrapper();
	pYYJSONValue->m_pDocument_mut = CreateDocument();

	pYYJSONValue->m_pVal_mut = yyjson_mut_arr_with_strcpy(
		pYYJSONValue->m_pDocument_mut.get(),
		strings,
		count
	);

	if (!pYYJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pYYJSONValue.release();
}

YYJSONValue* YYJSONManager::ArrayParseString(const char* str, yyjson_read_flag read_flg,
	char* error, size_t error_size)
{
	if (!str) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid string");
		}
		return nullptr;
	}

	auto pYYJSONValue = CreateWrapper();

	yyjson_read_err readError;
	yyjson_doc* idoc = yyjson_read_opts(const_cast<char*>(str), strlen(str), read_flg, nullptr, &readError);

	if (!idoc || readError.code) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Failed to parse JSON string: %s (error code: %u, position: %zu)",
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
			snprintf(error, error_size, "Root value is not an array (got %s)", yyjson_get_type_desc(root));
		}
		yyjson_doc_free(idoc);
		return nullptr;
	}

	pYYJSONValue->m_readSize = yyjson_doc_get_read_size(idoc);
	pYYJSONValue->m_pDocument = WrapImmutableDocument(idoc);
	pYYJSONValue->m_pVal = root;

	return pYYJSONValue.release();
}

YYJSONValue* YYJSONManager::ArrayParseFile(const char* path, yyjson_read_flag read_flg,
	char* error, size_t error_size)
{
	if (!path) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid path");
		}
		return nullptr;
	}

	char realpath[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);
	auto pYYJSONValue = CreateWrapper();

	yyjson_read_err readError;
	yyjson_doc* idoc = yyjson_read_file(realpath, read_flg, nullptr, &readError);

	if (!idoc || readError.code) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Failed to parse JSON file: %s (error code: %u, msg: %s, position: %zu)",
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
			snprintf(error, error_size, "Root value in file is not an array (got %s)", yyjson_get_type_desc(root));
		}
		yyjson_doc_free(idoc);
		return nullptr;
	}

	pYYJSONValue->m_readSize = yyjson_doc_get_read_size(idoc);
	pYYJSONValue->m_pDocument = WrapImmutableDocument(idoc);
	pYYJSONValue->m_pVal = root;

	return pYYJSONValue.release();
}

size_t YYJSONManager::ArrayGetSize(YYJSONValue* handle)
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

YYJSONValue* YYJSONManager::ArrayGet(YYJSONValue* handle, size_t index)
{
	if (!handle) {
		return nullptr;
	}

	auto pYYJSONValue = CreateWrapper();

	if (handle->IsMutable()) {
		size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
		if (index >= arr_size) {
			return nullptr;
		}

		yyjson_mut_val* val = yyjson_mut_arr_get(handle->m_pVal_mut, index);
		if (!val) {
			return nullptr;
		}

		pYYJSONValue->m_pDocument_mut = handle->m_pDocument_mut;
		pYYJSONValue->m_pVal_mut = val;
	} else {
		size_t arr_size = yyjson_arr_size(handle->m_pVal);
		if (index >= arr_size) {
			return nullptr;
		}

		yyjson_val* val = yyjson_arr_get(handle->m_pVal, index);
		if (!val) {
			return nullptr;
		}

		pYYJSONValue->m_pDocument = handle->m_pDocument;
		pYYJSONValue->m_pVal = val;
	}

	return pYYJSONValue.release();
}

YYJSONValue* YYJSONManager::ArrayGetFirst(YYJSONValue* handle)
{
	if (!handle) {
		return nullptr;
	}

	auto pYYJSONValue = CreateWrapper();

	if (handle->IsMutable()) {
		size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
		if (arr_size == 0) {
			return nullptr;
		}

		yyjson_mut_val* val = yyjson_mut_arr_get_first(handle->m_pVal_mut);
		if (!val) {
			return nullptr;
		}

		pYYJSONValue->m_pDocument_mut = handle->m_pDocument_mut;
		pYYJSONValue->m_pVal_mut = val;
	} else {
		size_t arr_size = yyjson_arr_size(handle->m_pVal);
		if (arr_size == 0) {
			return nullptr;
		}

		yyjson_val* val = yyjson_arr_get_first(handle->m_pVal);
		if (!val) {
			return nullptr;
		}

		pYYJSONValue->m_pDocument = handle->m_pDocument;
		pYYJSONValue->m_pVal = val;
	}

	return pYYJSONValue.release();
}

YYJSONValue* YYJSONManager::ArrayGetLast(YYJSONValue* handle)
{
	if (!handle) {
		return nullptr;
	}

	auto pYYJSONValue = CreateWrapper();

	if (handle->IsMutable()) {
		size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
		if (arr_size == 0) {
			return nullptr;
		}

		yyjson_mut_val* val = yyjson_mut_arr_get_last(handle->m_pVal_mut);
		if (!val) {
			return nullptr;
		}

		pYYJSONValue->m_pDocument_mut = handle->m_pDocument_mut;
		pYYJSONValue->m_pVal_mut = val;
	} else {
		size_t arr_size = yyjson_arr_size(handle->m_pVal);
		if (arr_size == 0) {
			return nullptr;
		}

		yyjson_val* val = yyjson_arr_get_last(handle->m_pVal);
		if (!val) {
			return nullptr;
		}

		pYYJSONValue->m_pDocument = handle->m_pDocument;
		pYYJSONValue->m_pVal = val;
	}

	return pYYJSONValue.release();
}

bool YYJSONManager::ArrayGetBool(YYJSONValue* handle, size_t index, bool* out_value)
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

bool YYJSONManager::ArrayGetFloat(YYJSONValue* handle, size_t index, double* out_value)
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

bool YYJSONManager::ArrayGetInt(YYJSONValue* handle, size_t index, int* out_value)
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

bool YYJSONManager::ArrayGetInt64(YYJSONValue* handle, size_t index, int64_t* out_value)
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

		*out_value = yyjson_mut_get_sint(val);
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

		*out_value = yyjson_get_sint(val);
		return true;
	}
}

bool YYJSONManager::ArrayGetString(YYJSONValue* handle, size_t index, const char** out_str, size_t* out_len)
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

bool YYJSONManager::ArrayIsNull(YYJSONValue* handle, size_t index)
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

bool YYJSONManager::ArrayReplace(YYJSONValue* handle, size_t index, YYJSONValue* value)
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

bool YYJSONManager::ArrayReplaceBool(YYJSONValue* handle, size_t index, bool value)
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

bool YYJSONManager::ArrayReplaceFloat(YYJSONValue* handle, size_t index, double value)
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

bool YYJSONManager::ArrayReplaceInt(YYJSONValue* handle, size_t index, int value)
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

bool YYJSONManager::ArrayReplaceInt64(YYJSONValue* handle, size_t index, int64_t value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
	if (index >= arr_size) {
		return false;
	}

	return yyjson_mut_arr_replace(handle->m_pVal_mut, index, yyjson_mut_sint(handle->m_pDocument_mut.get(), value)) != nullptr;
}

bool YYJSONManager::ArrayReplaceNull(YYJSONValue* handle, size_t index)
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

bool YYJSONManager::ArrayReplaceString(YYJSONValue* handle, size_t index, const char* value)
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

bool YYJSONManager::ArrayAppend(YYJSONValue* handle, YYJSONValue* value)
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

bool YYJSONManager::ArrayAppendBool(YYJSONValue* handle, bool value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_append(handle->m_pVal_mut, yyjson_mut_bool(handle->m_pDocument_mut.get(), value));
}

bool YYJSONManager::ArrayAppendFloat(YYJSONValue* handle, double value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_append(handle->m_pVal_mut, yyjson_mut_real(handle->m_pDocument_mut.get(), value));
}

bool YYJSONManager::ArrayAppendInt(YYJSONValue* handle, int value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_append(handle->m_pVal_mut, yyjson_mut_int(handle->m_pDocument_mut.get(), value));
}

bool YYJSONManager::ArrayAppendInt64(YYJSONValue* handle, int64_t value)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_append(handle->m_pVal_mut, yyjson_mut_sint(handle->m_pDocument_mut.get(), value));
}

bool YYJSONManager::ArrayAppendNull(YYJSONValue* handle)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_append(handle->m_pVal_mut, yyjson_mut_null(handle->m_pDocument_mut.get()));
}

bool YYJSONManager::ArrayAppendString(YYJSONValue* handle, const char* value)
{
	if (!handle || !handle->IsMutable() || !value) {
		return false;
	}

	return yyjson_mut_arr_append(handle->m_pVal_mut, yyjson_mut_strcpy(handle->m_pDocument_mut.get(), value));
}

bool YYJSONManager::ArrayRemove(YYJSONValue* handle, size_t index)
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

bool YYJSONManager::ArrayRemoveFirst(YYJSONValue* handle)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	if (yyjson_mut_arr_size(handle->m_pVal_mut) == 0) {
		return false;
	}

	return yyjson_mut_arr_remove_first(handle->m_pVal_mut) != nullptr;
}

bool YYJSONManager::ArrayRemoveLast(YYJSONValue* handle)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	if (yyjson_mut_arr_size(handle->m_pVal_mut) == 0) {
		return false;
	}

	return yyjson_mut_arr_remove_last(handle->m_pVal_mut) != nullptr;
}

bool YYJSONManager::ArrayRemoveRange(YYJSONValue* handle, size_t start_index, size_t end_index)
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

bool YYJSONManager::ArrayClear(YYJSONValue* handle)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	return yyjson_mut_arr_clear(handle->m_pVal_mut);
}

int YYJSONManager::ArrayIndexOfBool(YYJSONValue* handle, bool search_value)
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

int YYJSONManager::ArrayIndexOfString(YYJSONValue* handle, const char* search_value)
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

int YYJSONManager::ArrayIndexOfInt(YYJSONValue* handle, int search_value)
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

int YYJSONManager::ArrayIndexOfInt64(YYJSONValue* handle, int64_t search_value)
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

int YYJSONManager::ArrayIndexOfFloat(YYJSONValue* handle, double search_value)
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

bool YYJSONManager::ArraySort(YYJSONValue* handle, YYJSON_SORT_ORDER sort_mode)
{
	if (!handle || !handle->IsMutable()) {
		return false;
	}

	if (!yyjson_mut_is_arr(handle->m_pVal_mut)) {
		return false;
	}

	if (sort_mode < YYJSON_SORT_ASC || sort_mode > YYJSON_SORT_RANDOM) {
		return false;
	}

	size_t arr_size = yyjson_mut_arr_size(handle->m_pVal_mut);
	if (arr_size <= 1) return true;

	static std::vector<yyjson_mut_val*> values;
	values.clear();
	values.reserve(arr_size);

	size_t idx, max;
	yyjson_mut_val *val;
	yyjson_mut_arr_foreach(handle->m_pVal_mut, idx, max, val) {
		values.push_back(val);
	}

	if (sort_mode == YYJSON_SORT_RANDOM) {
		std::shuffle(values.begin(), values.end(), m_randomGenerator);
	}
	else {
		auto compare = [sort_mode](yyjson_mut_val* a, yyjson_mut_val* b) {
			if (a == b) return false;

			uint8_t type_a = yyjson_mut_get_type(a);
			uint8_t type_b = yyjson_mut_get_type(b);
			if (type_a != type_b) {
				return sort_mode == YYJSON_SORT_ASC ? type_a < type_b : type_a > type_b;
			}

			switch (type_a) {
			case YYJSON_TYPE_STR: {
				const char* str_a = yyjson_mut_get_str(a);
				const char* str_b = yyjson_mut_get_str(b);
				int cmp = strcmp(str_a, str_b);
				return sort_mode == YYJSON_SORT_ASC ? cmp < 0 : cmp > 0;
			}
			case YYJSON_TYPE_NUM: {
				if (yyjson_mut_is_int(a) && yyjson_mut_is_int(b)) {
					int64_t num_a = yyjson_mut_get_int(a);
					int64_t num_b = yyjson_mut_get_int(b);
					return sort_mode == YYJSON_SORT_ASC ? num_a < num_b : num_a > num_b;
				}
				double num_a = yyjson_mut_get_num(a);
				double num_b = yyjson_mut_get_num(b);
				return sort_mode == YYJSON_SORT_ASC ? num_a < num_b : num_a > num_b;
			}
			case YYJSON_TYPE_BOOL: {
				bool val_a = yyjson_mut_get_bool(a);
				bool val_b = yyjson_mut_get_bool(b);
				return sort_mode == YYJSON_SORT_ASC ? val_a < val_b : val_a > val_b;
			}
			default:
				return false;
			}
		};

		std::sort(values.begin(), values.end(), compare);
	}

	yyjson_mut_arr_clear(handle->m_pVal_mut);
	for (auto val : values) {
		yyjson_mut_arr_append(handle->m_pVal_mut, val);
	}

	return true;
}

const char* YYJSONManager::SkipSeparators(const char* ptr)
{
	while (*ptr && (isspace(*ptr) || *ptr == ':' || *ptr == ',')) {
		ptr++;
	}
	return ptr;
}

void YYJSONManager::SetPackError(char* error, size_t error_size, const char* fmt, ...)
{
	if (error && error_size > 0) {
		va_list args;
		va_start(args, fmt);
		vsnprintf(error, error_size, fmt, args);
		va_end(args);
		error[error_size - 1] = '\0';
	}
}

yyjson_mut_val* YYJSONManager::PackImpl(yyjson_mut_doc* doc, const char* format, 
                                          IPackParamProvider* provider, 
                                          char* error, size_t error_size,
                                          const char** out_end_ptr)
{
	if (!doc || !format || !*format) {
		SetPackError(error, error_size, "Invalid argument(s)");
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
		SetPackError(error, error_size, "Invalid format string: expected '{' or '['");
		return nullptr;
	}

	if (!root) {
		SetPackError(error, error_size, "Failed to create root object/array");
		return nullptr;
	}

	yyjson_mut_val* key_val = nullptr;
	yyjson_mut_val* val = nullptr;

	while (*ptr && *ptr != '}' && *ptr != ']') {
		if (is_obj) {
			if (*ptr != 's') {
				SetPackError(error, error_size, "Object key must be string, got '%c'", *ptr);
				return nullptr;
			}
		}
		switch (*ptr) {
			case 's': {
				if (is_obj) {
					const char* key;
					if (!provider->GetNextString(&key)) {
						SetPackError(error, error_size, "Invalid string key");
						return nullptr;
					}
					key_val = yyjson_mut_strcpy(doc, key);
					if (!key_val) {
						SetPackError(error, error_size, "Failed to create key");
						return nullptr;
					}

					ptr = SkipSeparators(ptr + 1);
					if (*ptr != 's' && *ptr != 'i' && *ptr != 'f' && *ptr != 'b' && 
						*ptr != 'n' && *ptr != '{' && *ptr != '[') {
						SetPackError(error, error_size, "Invalid value type after key");
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
									SetPackError(error, error_size, "Invalid string value");
									return nullptr;
								}
								val = yyjson_mut_strcpy(doc, val_str);
								ptr++;
								break;
							}
							case 'i': {
								int val_int;
								if (!provider->GetNextInt(&val_int)) {
									SetPackError(error, error_size, "Invalid integer value");
									return nullptr;
								}
								val = yyjson_mut_int(doc, val_int);
								ptr++;
								break;
							}
							case 'f': {
								float val_float;
								if (!provider->GetNextFloat(&val_float)) {
									SetPackError(error, error_size, "Invalid float value");
									return nullptr;
								}
								val = yyjson_mut_real(doc, val_float);
								ptr++;
								break;
							}
							case 'b': {
								bool val_bool;
								if (!provider->GetNextBool(&val_bool)) {
									SetPackError(error, error_size, "Invalid boolean value");
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
						SetPackError(error, error_size, "Failed to create value");
						return nullptr;
					}

					if (!yyjson_mut_obj_add(root, key_val, val)) {
						SetPackError(error, error_size, "Failed to add value to object");
						return nullptr;
					}
				} else {
					const char* val_str;
					if (!provider->GetNextString(&val_str)) {
						SetPackError(error, error_size, "Invalid string value");
						return nullptr;
					}
					if (!yyjson_mut_arr_add_strcpy(doc, root, val_str)) {
						SetPackError(error, error_size, "Failed to add string to array");
						return nullptr;
					}
					ptr++;
				}
				break;
			}
			case 'i': {
				int val_int;
				if (!provider->GetNextInt(&val_int)) {
					SetPackError(error, error_size, "Invalid integer value");
					return nullptr;
				}
				if (!yyjson_mut_arr_add_int(doc, root, val_int)) {
					SetPackError(error, error_size, "Failed to add integer to array");
					return nullptr;
				}
				ptr++;
				break;
			}
			case 'b': {
				bool val_bool;
				if (!provider->GetNextBool(&val_bool)) {
					SetPackError(error, error_size, "Invalid boolean value");
					return nullptr;
				}
				if (!yyjson_mut_arr_add_bool(doc, root, val_bool)) {
					SetPackError(error, error_size, "Failed to add boolean to array");
					return nullptr;
				}
				ptr++;
				break;
			}
			case 'n': {
				if (!yyjson_mut_arr_add_null(doc, root)) {
					SetPackError(error, error_size, "Failed to add null to array");
					return nullptr;
				}
				ptr++;
				break;
			}
			case 'f': {
				float val_float;
				if (!provider->GetNextFloat(&val_float)) {
					SetPackError(error, error_size, "Invalid float value");
					return nullptr;
				}
				if (!yyjson_mut_arr_add_real(doc, root, val_float)) {
					SetPackError(error, error_size, "Failed to add float to array");
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
					SetPackError(error, error_size, "Failed to add nested value to array");
					return nullptr;
				}
				break;
			}
			default: {
				SetPackError(error, error_size, "Invalid format character: %c", *ptr);
				return nullptr;
			}
		}
		ptr = SkipSeparators(ptr);
	}

	if (*ptr != (is_obj ? '}' : ']')) {
		SetPackError(error, error_size, "Unexpected end of format string");
		return nullptr;
	}

	if (out_end_ptr) {
		*out_end_ptr = ptr + 1;
	}

	return root;
}

YYJSONValue* YYJSONManager::Pack(const char* format, IPackParamProvider* param_provider, char* error, size_t error_size)
{
	if (!format || !param_provider) {
		SetPackError(error, error_size, "Invalid arguments");
		return nullptr;
	}

	auto pYYJSONValue = CreateWrapper();
	pYYJSONValue->m_pDocument_mut = CreateDocument();

	if (!pYYJSONValue->m_pDocument_mut) {
		SetPackError(error, error_size, "Failed to create document");
		return nullptr;
	}

	const char* end_ptr = nullptr;
	pYYJSONValue->m_pVal_mut = PackImpl(pYYJSONValue->m_pDocument_mut.get(), format, 
	                                       param_provider, error, error_size, &end_ptr);

	if (!pYYJSONValue->m_pVal_mut) {
		return nullptr;
	}

	yyjson_mut_doc_set_root(pYYJSONValue->m_pDocument_mut.get(), pYYJSONValue->m_pVal_mut);

	return pYYJSONValue.release();
}

YYJSONValue* YYJSONManager::CreateBool(bool value)
{
	auto pYYJSONValue = CreateWrapper();
	pYYJSONValue->m_pDocument_mut = CreateDocument();
	pYYJSONValue->m_pVal_mut = yyjson_mut_bool(pYYJSONValue->m_pDocument_mut.get(), value);

	if (!pYYJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pYYJSONValue.release();
}

YYJSONValue* YYJSONManager::CreateFloat(double value)
{
	auto pYYJSONValue = CreateWrapper();
	pYYJSONValue->m_pDocument_mut = CreateDocument();
	pYYJSONValue->m_pVal_mut = yyjson_mut_real(pYYJSONValue->m_pDocument_mut.get(), value);

	if (!pYYJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pYYJSONValue.release();
}

YYJSONValue* YYJSONManager::CreateInt(int value)
{
	auto pYYJSONValue = CreateWrapper();
	pYYJSONValue->m_pDocument_mut = CreateDocument();
	pYYJSONValue->m_pVal_mut = yyjson_mut_int(pYYJSONValue->m_pDocument_mut.get(), value);

	if (!pYYJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pYYJSONValue.release();
}

YYJSONValue* YYJSONManager::CreateInt64(int64_t value)
{
	auto pYYJSONValue = CreateWrapper();
	pYYJSONValue->m_pDocument_mut = CreateDocument();
	pYYJSONValue->m_pVal_mut = yyjson_mut_sint(pYYJSONValue->m_pDocument_mut.get(), value);

	if (!pYYJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pYYJSONValue.release();
}

YYJSONValue* YYJSONManager::CreateNull()
{
	auto pYYJSONValue = CreateWrapper();
	pYYJSONValue->m_pDocument_mut = CreateDocument();
	pYYJSONValue->m_pVal_mut = yyjson_mut_null(pYYJSONValue->m_pDocument_mut.get());

	if (!pYYJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pYYJSONValue.release();
}

YYJSONValue* YYJSONManager::CreateString(const char* value)
{
	if (!value) {
		return nullptr;
	}

	auto pYYJSONValue = CreateWrapper();
	pYYJSONValue->m_pDocument_mut = CreateDocument();
	pYYJSONValue->m_pVal_mut = yyjson_mut_strcpy(pYYJSONValue->m_pDocument_mut.get(), value);

	if (!pYYJSONValue->m_pVal_mut) {
		return nullptr;
	}

	return pYYJSONValue.release();
}

bool YYJSONManager::GetBool(YYJSONValue* handle, bool* out_value)
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

bool YYJSONManager::GetFloat(YYJSONValue* handle, double* out_value)
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

bool YYJSONManager::GetInt(YYJSONValue* handle, int* out_value)
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

bool YYJSONManager::GetInt64(YYJSONValue* handle, int64_t* out_value)
{
	if (!handle || !out_value) {
		return false;
	}

	if (handle->IsMutable()) {
		if (!yyjson_mut_is_int(handle->m_pVal_mut)) {
			return false;
		}
		*out_value = yyjson_mut_get_sint(handle->m_pVal_mut);
		return true;
	} else {
		if (!yyjson_is_int(handle->m_pVal)) {
			return false;
		}
		*out_value = yyjson_get_sint(handle->m_pVal);
		return true;
	}
}

bool YYJSONManager::GetString(YYJSONValue* handle, const char** out_str, size_t* out_len)
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

YYJSONValue* YYJSONManager::PtrGet(YYJSONValue* handle, const char* path, char* error, size_t error_size)
{
	if (!handle || !path) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters");
		}
		return nullptr;
	}

	auto pYYJSONValue = CreateWrapper();
	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (!val || ptrGetError.code) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return nullptr;
		}

		pYYJSONValue->m_pDocument_mut = handle->m_pDocument_mut;
		pYYJSONValue->m_pVal_mut = val;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (!val || ptrGetError.code) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return nullptr;
		}

		pYYJSONValue->m_pDocument = handle->m_pDocument;
		pYYJSONValue->m_pVal = val;
	}

	return pYYJSONValue.release();
}

bool YYJSONManager::PtrGetBool(YYJSONValue* handle, const char* path, bool* out_value, char* error, size_t error_size)
{
	if (!handle || !path || !out_value) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters");
		}
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_mut_is_bool(val)) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Type mismatch at path '%s': expected boolean value, got %s", path, yyjson_mut_get_type_desc(val));
			}
			return false;
		}

		*out_value = yyjson_mut_get_bool(val);
		return true;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_is_bool(val)) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Type mismatch at path '%s': expected boolean value, got %s", path, yyjson_get_type_desc(val));
			}
			return false;
		}

		*out_value = yyjson_get_bool(val);
		return true;
	}
}

bool YYJSONManager::PtrGetFloat(YYJSONValue* handle, const char* path, double* out_value, char* error, size_t error_size)
{
	if (!handle || !path || !out_value) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters");
		}
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_mut_is_real(val)) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Type mismatch at path '%s': expected float value, got %s", path, yyjson_mut_get_type_desc(val));
			}
			return false;
		}

		*out_value = yyjson_mut_get_real(val);
		return true;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_is_real(val)) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Type mismatch at path '%s': expected float value, got %s", path, yyjson_get_type_desc(val));
			}
			return false;
		}

		*out_value = yyjson_get_real(val);
		return true;
	}
}

bool YYJSONManager::PtrGetInt(YYJSONValue* handle, const char* path, int* out_value, char* error, size_t error_size)
{
	if (!handle || !path || !out_value) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters");
		}
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_mut_is_int(val)) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Type mismatch at path '%s': expected integer value, got %s", path, yyjson_mut_get_type_desc(val));
			}
			return false;
		}

		*out_value = yyjson_mut_get_int(val);
		return true;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_is_int(val)) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Type mismatch at path '%s': expected integer value, got %s", path, yyjson_get_type_desc(val));
			}
			return false;
		}

		*out_value = yyjson_get_int(val);
		return true;
	}
}

bool YYJSONManager::PtrGetInt64(YYJSONValue* handle, const char* path, int64_t* out_value, char* error, size_t error_size)
{
	if (!handle || !path || !out_value) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters");
		}
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_mut_is_int(val)) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Type mismatch at path '%s': expected integer64 value, got %s", path, yyjson_mut_get_type_desc(val));
			}
			return false;
		}

		*out_value = yyjson_mut_get_sint(val);
		return true;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_is_int(val)) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Type mismatch at path '%s': expected integer64 value, got %s", path, yyjson_get_type_desc(val));
			}
			return false;
		}

		*out_value = yyjson_get_sint(val);
		return true;
	}
}

bool YYJSONManager::PtrGetString(YYJSONValue* handle, const char* path, const char** out_str, size_t* out_len, char* error, size_t error_size)
{
	if (!handle || !path || !out_str) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters");
		}
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_mut_is_str(val)) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Type mismatch at path '%s': expected string value, got %s", path, yyjson_mut_get_type_desc(val));
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
				snprintf(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		if (!yyjson_is_str(val)) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Type mismatch at path '%s': expected string value, got %s", path, yyjson_get_type_desc(val));
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

bool YYJSONManager::PtrGetIsNull(YYJSONValue* handle, const char* path, bool* out_is_null, char* error, size_t error_size)
{
	if (!handle || !path || !out_is_null) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters");
		}
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
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
				snprintf(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
					ptrGetError.msg, ptrGetError.code, ptrGetError.pos, path);
			}
			return false;
		}

		*out_is_null = yyjson_is_null(val);
		return true;
	}
}

bool YYJSONManager::PtrGetLength(YYJSONValue* handle, const char* path, size_t* out_len, char* error, size_t error_size)
{
	if (!handle || !path || !out_len) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters");
		}
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (ptrGetError.code) {
			if (error && error_size > 0) {
				snprintf(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
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
				snprintf(error, error_size, "Failed to resolve JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
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

bool YYJSONManager::PtrSet(YYJSONValue* handle, const char* path, YYJSONValue* value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path || !value) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters or immutable document");
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
			snprintf(error, error_size, "Failed to copy JSON value");
		}
		return false;
	}

	yyjson_ptr_err ptrSetError;
	bool success = yyjson_mut_doc_ptr_setx(handle->m_pDocument_mut.get(), path, strlen(path), val_copy, true, nullptr, &ptrSetError);

	if (ptrSetError.code && error && error_size > 0) {
		snprintf(error, error_size, "Failed to set JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
			ptrSetError.msg, ptrSetError.code, ptrSetError.pos, path);
	}

	return success;
}

bool YYJSONManager::PtrSetBool(YYJSONValue* handle, const char* path, bool value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrSetError;
	bool success = yyjson_mut_doc_ptr_setx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_bool(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrSetError);

	if (ptrSetError.code && error && error_size > 0) {
		snprintf(error, error_size, "Failed to set JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
			ptrSetError.msg, ptrSetError.code, ptrSetError.pos, path);
	}

	return success;
}

bool YYJSONManager::PtrSetFloat(YYJSONValue* handle, const char* path, double value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrSetError;
	bool success = yyjson_mut_doc_ptr_setx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_real(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrSetError);

	if (ptrSetError.code && error && error_size > 0) {
		snprintf(error, error_size, "Failed to set JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
			ptrSetError.msg, ptrSetError.code, ptrSetError.pos, path);
	}

	return success;
}

bool YYJSONManager::PtrSetInt(YYJSONValue* handle, const char* path, int value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrSetError;
	bool success = yyjson_mut_doc_ptr_setx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_int(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrSetError);

	if (ptrSetError.code && error && error_size > 0) {
		snprintf(error, error_size, "Failed to set JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
			ptrSetError.msg, ptrSetError.code, ptrSetError.pos, path);
	}

	return success;
}

bool YYJSONManager::PtrSetInt64(YYJSONValue* handle, const char* path, int64_t value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrSetError;
	bool success = yyjson_mut_doc_ptr_setx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_sint(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrSetError);

	if (ptrSetError.code && error && error_size > 0) {
		snprintf(error, error_size, "Failed to set JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
			ptrSetError.msg, ptrSetError.code, ptrSetError.pos, path);
	}

	return success;
}

bool YYJSONManager::PtrSetString(YYJSONValue* handle, const char* path, const char* value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path || !value) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrSetError;
	bool success = yyjson_mut_doc_ptr_setx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_strcpy(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrSetError);

	if (ptrSetError.code && error && error_size > 0) {
		snprintf(error, error_size, "Failed to set JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
			ptrSetError.msg, ptrSetError.code, ptrSetError.pos, path);
	}

	return success;
}

bool YYJSONManager::PtrSetNull(YYJSONValue* handle, const char* path, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrSetError;
	bool success = yyjson_mut_doc_ptr_setx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_null(handle->m_pDocument_mut.get()), true, nullptr, &ptrSetError);

	if (ptrSetError.code && error && error_size > 0) {
		snprintf(error, error_size, "Failed to set JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
			ptrSetError.msg, ptrSetError.code, ptrSetError.pos, path);
	}

	return success;
}

bool YYJSONManager::PtrAdd(YYJSONValue* handle, const char* path, YYJSONValue* value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path || !value) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters or immutable document");
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
			snprintf(error, error_size, "Failed to copy JSON value");
		}
		return false;
	}

	yyjson_ptr_err ptrAddError;
	bool success = yyjson_mut_doc_ptr_addx(handle->m_pDocument_mut.get(), path, strlen(path), val_copy, true, nullptr, &ptrAddError);

	if (ptrAddError.code && error && error_size > 0) {
		snprintf(error, error_size, "Failed to add JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
			ptrAddError.msg, ptrAddError.code, ptrAddError.pos, path);
	}

	return success;
}

bool YYJSONManager::PtrAddBool(YYJSONValue* handle, const char* path, bool value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrAddError;
	bool success = yyjson_mut_doc_ptr_addx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_bool(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrAddError);

	if (ptrAddError.code && error && error_size > 0) {
		snprintf(error, error_size, "Failed to add JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
			ptrAddError.msg, ptrAddError.code, ptrAddError.pos, path);
	}

	return success;
}

bool YYJSONManager::PtrAddFloat(YYJSONValue* handle, const char* path, double value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrAddError;
	bool success = yyjson_mut_doc_ptr_addx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_real(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrAddError);

	if (ptrAddError.code && error && error_size > 0) {
		snprintf(error, error_size, "Failed to add JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
			ptrAddError.msg, ptrAddError.code, ptrAddError.pos, path);
	}

	return success;
}

bool YYJSONManager::PtrAddInt(YYJSONValue* handle, const char* path, int value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrAddError;
	bool success = yyjson_mut_doc_ptr_addx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_int(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrAddError);

	if (ptrAddError.code && error && error_size > 0) {
		snprintf(error, error_size, "Failed to add JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
			ptrAddError.msg, ptrAddError.code, ptrAddError.pos, path);
	}

	return success;
}

bool YYJSONManager::PtrAddInt64(YYJSONValue* handle, const char* path, int64_t value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrAddError;
	bool success = yyjson_mut_doc_ptr_addx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_sint(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrAddError);

	if (ptrAddError.code && error && error_size > 0) {
		snprintf(error, error_size, "Failed to add JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
			ptrAddError.msg, ptrAddError.code, ptrAddError.pos, path);
	}

	return success;
}

bool YYJSONManager::PtrAddString(YYJSONValue* handle, const char* path, const char* value, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path || !value) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrAddError;
	bool success = yyjson_mut_doc_ptr_addx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_strcpy(handle->m_pDocument_mut.get(), value), true, nullptr, &ptrAddError);

	if (ptrAddError.code && error && error_size > 0) {
		snprintf(error, error_size, "Failed to add JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
			ptrAddError.msg, ptrAddError.code, ptrAddError.pos, path);
	}

	return success;
}

bool YYJSONManager::PtrAddNull(YYJSONValue* handle, const char* path, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrAddError;
	bool success = yyjson_mut_doc_ptr_addx(handle->m_pDocument_mut.get(), path, strlen(path), yyjson_mut_null(handle->m_pDocument_mut.get()), true, nullptr, &ptrAddError);

	if (ptrAddError.code && error && error_size > 0) {
		snprintf(error, error_size, "Failed to add JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
			ptrAddError.msg, ptrAddError.code, ptrAddError.pos, path);
	}

	return success;
}

bool YYJSONManager::PtrRemove(YYJSONValue* handle, const char* path, char* error, size_t error_size)
{
	if (!handle || !handle->IsMutable() || !path) {
		if (error && error_size > 0) {
			snprintf(error, error_size, "Invalid parameters or immutable document");
		}
		return false;
	}

	yyjson_ptr_err ptrRemoveError;
	bool success = yyjson_mut_doc_ptr_removex(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrRemoveError) != nullptr;

	if (ptrRemoveError.code && error && error_size > 0) {
		snprintf(error, error_size, "Failed to remove JSON pointer: %s (error code: %u, position: %zu, path: %s)", 
			ptrRemoveError.msg, ptrRemoveError.code, ptrRemoveError.pos, path);
	}

	return success;
}

YYJSONValue* YYJSONManager::PtrTryGet(YYJSONValue* handle, const char* path)
{
	if (!handle || !path) {
		return nullptr;
	}

	auto pYYJSONValue = CreateWrapper();
	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (!val || ptrGetError.code) {
			return nullptr;
		}

		pYYJSONValue->m_pDocument_mut = handle->m_pDocument_mut;
		pYYJSONValue->m_pVal_mut = val;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (!val || ptrGetError.code) {
			return nullptr;
		}

		pYYJSONValue->m_pDocument = handle->m_pDocument;
		pYYJSONValue->m_pVal = val;
	}

	return pYYJSONValue.release();
}

bool YYJSONManager::PtrTryGetBool(YYJSONValue* handle, const char* path, bool* out_value)
{
	if (!handle || !path || !out_value) {
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (!val || ptrGetError.code || !yyjson_mut_is_bool(val)) {
			return false;
		}

		*out_value = yyjson_mut_get_bool(val);
		return true;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (!val || ptrGetError.code || !yyjson_is_bool(val)) {
			return false;
		}

		*out_value = yyjson_get_bool(val);
		return true;
	}
}

bool YYJSONManager::PtrTryGetFloat(YYJSONValue* handle, const char* path, double* out_value)
{
	if (!handle || !path || !out_value) {
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (!val || ptrGetError.code || !yyjson_mut_is_num(val)) {
			return false;
		}

		*out_value = yyjson_mut_get_num(val);
		return true;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (!val || ptrGetError.code || !yyjson_is_num(val)) {
			return false;
		}

		*out_value = yyjson_get_num(val);
		return true;
	}
}

bool YYJSONManager::PtrTryGetInt(YYJSONValue* handle, const char* path, int* out_value)
{
	if (!handle || !path || !out_value) {
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (!val || ptrGetError.code || !yyjson_mut_is_int(val)) {
			return false;
		}
		*out_value = yyjson_mut_get_int(val);
		return true;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (!val || ptrGetError.code || !yyjson_is_int(val)) {
			return false;
		}
		*out_value = yyjson_get_int(val);
		return true;
	}
}

bool YYJSONManager::PtrTryGetInt64(YYJSONValue* handle, const char* path, int64_t* out_value)
{
	if (!handle || !path || !out_value) {
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);
		if (!val || ptrGetError.code || !yyjson_mut_is_int(val)) {
			return false;
		}
		*out_value = yyjson_mut_get_sint(val);
		return true;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);
		if (!val || ptrGetError.code || !yyjson_is_int(val)) {
			return false;
		}
		*out_value = yyjson_get_sint(val);
		return true;
	}
}

bool YYJSONManager::PtrTryGetString(YYJSONValue* handle, const char* path, const char** out_str, size_t* out_len)
{
	if (!handle || !path || !out_str) {
		return false;
	}

	yyjson_ptr_err ptrGetError;

	if (handle->IsMutable()) {
		yyjson_mut_val* val = yyjson_mut_doc_ptr_getx(handle->m_pDocument_mut.get(), path, strlen(path), nullptr, &ptrGetError);

		if (!val || ptrGetError.code || !yyjson_mut_is_str(val)) {
			return false;
		}

		*out_str = yyjson_mut_get_str(val);
		if (out_len) {
			*out_len = yyjson_mut_get_len(val);
		}
		return true;
	} else {
		yyjson_val* val = yyjson_doc_ptr_getx(handle->m_pDocument.get(), path, strlen(path), &ptrGetError);

		if (!val || ptrGetError.code || !yyjson_is_str(val)) {
			return false;
		}

		*out_str = yyjson_get_str(val);
		if (out_len) {
			*out_len = yyjson_get_len(val);
		}
		return true;
	}
}

bool YYJSONManager::ObjectForeachNext(YYJSONValue* handle, const char** out_key, 
                                       size_t* out_key_len, YYJSONValue** out_value)
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

bool YYJSONManager::ArrayForeachNext(YYJSONValue* handle, size_t* out_index, 
                                      YYJSONValue** out_value)
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

bool YYJSONManager::ObjectForeachKeyNext(YYJSONValue* handle, const char** out_key, 
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

bool YYJSONManager::ArrayForeachIndexNext(YYJSONValue* handle, size_t* out_index)
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

void YYJSONManager::Release(YYJSONValue* value)
{
	if (value) {
		delete value;
	}
}

HandleType_t YYJSONManager::GetHandleType()
{
  return g_htJSON;
}

YYJSONValue* YYJSONManager::GetFromHandle(IPluginContext* pContext, Handle_t handle)
{
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	YYJSONValue* pYYJSONValue;
	if ((err = handlesys->ReadHandle(handle, g_htJSON, &sec, (void**)&pYYJSONValue)) != HandleError_None)
	{
		pContext->ReportError("Invalid YYJSON handle %x (error %d)", handle, err);
		return nullptr;
	}

	return pYYJSONValue;
}