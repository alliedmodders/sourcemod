#ifndef _INCLUDE_KNIGHT_ERRORMANAGER_H_
#define _INCLUDE_KNIGHT_ERRORMANAGER_H_

#include "Lexer.h"

namespace Knight
{
	enum ErrorMessage
	{
		ErrorMessage_UnexpectedToken = 0,
		ErrorMessages_Total
	};

	enum FatalMessage
	{
		FatalMessage_Assert = 0,
		FatalMessages_Total
	};

	class ErrorManager
	{
	public:
		ErrorManager();
	public:
		void AddError(ErrorMessage err, unsigned int line, ...);
		void ThrowFatal(FatalMessage err, ...);
	private:
		unsigned int m_NumErrors;
	};
}

#endif //_INCLUDE_KNIGHT_ERRORMANAGER_H_
