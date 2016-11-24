#include <anthem/output/Logger.h>

#include <anthem/output/Formatting.h>

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
			return {Color::Cyan, FontWeight::Bold};
		case Priority::Warning:
			return {Color::Magenta, FontWeight::Bold};
		case Priority::Error:
			return {Color::Red, FontWeight::Bold};
	}

	return {Color::White, FontWeight::Bold};
}

////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr const char *priorityName(Priority priority)
{
	switch (priority)
	{
		case Priority::Debug:
			return "debug";
		case Priority::Info:
			return "info";
		case Priority::Warning:
			return "warning";
		case Priority::Error:
			return "error";
	}

	return "message";
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
	m_outputPriority{Priority::Warning}
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

void Logger::setOutputPriority(Priority outputPriority)
{
	m_outputPriority = outputPriority;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::setColorPolicy(ColorStream::ColorPolicy colorPolicy)
{
	m_outputStream.setColorPolicy(colorPolicy);
	m_errorStream.setColorPolicy(colorPolicy);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::log(Priority priority, const char *message)
{
	const auto priorityID = static_cast<int>(priority);

	if (priorityID < static_cast<int>(m_outputPriority))
		return;

	m_outputStream
		<< priorityFormat(priority) << priorityName(priority) << ":"
		<< ResetFormat() << " "
		<< MessageBodyFormat << message
		<< ResetFormat() << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::log(Priority priority, const input::Location &location, const char *message)
{
	const auto priorityID = static_cast<int>(priority);

	if (priorityID < static_cast<int>(m_outputPriority))
		return;

	auto &stream =
		(priorityID > static_cast<int>(Priority::Warning))
		? m_errorStream
		: m_outputStream;

	stream
		<< LocationFormat
		<< location.sectionStart << ":" << location.rowStart << ":" << location.columnStart << ": "
		<< priorityFormat(priority) << priorityName(priority) << ":"
		<< ResetFormat() << " "
		<< MessageBodyFormat << message
		<< ResetFormat() << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
