#ifndef __ANTHEM__OUTPUT__LOGGER_H
#define __ANTHEM__OUTPUT__LOGGER_H

#include <string>

#include <anthem/input/Location.h>
#include <anthem/output/ColorStream.h>
#include <anthem/output/FormatScope.h>
#include <anthem/output/Priority.h>

namespace anthem
{
namespace output
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Logger
//
////////////////////////////////////////////////////////////////////////////////////////////////////

class Logger
{
	public:
		explicit Logger();
		explicit Logger(ColorStream &&outputStream);
		explicit Logger(ColorStream &&outputStream, ColorStream &&errorStream);

		ColorStream &outputStream();
		ColorStream &errorStream();

		// The level from which on messages should be printed
		void setLogPriority(Priority logPriority);
		void setColorPolicy(ColorStream::ColorPolicy colorPolicy);

		FormatScope log(Priority priority);
		FormatScope log(Priority priority, const input::Location &location);

	private:
		ColorStream m_outputStream;
		ColorStream m_errorStream;

		Priority m_logPriority;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
