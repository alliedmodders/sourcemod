#ifndef _INCLUDE_JSONMANAGER_H_
#define _INCLUDE_JSONMANAGER_H_

#include <IJsonManager.h>
#include <yyjson.h>
#include <random>
#include <memory>
#include <charconv>

/**
 * @brief Base class for intrusive reference counting
 *
 * Objects inheriting from this class can be managed by RefPtr.
 * Reference count starts at 0 and is incremented when RefPtr takes ownership.
 */
class RefCounted {
private:
	mutable size_t ref_count_ = 0;

protected:
	virtual ~RefCounted() = default;
	RefCounted() = default;

	RefCounted(const RefCounted &) = delete;
	RefCounted &operator=(const RefCounted &) = delete;

	RefCounted(RefCounted &&) noexcept : ref_count_(0) {}
	RefCounted &operator=(RefCounted &&) noexcept { return *this; }

public:
	void add_ref() const noexcept { ++ref_count_; }

	void release() const noexcept {
		assert(ref_count_ > 0 && "Reference count underflow");
		if (--ref_count_ == 0) {
			delete this;
		}
	}

	size_t use_count() const noexcept { return ref_count_; }
};

/**
 * @brief Smart pointer for intrusive reference counting
 *
 * Similar to std::shared_ptr but uses intrusive reference counting.
 * Automatically manages object lifetime through add_ref() and release().
 * @warning This implementation is not thread-safe. It must only be used
 *          in single-threaded contexts or with external synchronization.
 */
template <typename T> class RefPtr {
private:
	T *ptr_;

public:
	RefPtr() noexcept : ptr_(nullptr) {}

	explicit RefPtr(T *p) noexcept : ptr_(p) {
		if (ptr_)
			ptr_->add_ref();
	}

	RefPtr(const RefPtr &other) noexcept : ptr_(other.ptr_) {
		if (ptr_)
			ptr_->add_ref();
	}

	RefPtr(RefPtr &&other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }

	~RefPtr() {
		if (ptr_)
			ptr_->release();
	}

	RefPtr &operator=(const RefPtr &other) noexcept {
		if (ptr_ != other.ptr_) {
			if (other.ptr_)
				other.ptr_->add_ref();
			if (ptr_)
				ptr_->release();
			ptr_ = other.ptr_;
		}
		return *this;
	}

	RefPtr &operator=(RefPtr &&other) noexcept {
		if (this != &other) {
			if (ptr_)
				ptr_->release();
			ptr_ = other.ptr_;
			other.ptr_ = nullptr;
		}
		return *this;
	}

	RefPtr &operator=(T *p) noexcept {
		reset(p);
		return *this;
	}

	T *operator->() const noexcept { return ptr_; }
	T &operator*() const noexcept { return *ptr_; }
	T *get() const noexcept { return ptr_; }

	explicit operator bool() const noexcept { return ptr_ != nullptr; }

	size_t use_count() const noexcept { return ptr_ ? ptr_->use_count() : 0; }

	void reset(T *p = nullptr) noexcept {
		if (ptr_ != p) {
			if (p)
				p->add_ref();
			if (ptr_)
				ptr_->release();
			ptr_ = p;
		}
	}

	bool operator==(const RefPtr &other) const noexcept { return ptr_ == other.ptr_; }
	bool operator!=(const RefPtr &other) const noexcept { return ptr_ != other.ptr_; }
	bool operator==(std::nullptr_t) const noexcept { return ptr_ == nullptr; }
	bool operator!=(std::nullptr_t) const noexcept { return ptr_ != nullptr; }
};

/**
 * @brief Factory function to create RefPtr from new object
 */
template <typename T, typename... Args> RefPtr<T> make_ref(Args &&...args) {
	return RefPtr<T>(new T(std::forward<Args>(args)...));
}

/**
 * @brief Wrapper for yyjson_mut_doc with intrusive reference counting
 */
class RefCountedMutDoc : public RefCounted {
private:
	yyjson_mut_doc *doc_;

public:
	explicit RefCountedMutDoc(yyjson_mut_doc *doc) noexcept : doc_(doc) {}

	RefCountedMutDoc(const RefCountedMutDoc &) = delete;
	RefCountedMutDoc &operator=(const RefCountedMutDoc &) = delete;

	~RefCountedMutDoc() noexcept override {
		if (doc_) {
			yyjson_mut_doc_free(doc_);
		}
	}

	yyjson_mut_doc *get() const noexcept { return doc_; }
};

/**
 * @brief Wrapper for yyjson_doc with intrusive reference counting
 */
class RefCountedImmutableDoc : public RefCounted {
private:
	yyjson_doc *doc_;

public:
	explicit RefCountedImmutableDoc(yyjson_doc *doc) noexcept : doc_(doc) {}

	RefCountedImmutableDoc(const RefCountedImmutableDoc &) = delete;
	RefCountedImmutableDoc &operator=(const RefCountedImmutableDoc &) = delete;

	~RefCountedImmutableDoc() noexcept override {
		if (doc_) {
			yyjson_doc_free(doc_);
		}
	}

	yyjson_doc *get() const noexcept { return doc_; }
};

/**
 * @brief JSON value wrapper
 *
 * Wraps json mutable/immutable documents and values.
 * Used as the primary data type for JSON operations.
 */
class JsonValue {
public:
	JsonValue() = default;
	~JsonValue() = default;

	JsonValue(const JsonValue&) = delete;
	JsonValue& operator=(const JsonValue&) = delete;

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

	size_t GetDocumentRefCount() const {
		if (m_pDocument_mut) {
			return m_pDocument_mut.use_count();
		} else if (m_pDocument) {
			return m_pDocument.use_count();
		}
		return 0;
	}

	// Mutable document
	RefPtr<RefCountedMutDoc> m_pDocument_mut;
	yyjson_mut_val* m_pVal_mut{ nullptr };

	// Immutable document
	RefPtr<RefCountedImmutableDoc> m_pDocument;
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

/**
 * @brief Array iterator wrapper
 *
 * Wraps yyjson_arr_iter and yyjson_mut_arr_iter for array iteration.
 */
class JsonArrIter {
public:
	JsonArrIter() = default;
	~JsonArrIter() = default;

	JsonArrIter(const JsonArrIter&) = delete;
	JsonArrIter& operator=(const JsonArrIter&) = delete;

	bool IsMutable() const {
		return m_isMutable;
	}

	RefPtr<RefCountedMutDoc> m_pDocument_mut;
	RefPtr<RefCountedImmutableDoc> m_pDocument;

	yyjson_mut_arr_iter m_iterMut;
	yyjson_arr_iter m_iterImm;
	yyjson_mut_val* m_rootMut{ nullptr };
	yyjson_val* m_rootImm{ nullptr };

	Handle_t m_handle{ BAD_HANDLE };
	bool m_isMutable{ false };
	bool m_initialized{ false };
};

/**
 * @brief Object iterator wrapper
 *
 * Wraps yyjson_obj_iter and yyjson_mut_obj_iter for object iteration.
 */
class JsonObjIter {
public:
	JsonObjIter() = default;
	~JsonObjIter() = default;

	JsonObjIter(const JsonObjIter&) = delete;
	JsonObjIter& operator=(const JsonObjIter&) = delete;

	bool IsMutable() const {
		return m_isMutable;
	}

	RefPtr<RefCountedMutDoc> m_pDocument_mut;
	RefPtr<RefCountedImmutableDoc> m_pDocument;

	yyjson_mut_obj_iter m_iterMut;
	yyjson_obj_iter m_iterImm;
	yyjson_mut_val* m_rootMut{ nullptr };
	yyjson_val* m_rootImm{ nullptr };

	void* m_currentKey{ nullptr };

	Handle_t m_handle{ BAD_HANDLE };
	bool m_isMutable{ false };
	bool m_initialized{ false };
};

class JsonManager : public IJsonManager
{
public:
	JsonManager();
	~JsonManager();

public:
	// ========== Document Operations ==========
	virtual JsonValue* ParseJSON(const char* json_str, bool is_file, bool is_mutable,
		yyjson_read_flag read_flg, char* error, size_t error_size) override;
	virtual bool WriteToString(JsonValue* handle, char* buffer, size_t buffer_size,
		yyjson_write_flag write_flg, size_t* out_size) override;
	virtual char* WriteToStringPtr(JsonValue* handle, yyjson_write_flag write_flg, size_t* out_size) override;
	virtual JsonValue* ApplyJsonPatch(JsonValue* target, JsonValue* patch, bool result_mutable,
		char* error, size_t error_size) override;
	virtual bool JsonPatchInPlace(JsonValue* target, JsonValue* patch,
		char* error, size_t error_size) override;
	virtual JsonValue* ApplyMergePatch(JsonValue* target, JsonValue* patch, bool result_mutable,
		char* error, size_t error_size) override;
	virtual bool MergePatchInPlace(JsonValue* target, JsonValue* patch,
		char* error, size_t error_size) override;
	virtual bool WriteToFile(JsonValue* handle, const char* path, yyjson_write_flag write_flg,
		char* error, size_t error_size) override;
	virtual bool Equals(JsonValue* handle1, JsonValue* handle2) override;
	virtual bool EqualsStr(JsonValue* handle, const char* str) override;
	virtual JsonValue* DeepCopy(JsonValue* targetDoc, JsonValue* sourceValue) override;
	virtual const char* GetTypeDesc(JsonValue* handle) override;
	virtual size_t GetSerializedSize(JsonValue* handle, yyjson_write_flag write_flg) override;
	virtual JsonValue* ToMutable(JsonValue* handle) override;
	virtual JsonValue* ToImmutable(JsonValue* handle) override;
	virtual yyjson_type GetType(JsonValue* handle) override;
	virtual yyjson_subtype GetSubtype(JsonValue* handle) override;
	virtual bool IsArray(JsonValue* handle) override;
	virtual bool IsObject(JsonValue* handle) override;
	virtual bool IsInt(JsonValue* handle) override;
	virtual bool IsUint(JsonValue* handle) override;
	virtual bool IsSint(JsonValue* handle) override;
	virtual bool IsNum(JsonValue* handle) override;
	virtual bool IsBool(JsonValue* handle) override;
	virtual bool IsTrue(JsonValue* handle) override;
	virtual bool IsFalse(JsonValue* handle) override;
	virtual bool IsFloat(JsonValue* handle) override;
	virtual bool IsStr(JsonValue* handle) override;
	virtual bool IsNull(JsonValue* handle) override;
	virtual bool IsCtn(JsonValue* handle) override;
	virtual bool IsMutable(JsonValue* handle) override;
	virtual bool IsImmutable(JsonValue* handle) override;
	virtual size_t GetReadSize(JsonValue* handle) override;
	virtual size_t GetRefCount(JsonValue* handle) override;
	virtual size_t GetValCount(JsonValue* handle) override;

	// ========== Object Operations ==========
	virtual JsonValue* ObjectInit() override;
	virtual JsonValue* ObjectInitWithStrings(const char** pairs, size_t count) override;
	virtual JsonValue* ObjectParseString(const char* str, yyjson_read_flag read_flg,
		char* error, size_t error_size) override;
	virtual JsonValue* ObjectParseFile(const char* path, yyjson_read_flag read_flg,
		char* error, size_t error_size) override;
	virtual size_t ObjectGetSize(JsonValue* handle) override;
	virtual bool ObjectGetKey(JsonValue* handle, size_t index, const char** out_key) override;
	virtual JsonValue* ObjectGetValueAt(JsonValue* handle, size_t index) override;
	virtual JsonValue* ObjectGet(JsonValue* handle, const char* key) override;
	virtual bool ObjectGetBool(JsonValue* handle, const char* key, bool* out_value) override;
	virtual bool ObjectGetDouble(JsonValue* handle, const char* key, double* out_value) override;
	virtual bool ObjectGetInt(JsonValue* handle, const char* key, int* out_value) override;
	virtual bool ObjectGetInt64(JsonValue* handle, const char* key, std::variant<int64_t, uint64_t>* out_value) override;
	virtual bool ObjectGetString(JsonValue* handle, const char* key, const char** out_str, size_t* out_len) override;
	virtual bool ObjectIsNull(JsonValue* handle, const char* key, bool* out_is_null) override;
	virtual bool ObjectHasKey(JsonValue* handle, const char* key, bool use_pointer) override;
	virtual bool ObjectRenameKey(JsonValue* handle, const char* old_key, const char* new_key, bool allow_duplicate) override;
	virtual bool ObjectSet(JsonValue* handle, const char* key, JsonValue* value) override;
	virtual bool ObjectSetBool(JsonValue* handle, const char* key, bool value) override;
	virtual bool ObjectSetDouble(JsonValue* handle, const char* key, double value) override;
	virtual bool ObjectSetInt(JsonValue* handle, const char* key, int value) override;
	virtual bool ObjectSetInt64(JsonValue* handle, const char* key, std::variant<int64_t, uint64_t> value) override;
	virtual bool ObjectSetNull(JsonValue* handle, const char* key) override;
	virtual bool ObjectSetString(JsonValue* handle, const char* key, const char* value) override;
	virtual bool ObjectRemove(JsonValue* handle, const char* key) override;
	virtual bool ObjectClear(JsonValue* handle) override;
	virtual bool ObjectSort(JsonValue* handle, JSON_SORT_ORDER sort_mode) override;
	virtual bool ObjectRotate(JsonValue* handle, size_t idx) override;

	// ========== Array Operations ==========
	virtual JsonValue* ArrayInit() override;
	virtual JsonValue* ArrayInitWithStrings(const char** strings, size_t count) override;
	virtual JsonValue* ArrayInitWithInt32(const int32_t* values, size_t count) override;
	virtual JsonValue* ArrayInitWithInt64(const char** values, size_t count,
		char* error, size_t error_size) override;
	virtual JsonValue* ArrayInitWithBool(const bool* values, size_t count) override;
	virtual JsonValue* ArrayInitWithDouble(const double* values, size_t count) override;
	virtual JsonValue* ArrayParseString(const char* str, yyjson_read_flag read_flg,
		char* error, size_t error_size) override;
	virtual JsonValue* ArrayParseFile(const char* path, yyjson_read_flag read_flg,
		char* error, size_t error_size) override;
	virtual size_t ArrayGetSize(JsonValue* handle) override;
	virtual JsonValue* ArrayGet(JsonValue* handle, size_t index) override;
	virtual JsonValue* ArrayGetFirst(JsonValue* handle) override;
	virtual JsonValue* ArrayGetLast(JsonValue* handle) override;
	virtual bool ArrayGetBool(JsonValue* handle, size_t index, bool* out_value) override;
	virtual bool ArrayGetDouble(JsonValue* handle, size_t index, double* out_value) override;
	virtual bool ArrayGetInt(JsonValue* handle, size_t index, int* out_value) override;
	virtual bool ArrayGetInt64(JsonValue* handle, size_t index, std::variant<int64_t, uint64_t>* out_value) override;
	virtual bool ArrayGetString(JsonValue* handle, size_t index, const char** out_str, size_t* out_len) override;
	virtual bool ArrayIsNull(JsonValue* handle, size_t index) override;
	virtual bool ArrayReplace(JsonValue* handle, size_t index, JsonValue* value) override;
	virtual bool ArrayReplaceBool(JsonValue* handle, size_t index, bool value) override;
	virtual bool ArrayReplaceDouble(JsonValue* handle, size_t index, double value) override;
	virtual bool ArrayReplaceInt(JsonValue* handle, size_t index, int value) override;
	virtual bool ArrayReplaceInt64(JsonValue* handle, size_t index, std::variant<int64_t, uint64_t> value) override;
	virtual bool ArrayReplaceNull(JsonValue* handle, size_t index) override;
	virtual bool ArrayReplaceString(JsonValue* handle, size_t index, const char* value) override;
	virtual bool ArrayAppend(JsonValue* handle, JsonValue* value) override;
	virtual bool ArrayAppendBool(JsonValue* handle, bool value) override;
	virtual bool ArrayAppendDouble(JsonValue* handle, double value) override;
	virtual bool ArrayAppendInt(JsonValue* handle, int value) override;
	virtual bool ArrayAppendInt64(JsonValue* handle, std::variant<int64_t, uint64_t> value) override;
	virtual bool ArrayAppendNull(JsonValue* handle) override;
	virtual bool ArrayAppendString(JsonValue* handle, const char* value) override;
	virtual bool ArrayInsert(JsonValue* handle, size_t index, JsonValue* value) override;
	virtual bool ArrayInsertBool(JsonValue* handle, size_t index, bool value) override;
	virtual bool ArrayInsertInt(JsonValue* handle, size_t index, int value) override;
	virtual bool ArrayInsertInt64(JsonValue* handle, size_t index, std::variant<int64_t, uint64_t> value) override;
	virtual bool ArrayInsertDouble(JsonValue* handle, size_t index, double value) override;
	virtual bool ArrayInsertString(JsonValue* handle, size_t index, const char* value) override;
	virtual bool ArrayInsertNull(JsonValue* handle, size_t index) override;
	virtual bool ArrayPrepend(JsonValue* handle, JsonValue* value) override;
	virtual bool ArrayPrependBool(JsonValue* handle, bool value) override;
	virtual bool ArrayPrependInt(JsonValue* handle, int value) override;
	virtual bool ArrayPrependInt64(JsonValue* handle, std::variant<int64_t, uint64_t> value) override;
	virtual bool ArrayPrependDouble(JsonValue* handle, double value) override;
	virtual bool ArrayPrependString(JsonValue* handle, const char* value) override;
	virtual bool ArrayPrependNull(JsonValue* handle) override;
	virtual bool ArrayRemove(JsonValue* handle, size_t index) override;
	virtual bool ArrayRemoveFirst(JsonValue* handle) override;
	virtual bool ArrayRemoveLast(JsonValue* handle) override;
	virtual bool ArrayRemoveRange(JsonValue* handle, size_t start_index, size_t count) override;
	virtual bool ArrayClear(JsonValue* handle) override;
	virtual int ArrayIndexOfBool(JsonValue* handle, bool search_value) override;
	virtual int ArrayIndexOfString(JsonValue* handle, const char* search_value) override;
	virtual int ArrayIndexOfInt(JsonValue* handle, int search_value) override;
	virtual int ArrayIndexOfInt64(JsonValue* handle, std::variant<int64_t, uint64_t> search_value) override;
	virtual int ArrayIndexOfDouble(JsonValue* handle, double search_value) override;
	virtual bool ArraySort(JsonValue* handle, JSON_SORT_ORDER sort_mode) override;
	virtual bool ArrayRotate(JsonValue* handle, size_t idx) override;

	// ========== Value Operations ==========
	virtual JsonValue* Pack(const char* format, IPackParamProvider* param_provider, char* error, size_t error_size) override;
	virtual JsonValue* CreateBool(bool value) override;
	virtual JsonValue* CreateDouble(double value) override;
	virtual JsonValue* CreateInt(int value) override;
	virtual JsonValue* CreateInt64(std::variant<int64_t, uint64_t> value) override;
	virtual JsonValue* CreateNull() override;
	virtual JsonValue* CreateString(const char* value) override;
	virtual bool GetBool(JsonValue* handle, bool* out_value) override;
	virtual bool GetDouble(JsonValue* handle, double* out_value) override;
	virtual bool GetInt(JsonValue* handle, int* out_value) override;
	virtual bool GetInt64(JsonValue* handle, std::variant<int64_t, uint64_t>* out_value) override;
	virtual bool GetString(JsonValue* handle, const char** out_str, size_t* out_len) override;

	// ========== Pointer Operations ==========
	virtual JsonValue* PtrGet(JsonValue* handle, const char* path, char* error, size_t error_size) override;
	virtual bool PtrGetBool(JsonValue* handle, const char* path, bool* out_value, char* error, size_t error_size) override;
	virtual bool PtrGetDouble(JsonValue* handle, const char* path, double* out_value, char* error, size_t error_size) override;
	virtual bool PtrGetInt(JsonValue* handle, const char* path, int* out_value, char* error, size_t error_size) override;
	virtual bool PtrGetInt64(JsonValue* handle, const char* path, std::variant<int64_t, uint64_t>* out_value, char* error, size_t error_size) override;
	virtual bool PtrGetString(JsonValue* handle, const char* path, const char** out_str, size_t* out_len, char* error, size_t error_size) override;
	virtual bool PtrGetIsNull(JsonValue* handle, const char* path, bool* out_is_null, char* error, size_t error_size) override;
	virtual bool PtrGetLength(JsonValue* handle, const char* path, size_t* out_len, char* error, size_t error_size) override;
	virtual bool PtrSet(JsonValue* handle, const char* path, JsonValue* value, char* error, size_t error_size) override;
	virtual bool PtrSetBool(JsonValue* handle, const char* path, bool value, char* error, size_t error_size) override;
	virtual bool PtrSetDouble(JsonValue* handle, const char* path, double value, char* error, size_t error_size) override;
	virtual bool PtrSetInt(JsonValue* handle, const char* path, int value, char* error, size_t error_size) override;
	virtual bool PtrSetInt64(JsonValue* handle, const char* path, std::variant<int64_t, uint64_t> value, char* error, size_t error_size) override;
	virtual bool PtrSetString(JsonValue* handle, const char* path, const char* value, char* error, size_t error_size) override;
	virtual bool PtrSetNull(JsonValue* handle, const char* path, char* error, size_t error_size) override;
	virtual bool PtrAdd(JsonValue* handle, const char* path, JsonValue* value, char* error, size_t error_size) override;
	virtual bool PtrAddBool(JsonValue* handle, const char* path, bool value, char* error, size_t error_size) override;
	virtual bool PtrAddDouble(JsonValue* handle, const char* path, double value, char* error, size_t error_size) override;
	virtual bool PtrAddInt(JsonValue* handle, const char* path, int value, char* error, size_t error_size) override;
	virtual bool PtrAddInt64(JsonValue* handle, const char* path, std::variant<int64_t, uint64_t> value, char* error, size_t error_size) override;
	virtual bool PtrAddString(JsonValue* handle, const char* path, const char* value, char* error, size_t error_size) override;
	virtual bool PtrAddNull(JsonValue* handle, const char* path, char* error, size_t error_size) override;
	virtual bool PtrRemove(JsonValue* handle, const char* path, char* error, size_t error_size) override;
	virtual JsonValue* PtrTryGet(JsonValue* handle, const char* path) override;
	virtual bool PtrTryGetBool(JsonValue* handle, const char* path, bool* out_value) override;
	virtual bool PtrTryGetDouble(JsonValue* handle, const char* path, double* out_value) override;
	virtual bool PtrTryGetInt(JsonValue* handle, const char* path, int* out_value) override;
	virtual bool PtrTryGetInt64(JsonValue* handle, const char* path, std::variant<int64_t, uint64_t>* out_value) override;
	virtual bool PtrTryGetString(JsonValue* handle, const char* path, const char** out_str, size_t* out_len) override;

	// ========== Iterator Operations ==========
	virtual bool ObjectForeachNext(JsonValue* handle, const char** out_key,
	                                size_t* out_key_len, JsonValue** out_value) override;
	virtual bool ArrayForeachNext(JsonValue* handle, size_t* out_index,
	                               JsonValue** out_value) override;
	virtual bool ObjectForeachKeyNext(JsonValue* handle, const char** out_key,
	                                   size_t* out_key_len) override;
	virtual bool ArrayForeachIndexNext(JsonValue* handle, size_t* out_index) override;

	// ========== Array Iterator Operations ==========
	virtual JsonArrIter* ArrIterInit(JsonValue* handle) override;
	virtual JsonArrIter* ArrIterWith(JsonValue* handle) override;
	virtual bool ArrIterReset(JsonArrIter* iter) override;
	virtual JsonValue* ArrIterNext(JsonArrIter* iter) override;
	virtual bool ArrIterHasNext(JsonArrIter* iter) override;
	virtual size_t ArrIterGetIndex(JsonArrIter* iter) override;
	virtual void* ArrIterRemove(JsonArrIter* iter) override;

	// ========== Object Iterator Operations ==========
	virtual JsonObjIter* ObjIterInit(JsonValue* handle) override;
	virtual JsonObjIter* ObjIterWith(JsonValue* handle) override;
	virtual bool ObjIterReset(JsonObjIter* iter) override;
	virtual void* ObjIterNext(JsonObjIter* iter) override;
	virtual bool ObjIterHasNext(JsonObjIter* iter) override;
	virtual JsonValue* ObjIterGetVal(JsonObjIter* iter, void* key) override;
	virtual JsonValue* ObjIterGet(JsonObjIter* iter, const char* key) override;
	virtual size_t ObjIterGetIndex(JsonObjIter* iter) override;
	virtual void* ObjIterRemove(JsonObjIter* iter) override;
	virtual bool ObjIterGetKeyString(JsonObjIter* iter, void* key, const char** out_str, size_t* out_len = nullptr) override;

	// ========== Iterator Release Operations ==========
	virtual void ReleaseArrIter(JsonArrIter* iter) override;
	virtual void ReleaseObjIter(JsonObjIter* iter) override;

	// ========== Iterator Handle Type Operations ==========
	virtual HandleType_t GetArrIterHandleType() override;
	virtual HandleType_t GetObjIterHandleType() override;
	virtual JsonArrIter* GetArrIterFromHandle(IPluginContext* pContext, Handle_t handle) override;
	virtual JsonObjIter* GetObjIterFromHandle(IPluginContext* pContext, Handle_t handle) override;

	// ========== Release Operations ==========
	virtual void Release(JsonValue* value) override;

	// ========== Handle Type Operations ==========
	virtual HandleType_t GetJsonHandleType() override;

	// ========== Handle Operations ==========
	virtual JsonValue* GetValueFromHandle(IPluginContext* pContext, Handle_t handle) override;

	// ========== Number Read/Write Operations ==========
	virtual JsonValue* ReadNumber(const char* dat, uint32_t read_flg = 0,
		char* error = nullptr, size_t error_size = 0, size_t* out_consumed = nullptr) override;
	virtual bool WriteNumber(JsonValue* handle, char* buffer, size_t buffer_size,
		size_t* out_written = nullptr) override;

	// ========== Floating-Point Format Operations ==========
	virtual bool SetFpToFloat(JsonValue* handle, bool flt) override;
	virtual bool SetFpToFixed(JsonValue* handle, int prec) override;

	// ========== Direct Value Modification Operations ==========
	virtual bool SetBool(JsonValue* handle, bool value) override;
	virtual bool SetInt(JsonValue* handle, int value) override;
	virtual bool SetInt64(JsonValue* handle, std::variant<int64_t, uint64_t> value) override;
	virtual bool SetDouble(JsonValue* handle, double value) override;
	virtual bool SetString(JsonValue* handle, const char* value) override;
	virtual bool SetNull(JsonValue* handle) override;

	virtual bool ParseInt64Variant(const char* value, std::variant<int64_t, uint64_t>* out_value,
		char* error = nullptr, size_t error_size = 0) override;

private:
	std::random_device m_randomDevice;
	std::mt19937 m_randomGenerator;

	// Helper methods
	static std::unique_ptr<JsonValue> CreateWrapper();
	static RefPtr<RefCountedMutDoc> WrapDocument(yyjson_mut_doc* doc);
	static RefPtr<RefCountedMutDoc> CopyDocument(yyjson_doc* doc);
	static RefPtr<RefCountedMutDoc> CreateDocument();
	static RefPtr<RefCountedImmutableDoc> WrapImmutableDocument(yyjson_doc* doc);
	static RefPtr<RefCountedMutDoc> CloneValueToMutable(JsonValue* value);

	// Pack helper methods
	static const char* SkipSeparators(const char* ptr);
	static yyjson_mut_val* PackImpl(yyjson_mut_doc* doc, const char* format,
	                                 IPackParamProvider* provider, char* error,
	                                 size_t error_size, const char** out_end_ptr);

	// PtrTryGet helper methods
	struct PtrGetValueResult {
		yyjson_mut_val* mut_val{ nullptr };
		yyjson_val* imm_val{ nullptr };
		bool success{ false };
	};
	static PtrGetValueResult PtrGetValueInternal(JsonValue* handle, const char* path);
};

#endif // _INCLUDE_JSONMANAGER_H_