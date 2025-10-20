#ifndef _INCLUDE_SM_YYJSON_YYJSONMANAGER_H_
#define _INCLUDE_SM_YYJSON_YYJSONMANAGER_H_

#include <IYYJSONManager.h>
#include <yyjson.h>
#include <IHandleSys.h>
#include <random>
#include <memory>

/**
 * @brief JSON value wrapper
 *
 * Wraps yyjson mutable/immutable documents and values.
 * Used as the primary data type for JSON operations.
 */
class YYJSONValue {
public:
	YYJSONValue() = default;
	~YYJSONValue() {
		if (m_pDocument_mut.unique()) {
			yyjson_mut_doc_free(m_pDocument_mut.get());
		}
		if (m_pDocument.unique()) {
			yyjson_doc_free(m_pDocument.get());
		}
	}

	YYJSONValue(const YYJSONValue&) = delete;
	YYJSONValue& operator=(const YYJSONValue&) = delete;

	void ResetObjectIterator() {
		m_iterInitialized = false;
	}

	void ResetArrayIterator() {
		m_iterInitialized = false;
		m_arrayIndex = 0;
	}

	bool IsMutable() const {
		return m_pDocument_mut != nullptr;
	}

	bool IsImmutable() const {
		return m_pDocument != nullptr;
	}

	// Mutable document
	std::shared_ptr<yyjson_mut_doc> m_pDocument_mut;
	yyjson_mut_val* m_pVal_mut{ nullptr };

	// Immutable document
	std::shared_ptr<yyjson_doc> m_pDocument;
	yyjson_val* m_pVal{ nullptr };

	// Mutable document iterators
	yyjson_mut_obj_iter m_iterObj;
	yyjson_mut_arr_iter m_iterArr;

	// Immutable document iterators
	yyjson_obj_iter m_iterObjImm;
	yyjson_arr_iter m_iterArrImm;

	Handle_t m_handle{ BAD_HANDLE };
	size_t m_arrayIndex{ 0 };
	size_t m_readSize{ 0 };
	bool m_iterInitialized{ false };
};

class YYJSONManager : public IYYJSONManager
{
public:
	YYJSONManager();
	~YYJSONManager();

public:
	// ========== Document Operations ==========
	virtual YYJSONValue* ParseJSON(const char* json_str, bool is_file, bool is_mutable, 
		yyjson_read_flag read_flg, char* error, size_t error_size) override;
	virtual bool WriteToString(YYJSONValue* handle, char* buffer, size_t buffer_size, 
		yyjson_write_flag write_flg, size_t* out_size) override;
	virtual bool WriteToFile(YYJSONValue* handle, const char* path, yyjson_write_flag write_flg,
		char* error, size_t error_size) override;
	virtual bool Equals(YYJSONValue* handle1, YYJSONValue* handle2) override;
	virtual YYJSONValue* DeepCopy(YYJSONValue* targetDoc, YYJSONValue* sourceValue) override;
	virtual const char* GetTypeDesc(YYJSONValue* handle) override;
	virtual size_t GetSerializedSize(YYJSONValue* handle, yyjson_write_flag write_flg) override;
	virtual YYJSONValue* ToMutable(YYJSONValue* handle) override;
	virtual YYJSONValue* ToImmutable(YYJSONValue* handle) override;
	virtual yyjson_type GetType(YYJSONValue* handle) override;
	virtual yyjson_subtype GetSubtype(YYJSONValue* handle) override;
	virtual bool IsArray(YYJSONValue* handle) override;
	virtual bool IsObject(YYJSONValue* handle) override;
	virtual bool IsInt(YYJSONValue* handle) override;
	virtual bool IsUint(YYJSONValue* handle) override;
	virtual bool IsSint(YYJSONValue* handle) override;
	virtual bool IsNum(YYJSONValue* handle) override;
	virtual bool IsBool(YYJSONValue* handle) override;
	virtual bool IsTrue(YYJSONValue* handle) override;
	virtual bool IsFalse(YYJSONValue* handle) override;
	virtual bool IsFloat(YYJSONValue* handle) override;
	virtual bool IsStr(YYJSONValue* handle) override;
	virtual bool IsNull(YYJSONValue* handle) override;
	virtual bool IsCtn(YYJSONValue* handle) override;
	virtual bool IsMutable(YYJSONValue* handle) override;
	virtual bool IsImmutable(YYJSONValue* handle) override;
	virtual size_t GetReadSize(YYJSONValue* handle) override;

	// ========== Object Operations ==========
	virtual YYJSONValue* ObjectInit() override;
	virtual YYJSONValue* ObjectInitWithStrings(const char** pairs, size_t count) override;
	virtual YYJSONValue* ObjectParseString(const char* str, yyjson_read_flag read_flg, 
		char* error, size_t error_size) override;
	virtual YYJSONValue* ObjectParseFile(const char* path, yyjson_read_flag read_flg,
		char* error, size_t error_size) override;
	virtual size_t ObjectGetSize(YYJSONValue* handle) override;
	virtual bool ObjectGetKey(YYJSONValue* handle, size_t index, const char** out_key) override;
	virtual YYJSONValue* ObjectGetValueAt(YYJSONValue* handle, size_t index) override;
	virtual YYJSONValue* ObjectGet(YYJSONValue* handle, const char* key) override;
	virtual bool ObjectGetBool(YYJSONValue* handle, const char* key, bool* out_value) override;
	virtual bool ObjectGetFloat(YYJSONValue* handle, const char* key, double* out_value) override;
	virtual bool ObjectGetInt(YYJSONValue* handle, const char* key, int* out_value) override;
	virtual bool ObjectGetInt64(YYJSONValue* handle, const char* key, int64_t* out_value) override;
	virtual bool ObjectGetString(YYJSONValue* handle, const char* key, const char** out_str, size_t* out_len) override;
	virtual bool ObjectIsNull(YYJSONValue* handle, const char* key, bool* out_is_null) override;
	virtual bool ObjectHasKey(YYJSONValue* handle, const char* key, bool use_pointer) override;
	virtual bool ObjectRenameKey(YYJSONValue* handle, const char* old_key, const char* new_key, bool allow_duplicate) override;
	virtual bool ObjectSet(YYJSONValue* handle, const char* key, YYJSONValue* value) override;
	virtual bool ObjectSetBool(YYJSONValue* handle, const char* key, bool value) override;
	virtual bool ObjectSetFloat(YYJSONValue* handle, const char* key, double value) override;
	virtual bool ObjectSetInt(YYJSONValue* handle, const char* key, int value) override;
	virtual bool ObjectSetInt64(YYJSONValue* handle, const char* key, int64_t value) override;
	virtual bool ObjectSetNull(YYJSONValue* handle, const char* key) override;
	virtual bool ObjectSetString(YYJSONValue* handle, const char* key, const char* value) override;
	virtual bool ObjectRemove(YYJSONValue* handle, const char* key) override;
	virtual bool ObjectClear(YYJSONValue* handle) override;
	virtual bool ObjectSort(YYJSONValue* handle, YYJSON_SORT_ORDER sort_mode) override;

	// ========== Array Operations ==========
	virtual YYJSONValue* ArrayInit() override;
	virtual YYJSONValue* ArrayInitWithStrings(const char** strings, size_t count) override;
	virtual YYJSONValue* ArrayParseString(const char* str, yyjson_read_flag read_flg,
		char* error, size_t error_size) override;
	virtual YYJSONValue* ArrayParseFile(const char* path, yyjson_read_flag read_flg,
		char* error, size_t error_size) override;
	virtual size_t ArrayGetSize(YYJSONValue* handle) override;
	virtual YYJSONValue* ArrayGet(YYJSONValue* handle, size_t index) override;
	virtual YYJSONValue* ArrayGetFirst(YYJSONValue* handle) override;
	virtual YYJSONValue* ArrayGetLast(YYJSONValue* handle) override;
	virtual bool ArrayGetBool(YYJSONValue* handle, size_t index, bool* out_value) override;
	virtual bool ArrayGetFloat(YYJSONValue* handle, size_t index, double* out_value) override;
	virtual bool ArrayGetInt(YYJSONValue* handle, size_t index, int* out_value) override;
	virtual bool ArrayGetInt64(YYJSONValue* handle, size_t index, int64_t* out_value) override;
	virtual bool ArrayGetString(YYJSONValue* handle, size_t index, const char** out_str, size_t* out_len) override;
	virtual bool ArrayIsNull(YYJSONValue* handle, size_t index) override;
	virtual bool ArrayReplace(YYJSONValue* handle, size_t index, YYJSONValue* value) override;
	virtual bool ArrayReplaceBool(YYJSONValue* handle, size_t index, bool value) override;
	virtual bool ArrayReplaceFloat(YYJSONValue* handle, size_t index, double value) override;
	virtual bool ArrayReplaceInt(YYJSONValue* handle, size_t index, int value) override;
	virtual bool ArrayReplaceInt64(YYJSONValue* handle, size_t index, int64_t value) override;
	virtual bool ArrayReplaceNull(YYJSONValue* handle, size_t index) override;
	virtual bool ArrayReplaceString(YYJSONValue* handle, size_t index, const char* value) override;
	virtual bool ArrayAppend(YYJSONValue* handle, YYJSONValue* value) override;
	virtual bool ArrayAppendBool(YYJSONValue* handle, bool value) override;
	virtual bool ArrayAppendFloat(YYJSONValue* handle, double value) override;
	virtual bool ArrayAppendInt(YYJSONValue* handle, int value) override;
	virtual bool ArrayAppendInt64(YYJSONValue* handle, int64_t value) override;
	virtual bool ArrayAppendNull(YYJSONValue* handle) override;
	virtual bool ArrayAppendString(YYJSONValue* handle, const char* value) override;
	virtual bool ArrayRemove(YYJSONValue* handle, size_t index) override;
	virtual bool ArrayRemoveFirst(YYJSONValue* handle) override;
	virtual bool ArrayRemoveLast(YYJSONValue* handle) override;
	virtual bool ArrayRemoveRange(YYJSONValue* handle, size_t start_index, size_t end_index) override;
	virtual bool ArrayClear(YYJSONValue* handle) override;
	virtual int ArrayIndexOfBool(YYJSONValue* handle, bool search_value) override;
	virtual int ArrayIndexOfString(YYJSONValue* handle, const char* search_value) override;
	virtual int ArrayIndexOfInt(YYJSONValue* handle, int search_value) override;
	virtual int ArrayIndexOfInt64(YYJSONValue* handle, int64_t search_value) override;
	virtual int ArrayIndexOfFloat(YYJSONValue* handle, double search_value) override;
	virtual bool ArraySort(YYJSONValue* handle, YYJSON_SORT_ORDER sort_mode) override;

	// ========== Value Operations ==========
	virtual YYJSONValue* Pack(const char* format, IPackParamProvider* param_provider, char* error, size_t error_size) override;
	virtual YYJSONValue* CreateBool(bool value) override;
	virtual YYJSONValue* CreateFloat(double value) override;
	virtual YYJSONValue* CreateInt(int value) override;
	virtual YYJSONValue* CreateInt64(int64_t value) override;
	virtual YYJSONValue* CreateNull() override;
	virtual YYJSONValue* CreateString(const char* value) override;
	virtual bool GetBool(YYJSONValue* handle, bool* out_value) override;
	virtual bool GetFloat(YYJSONValue* handle, double* out_value) override;
	virtual bool GetInt(YYJSONValue* handle, int* out_value) override;
	virtual bool GetInt64(YYJSONValue* handle, int64_t* out_value) override;
	virtual bool GetString(YYJSONValue* handle, const char** out_str, size_t* out_len) override;

	// ========== Pointer Operations ==========
	virtual YYJSONValue* PtrGet(YYJSONValue* handle, const char* path, char* error, size_t error_size) override;
	virtual bool PtrGetBool(YYJSONValue* handle, const char* path, bool* out_value, char* error, size_t error_size) override;
	virtual bool PtrGetFloat(YYJSONValue* handle, const char* path, double* out_value, char* error, size_t error_size) override;
	virtual bool PtrGetInt(YYJSONValue* handle, const char* path, int* out_value, char* error, size_t error_size) override;
	virtual bool PtrGetInt64(YYJSONValue* handle, const char* path, int64_t* out_value, char* error, size_t error_size) override;
	virtual bool PtrGetString(YYJSONValue* handle, const char* path, const char** out_str, size_t* out_len, char* error, size_t error_size) override;
	virtual bool PtrGetIsNull(YYJSONValue* handle, const char* path, bool* out_is_null, char* error, size_t error_size) override;
	virtual bool PtrGetLength(YYJSONValue* handle, const char* path, size_t* out_len, char* error, size_t error_size) override;
	virtual bool PtrSet(YYJSONValue* handle, const char* path, YYJSONValue* value, char* error, size_t error_size) override;
	virtual bool PtrSetBool(YYJSONValue* handle, const char* path, bool value, char* error, size_t error_size) override;
	virtual bool PtrSetFloat(YYJSONValue* handle, const char* path, double value, char* error, size_t error_size) override;
	virtual bool PtrSetInt(YYJSONValue* handle, const char* path, int value, char* error, size_t error_size) override;
	virtual bool PtrSetInt64(YYJSONValue* handle, const char* path, int64_t value, char* error, size_t error_size) override;
	virtual bool PtrSetString(YYJSONValue* handle, const char* path, const char* value, char* error, size_t error_size) override;
	virtual bool PtrSetNull(YYJSONValue* handle, const char* path, char* error, size_t error_size) override;
	virtual bool PtrAdd(YYJSONValue* handle, const char* path, YYJSONValue* value, char* error, size_t error_size) override;
	virtual bool PtrAddBool(YYJSONValue* handle, const char* path, bool value, char* error, size_t error_size) override;
	virtual bool PtrAddFloat(YYJSONValue* handle, const char* path, double value, char* error, size_t error_size) override;
	virtual bool PtrAddInt(YYJSONValue* handle, const char* path, int value, char* error, size_t error_size) override;
	virtual bool PtrAddInt64(YYJSONValue* handle, const char* path, int64_t value, char* error, size_t error_size) override;
	virtual bool PtrAddString(YYJSONValue* handle, const char* path, const char* value, char* error, size_t error_size) override;
	virtual bool PtrAddNull(YYJSONValue* handle, const char* path, char* error, size_t error_size) override;
	virtual bool PtrRemove(YYJSONValue* handle, const char* path, char* error, size_t error_size) override;
	virtual YYJSONValue* PtrTryGet(YYJSONValue* handle, const char* path) override;
	virtual bool PtrTryGetBool(YYJSONValue* handle, const char* path, bool* out_value) override;
	virtual bool PtrTryGetFloat(YYJSONValue* handle, const char* path, double* out_value) override;
	virtual bool PtrTryGetInt(YYJSONValue* handle, const char* path, int* out_value) override;
	virtual bool PtrTryGetInt64(YYJSONValue* handle, const char* path, int64_t* out_value) override;
	virtual bool PtrTryGetString(YYJSONValue* handle, const char* path, const char** out_str, size_t* out_len) override;

	// ========== Iterator Operations ==========
	virtual bool ObjectForeachNext(YYJSONValue* handle, const char** out_key, 
	                                size_t* out_key_len, YYJSONValue** out_value) override;
	virtual bool ArrayForeachNext(YYJSONValue* handle, size_t* out_index, 
	                               YYJSONValue** out_value) override;
	virtual bool ObjectForeachKeyNext(YYJSONValue* handle, const char** out_key, 
	                                   size_t* out_key_len) override;
	virtual bool ArrayForeachIndexNext(YYJSONValue* handle, size_t* out_index) override;

	// ========== Release Operations ==========
	virtual void Release(YYJSONValue* value) override;

	// ========== Handle Type Operations ==========
	virtual HandleType_t GetHandleType() override;

	// ========== Handle Operations ==========
	virtual YYJSONValue* GetFromHandle(IPluginContext* pContext, Handle_t handle) override;

private:
	std::random_device m_randomDevice;
	std::mt19937 m_randomGenerator;

	// Helper methods
	static std::unique_ptr<YYJSONValue> CreateWrapper();
	static std::shared_ptr<yyjson_mut_doc> WrapDocument(yyjson_mut_doc* doc);
	static std::shared_ptr<yyjson_mut_doc> CopyDocument(yyjson_doc* doc);
	static std::shared_ptr<yyjson_mut_doc> CreateDocument();
	static std::shared_ptr<yyjson_doc> WrapImmutableDocument(yyjson_doc* doc);

	// Pack helper methods
	static const char* SkipSeparators(const char* ptr);
	static void SetPackError(char* error, size_t error_size, const char* fmt, ...);
	static yyjson_mut_val* PackImpl(yyjson_mut_doc* doc, const char* format, 
	                                 IPackParamProvider* provider, char* error, 
	                                 size_t error_size, const char** out_end_ptr);
};

#endif // _INCLUDE_SM_YYJSON_YYJSONMANAGER_H_