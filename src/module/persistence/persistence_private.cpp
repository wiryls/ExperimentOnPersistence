/****************************************************************************
 *  license
 ***************************************************************************/

#include <exception>
#include <cstdio>
#include "persistence_private.hpp"

/****************************************************************************
 *  exception
 ***************************************************************************/
CV_FS_PRIVATE_BEGIN
namespace exception
{
    /************************************************************************
     * TODO: replace it
     ***********************************************************************/
	void error(int code, char const * err, POS_TYPE_)
	{
		char buf[1024];
        ::sprintf(
			buf,
			"Error: code=%d (%s)\n\nin:\n func: %s,\n file: %s,\n line: %d.",
            code,
			(err  ? err  : "unknown"),
			(func ? func : "unknown function"),
			file,
			line
		);
        ::fprintf(stderr, "%s\n", buf);
        ::fflush(stderr);
		::std::terminate();
	}
}
CV_FS_PRIVATE_END
