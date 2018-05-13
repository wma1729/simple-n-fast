#ifndef _SNF_DBG_H_
#define _SNF_DBG_H_

#if defined(_WIN32)
	#if defined(_DEBUG)
		#define _CRTDBG_MAP_ALLOC
		#include <cstdlib>
		#include <crtdbg.h>
		#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
		#define ENABLE_LEAK_CHECK \
			do {
				_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
			} while (0)
	#endif // _DEBUG
#endif // _WIN32

#if !defined(DBG_NEW)
	#define DBG_NEW new
#endif // DBG_NEW

#if !defined(ENABLE_LEAK_CHECK)
	#define ENABLE_LEAK_CHECK do { ; } while (0)
#endif

#endif // _SNF_DBG_H_
