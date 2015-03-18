#include "BasicIO.h"

namespace acqua
{
	std::string StringFromFile( const std::string& file_name )
	{
		std::ifstream in(file_name);
		std::string s((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		in.close();
		return s;
	}
}

