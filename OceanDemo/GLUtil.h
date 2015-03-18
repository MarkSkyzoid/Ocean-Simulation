#pragma once

void CheckOpenGLError(const char* stmt, const char* fname, int line);

#ifdef _DEBUG
	#define GL_CHECK(stmt) do { \
			stmt; \
			CheckOpenGLError(#stmt, __FILE__, __LINE__); \
		} while(0)
#else
	#define GL_CHECK(stmt) stmt
#endif