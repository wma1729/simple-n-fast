#ifndef _SNF_DBG_H_
#define _SNF_DBG_H_

#if defined(_WIN32)
	#if defined(_DEBUG)
		#define _CRTDBG_MAP_ALLOC
		#include <cstdlib>
		#include <crtdbg.h>
		#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
	#endif // _DEBUG
#endif // _WIN32

#if !defined(DBG_NEW)
	#define DBG_NEW new
#endif // DBG_NEW

#endif // _SNF_DBG_H_
