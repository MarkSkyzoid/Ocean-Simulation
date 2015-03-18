#pragma once

#if defined( _DEBUG )
	#include <assert.h>
#endif

#if defined( _DEBUG )
	#define ASSERT( exp, message ) assert( exp );
#else
	#define ASSERT( exp, message )
#endif