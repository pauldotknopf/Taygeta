#pragma once

using namespace System;

namespace Taygeta { namespace Core 
{
	public ref class ShaderCompilationException : public Exception
	{
	public:
		ShaderCompilationException(String^ message)
			: Exception(message)
		{

		}
	};
}
}

