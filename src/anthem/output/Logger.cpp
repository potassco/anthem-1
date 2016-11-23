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
		case Priority::Note:
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
		case Priority::Note:
			return "note";
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
:	m_outputStream{std::cout},
	m_errorStream{std::cerr},
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

	auto &stream =
		(priorityID > static_cast<int>(Priority::Warning))
		? m_errorStream
		: m_outputStream;

	stream
		<< priorityFormat(priority) << priorityName(priority) << ":"
		<< ResetFormat() << " "
		<< MessageBodyFormat << message
		<< ResetFormat() << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::log(Priority priority, const input::Location &location, const char *message)
{
	const auto priorityID = static_cast<int>(priority);

	auto &stream =
		(priorityID > static_cast<int>(Priority::Warning))
		? m_errorStream
		: m_outputStream;

	stream
		<< LocationFormat
		<< location.sectionStart << ":" << location.rowStart << ":" << location.columnStart << ":"
		<< priorityFormat(priority) << priorityName(priority) << ":"
		<< ResetFormat() << " "
		<< MessageBodyFormat << message
		<< ResetFormat() << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
