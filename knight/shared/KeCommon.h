#ifndef _INCLUDE_KNIGHT_KE_COMMON_UTILS_H_
#define _INCLUDE_KNIGHT_KE_COMMON_UTILS_H_

#include <stddef.h>
#include <stdarg.h>

namespace Knight
{
	/**
	 * @brief Formats a buffer with C platform rules.
	 *
	 * Unlink platform snprintf, this will never return nonsense values like -1.
	 *
	 * @param buffer			Buffer to store to.
	 * @param maxlength			Maximum length of buffer (including null terminator).
	 * @param fmt				printf() format string.
	 * @param ...				Formatting arguments.
	 * @return					Number of characters written.
	 */
	extern size_t KE_PFormat(char *buffer, size_t maxlength, const char *fmt, ...);

	/**
	 * @brief Formats a buffer with C platform rules.
	 *
	 * Unlink platform snprintf, this will never return nonsense values like -1.
	 *
	 * @param buffer			Buffer to store to.
	 * @param maxlength			Maximum length of buffer (including null terminator).
	 * @param fmt				printf() format string.
	 * @param args				Formatting arguments.
	 * @return					Number of characters written.
	 */
	extern size_t KE_PFormatArgs(char *buffer, size_t maxlength, const char *fmt, va_list args);
}

#endif //_INCLUDE_KNIGHT_KE_COMMON_UTILS_H_
