#ifndef __ANTHEM__EXCEPTION_H
#define __ANTHEM__EXCEPTION_H

#include <exception>
#include <string>

#include <anthem/Location.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Exception
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: implement stream operators to modify message
class Exception: public std::exception
{
	public:
		explicit Exception()
		:	Exception("unspecified parser error")
		{
		}

		explicit Exception(const char *message)
		:	Exception(static_cast<std::string>(message))
		{
		}

		explicit Exception(const std::string &message)
		:	m_message{message},
			m_plainMessage{message}
		{
		}

		explicit Exception(const Location &location)
		:	Exception(location, "unspecified error")
		{
		}

		explicit Exception(const Location &location, const char *message)
		:	Exception(location, static_cast<std::string>(message))
		{
		}

		explicit Exception(const Location &location, const std::string &message)
		:	m_location{location},
			m_message{message},
			// TODO: refactor
			m_plainMessage{std::string(m_location.sectionStart) + ":" + std::to_string(m_location.rowStart)
				+ ":" + std::to_string(m_location.columnStart) + " " + m_message}
		{
		}

		Exception(const Exception &other) = delete;
		Exception &operator=(const Exception &other) = delete;
		Exception(Exception &&other) = default;
		Exception &operator=(Exception &&other) = default;

		~Exception() noexcept = default;

		const char *what() const noexcept
		{
			return m_plainMessage.c_str();
		}

		const Location &location() const
		{
			return m_location;
		}

		const std::string &message() const
		{
			return m_message;
		}

	private:
		Location m_location;
		std::string m_message;
		std::string m_plainMessage;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

class LogicException : public Exception
{
	using Exception::Exception;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

class TranslationException : public Exception
{
	using Exception::Exception;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

class SimplificationException : public Exception
{
	using Exception::Exception;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

class CompletionException : public Exception
{
	using Exception::Exception;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
