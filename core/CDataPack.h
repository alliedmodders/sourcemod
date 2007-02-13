/**
* vim: set ts=4 :
* ===============================================================
* SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
* ===============================================================
*
* This file is not open source and may not be copied without explicit
* written permission of AlliedModders LLC.  This file may not be redistributed 
* in whole or significant part.
* For information, see LICENSE.txt or http://www.sourcemod.net/license.php
*
* Version: $Id$
*/

#ifndef _INCLUDE_SOURCEMOD_CDATAPACK_H_
#define _INCLUDE_SOURCEMOD_CDATAPACK_H_

#include <IDataPack.h>

using namespace SourceMod;

class CDataPack : public IDataPack
{
public:
	CDataPack();
	~CDataPack();
public: //IDataReader
	void Reset() const;
	size_t GetPosition() const;
	bool SetPosition(size_t pos) const;
	cell_t ReadCell() const;
	float ReadFloat() const;
	bool IsReadable(size_t bytes) const;
	const char *ReadString(size_t *len) const;
	void *GetMemory() const;
public: //IDataPack
	void ResetSize();
	void PackCell(cell_t cell);
	void PackFloat(float val);
	void PackString(const char *string);
	size_t CreateMemory(size_t size, void **addr);
public:
	void Initialize();
private:
	void CheckSize(size_t sizetype);
private:
	char *m_pBase;
	mutable char *m_curptr;
	size_t m_capacity;
	size_t m_size;
};

#endif //_INCLUDE_SOURCEMOD_CDATAPACK_H_
