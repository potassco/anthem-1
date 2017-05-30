#include <anthem/output/NullStream.h>

namespace anthem
{
namespace output
{
namespace detail
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// NullStream
//
////////////////////////////////////////////////////////////////////////////////////////////////////

NullBuffer nullBuffer;
std::ostream nullOStream(&nullBuffer);
ColorStream nullStream(nullOStream);

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
}
