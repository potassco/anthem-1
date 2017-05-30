#ifndef __ANTHEM__OUTPUT__FORMAT_SCOPE_H
#define __ANTHEM__OUTPUT__FORMAT_SCOPE_H

#include <anthem/output/ColorStream.h>
#include <anthem/output/Formatting.h>

namespace anthem
{
namespace output
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FormatScope
//
////////////////////////////////////////////////////////////////////////////////////////////////////

class FormatScope
{
	public:
		explicit FormatScope(ColorStream &colorStream)
		:	m_colorStream{colorStream}
		{
		}

		~FormatScope()
		{
			m_colorStream << output::ResetFormat() << std::endl;
		}

		template<class T>
		inline FormatScope &operator<<(T &&value)
		{
			m_colorStream << std::forward<T>(value);

			return *this;
		}

	private:
		ColorStream &m_colorStream;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
