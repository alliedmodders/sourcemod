/**
* =============================================================================
* DynamicHooks
* Copyright (C) 2015 Robin Gohmert. All rights reserved.
* Copyright (C) 2018-2021 AlliedModders LLC.  All rights reserved.
* =============================================================================
*
* This software is provided 'as-is', without any express or implied warranty.
* In no event will the authors be held liable for any damages arising from 
* the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose, 
* including commercial applications, and to alter it and redistribute it 
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not 
* claim that you wrote the original software. If you use this software in a 
* product, an acknowledgment in the product documentation would be 
* appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
*
* asm.h/cpp from devmaster.net (thanks cybermind) edited by pRED* to handle gcc
* -fPIC thunks correctly
*
* Idea and trampoline code taken from DynDetours (thanks your-name-here).
*
* Adopted to provide similar features to SourceHook by AlliedModders LLC.
*/

#ifndef _CONVENTION_H
#define _CONVENTION_H

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "registers.h"
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
#include <utility>

// ============================================================================
// >> DataType_t
// ============================================================================
enum DataType_t
{
	DATA_TYPE_VOID,
	DATA_TYPE_BOOL,
	DATA_TYPE_CHAR,
	DATA_TYPE_UCHAR,
	DATA_TYPE_SHORT,
	DATA_TYPE_USHORT,
	DATA_TYPE_INT,
	DATA_TYPE_UINT,
	DATA_TYPE_LONG,
	DATA_TYPE_ULONG,
	DATA_TYPE_LONG_LONG,
	DATA_TYPE_ULONG_LONG,
	DATA_TYPE_FLOAT,
	DATA_TYPE_DOUBLE,
	DATA_TYPE_POINTER,
	DATA_TYPE_STRING,
	DATA_TYPE_OBJECT
};

typedef struct DataTypeSized_s {
	DataTypeSized_s()
	{
		type = DATA_TYPE_POINTER;
		size = 0;
		custom_register = None;
	}
	DataType_t type;
	size_t size;
	Register_t custom_register;
} DataTypeSized_t;


// ============================================================================
// >> FUNCTIONS
// ============================================================================
/*
Returns the size after applying alignment.

@param <size>:
The size that should be aligned.

@param <alignment>:
The alignment that should be used.
*/
inline int Align(int size, int alignment)
{    
	int unaligned = size % alignment;
	if (unaligned == 0)
		return size;

	return size + (alignment - unaligned);
}

/*
Returns the size of a data type after applying alignment.

@param <type>:
The data type you would like to get the size of.

@param <alignment>:
The alignment that should be used.
*/
inline int GetDataTypeSize(DataTypeSized_t type, int iAlignment=4)
{
	switch(type.type)
	{
		case DATA_TYPE_VOID:		return 0;
		case DATA_TYPE_BOOL:		return Align(sizeof(bool),					iAlignment);
		case DATA_TYPE_CHAR:		return Align(sizeof(char),					iAlignment);
		case DATA_TYPE_UCHAR:		return Align(sizeof(unsigned char),			iAlignment);
		case DATA_TYPE_SHORT:		return Align(sizeof(short),					iAlignment);
		case DATA_TYPE_USHORT:		return Align(sizeof(unsigned short),		iAlignment);
		case DATA_TYPE_INT:			return Align(sizeof(int),					iAlignment);
		case DATA_TYPE_UINT:		return Align(sizeof(unsigned int),			iAlignment);
		case DATA_TYPE_LONG:		return Align(sizeof(long),					iAlignment);
		case DATA_TYPE_ULONG:		return Align(sizeof(unsigned long),			iAlignment);
		case DATA_TYPE_LONG_LONG:	return Align(sizeof(long long),				iAlignment);
		case DATA_TYPE_ULONG_LONG:	return Align(sizeof(unsigned long long),	iAlignment);
		case DATA_TYPE_FLOAT:		return Align(sizeof(float),					iAlignment);
		case DATA_TYPE_DOUBLE:		return Align(sizeof(double),				iAlignment);
		case DATA_TYPE_POINTER:		return Align(sizeof(void *),				iAlignment);
		case DATA_TYPE_STRING:		return Align(sizeof(char *),				iAlignment);
		case DATA_TYPE_OBJECT:		return type.size;
		default: puts("Unknown data type.");
	}
	return 0;
}

// ============================================================================
// >> CLASSES
// ============================================================================
/*
This is the base class for every calling convention. Inherit from this class
to create your own calling convention.
*/
class ICallingConvention
{
public:
	/*
	Initializes the calling convention.

	@param <vecArgTypes>:
	A list of DataType_t objects, which define the arguments of the function.

	@param <returnType>:
	The return type of the function.
	*/
	ICallingConvention(std::vector<DataTypeSized_t> &vecArgTypes, DataTypeSized_t returnType, int iAlignment=4)
	{
		m_vecArgTypes = std::move(vecArgTypes);
		
		for (size_t i=0; i < m_vecArgTypes.size(); i++)
		{
			DataTypeSized_t &type = m_vecArgTypes[i];
			if (!type.size)
				type.size = GetDataTypeSize(type, iAlignment);
		}
		m_returnType = returnType;
		if (!m_returnType.size)
			m_returnType.size = GetDataTypeSize(m_returnType, iAlignment);
		m_iAlignment = iAlignment;
	}

	virtual ~ICallingConvention()
	{
	}

	/*
	This should return a list of Register_t values. These registers will be
	saved for later access.
	*/
	virtual std::vector<Register_t> GetRegisters() = 0;

	/*
	Returns the number of bytes that should be added to the stack to clean up.
	*/
	virtual int GetPopSize() = 0;

	virtual int GetArgStackSize() = 0;
	virtual void** GetStackArgumentPtr(CRegisters* pRegisters) = 0;

	/*
	Returns the number of bytes for the buffer to store all the arguments that are passed in a register in.
	*/
	virtual int GetArgRegisterSize() = 0;

	/*
	Returns a pointer to the argument at the given index.

	@param <iIndex>:
	The index of the argument.

	@param <pRegisters>:
	A snapshot of all saved registers.
	*/
	virtual void* GetArgumentPtr(unsigned int iIndex, CRegisters* pRegisters) = 0;

	/*
	*/
	virtual void ArgumentPtrChanged(unsigned int iIndex, CRegisters* pRegisters, void* pArgumentPtr) = 0;

	/*
	Returns a pointer to the return value.

	@param <pRegisters>:
	A snapshot of all saved registers.
	*/
	virtual void* GetReturnPtr(CRegisters* pRegisters) = 0;

	/*
	*/
	virtual void ReturnPtrChanged(CRegisters* pRegisters, void* pReturnPtr) = 0;

	/*
	Save the return value in a seperate buffer, so we can restore it after calling the original function.

	@param <pRegisters>:
	A snapshot of all saved registers.
	*/
	virtual void SaveReturnValue(CRegisters* pRegisters)
	{
		std::unique_ptr<uint8_t[]> pSavedReturnValue = std::make_unique<uint8_t[]>(m_returnType.size);
		memcpy(pSavedReturnValue.get(), GetReturnPtr(pRegisters), m_returnType.size);
		m_pSavedReturnBuffers.push_back(std::move(pSavedReturnValue));
	}

	virtual void RestoreReturnValue(CRegisters* pRegisters)
	{
		uint8_t* pSavedReturnValue = m_pSavedReturnBuffers.back().get();
		memcpy(GetReturnPtr(pRegisters), pSavedReturnValue, m_returnType.size);
		ReturnPtrChanged(pRegisters, pSavedReturnValue);
		m_pSavedReturnBuffers.pop_back();
	}

	/*
	Save the value of arguments in a seperate buffer for the post callback.
	Compiler optimizations might cause the registers or stack space to be reused
	and overwritten during function execution if the value isn't needed anymore
	at some point. This leads to different values in the post hook.

	@param <pRegisters>:
	A snapshot of all saved registers.
	*/
	virtual void SaveCallArguments(CRegisters* pRegisters)
	{
		int size = GetArgStackSize() + GetArgRegisterSize();
		std::unique_ptr<uint8_t[]> pSavedCallArguments = std::make_unique<uint8_t[]>(size);
		size_t offset = 0;
		for (size_t i = 0; i < m_vecArgTypes.size(); i++) {
			DataTypeSized_t &type = m_vecArgTypes[i];
			memcpy((void *)((unsigned long)pSavedCallArguments.get() + offset), GetArgumentPtr(i, pRegisters), type.size);
			offset += type.size;
		}
		m_pSavedCallArguments.push_back(std::move(pSavedCallArguments));
	}

	virtual void RestoreCallArguments(CRegisters* pRegisters)
	{
		uint8_t *pSavedCallArguments = m_pSavedCallArguments.back().get();
		size_t offset = 0;
		for (size_t i = 0; i < m_vecArgTypes.size(); i++) {
			DataTypeSized_t &type = m_vecArgTypes[i];
			memcpy(GetArgumentPtr(i, pRegisters), (void *)((unsigned long)pSavedCallArguments + offset), type.size);
			offset += type.size;
		}
		m_pSavedCallArguments.pop_back();
	}

public:
	std::vector<DataTypeSized_t> m_vecArgTypes;
	DataTypeSized_t m_returnType;
	int m_iAlignment;
	// Save the return in case we call the original function and want to override the return again.
	std::vector<std::unique_ptr<uint8_t[]>> m_pSavedReturnBuffers;
	// Save call arguments in case the function reuses the space and overwrites the values for the post hook.
	std::vector<std::unique_ptr<uint8_t[]>> m_pSavedCallArguments;
};

#endif // _CONVENTION_H