#include <anthem/output/Logger.h>

#include <anthem/output/Formatting.h>
#include <anthem/output/NullStream.h>

namespace anthem
{
namespace output
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Logger
//
////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr Format priorityFormat(Priority priority)
{
	switch (priority)
	{
		case Priority::Debug:
			return {Color::Green, FontWeight::Bold};
		case Priority::Info:
			return {Color::Blue, FontWeight::Bold};
		case Priority::Warning:
			return {Color::Magenta, FontWeight::Bold};
		case Priority::Error:
			return {Color::Red, FontWeight::Bold};
	}

	return {Color::White, FontWeight::Bold};
}

////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr const Format MessageBodyFormat = {Color::White, FontWeight::Bold};

////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr const Format LocationFormat = {Color::White, FontWeight::Bold};

////////////////////////////////////////////////////////////////////////////////////////////////////

Logger::Logger()
:	Logger(std::cout, std::cerr)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Logger::Logger(ColorStream &&outputStream)
:	Logger(std::forward<ColorStream &&>(outputStream), std::cerr)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Logger::Logger(ColorStream &&outputStream, ColorStream &&errorStream)
:	m_outputStream{outputStream},
	m_errorStream{errorStream},
	m_logPriority{Priority::Warning}
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ColorStream &Logger::outputStream()
{
	return m_outputStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ColorStream &Logger::errorStream()
{
	return m_errorStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::setLogPriority(Priority logPriority)
{
	m_logPriority = logPriority;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::setColorPolicy(ColorStream::ColorPolicy colorPolicy)
{
	m_outputStream.setColorPolicy(colorPolicy);
	m_errorStream.setColorPolicy(colorPolicy);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

FormatScope Logger::log(Priority priority)
{
	const auto priorityID = static_cast<int>(priority);

	if (priorityID < static_cast<int>(m_logPriority))
		return FormatScope(detail::nullStream);

	m_errorStream
		<< priorityFormat(priority) << priorityName(priority) << ":"
		<< ResetFormat() << " "
		<< MessageBodyFormat;

	return FormatScope(m_errorStream);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

FormatScope Logger::log(Priority priority, const Location &location)
{
	const auto priorityID = static_cast<int>(priority);

	if (priorityID < static_cast<int>(m_logPriority))
		return FormatScope(detail::nullStream);

	m_errorStream
		<< LocationFormat
		<< location.sectionStart << ":" << location.rowStart << ":" << location.columnStart << ":"
		<< ResetFormat() << " "
		<< priorityFormat(priority) << priorityName(priority) << ":"
		<< ResetFormat() << " "
		<< MessageBodyFormat;

	return FormatScope(m_errorStream);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
