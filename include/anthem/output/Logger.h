#ifndef __ANTHEM__OUTPUT__LOGGER_H
#define __ANTHEM__OUTPUT__LOGGER_H

#include <string>

#include <anthem/input/Location.h>
#include <anthem/output/ColorStream.h>
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
		Logger();

		ColorStream &outputStream();
		ColorStream &errorStream();

		// The level from which on messages should be printed
		void setOutputPriority(Priority outputLevel);
		void setColorPolicy(ColorStream::ColorPolicy colorPolicy);

		void log(Priority priority, const char *message);
		void log(Priority priority, const input::Location &location, const char *message);

	private:
		ColorStream m_outputStream;
		ColorStream m_errorStream;

		Priority m_outputPriority;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
