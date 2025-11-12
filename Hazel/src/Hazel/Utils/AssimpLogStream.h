#pragma once

#include "Hazel/Core/Log.h"

#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>

namespace Hazel {
	struct AssimpLogStream : public Assimp::LogStream
	{
		static void Initialize()
		{
			if (Assimp::DefaultLogger::isNullLogger())
			{
				Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
				Assimp::DefaultLogger::get()->attachStream(new AssimpLogStream, /*Assimp::Logger::Debugging | Assimp::Logger::info |*/ Assimp::Logger::Warn | Assimp::Logger::Err);
			}
		}

		virtual void write(const char* message) override
		{
			std::string msg(message);
			if (!msg.empty() && msg[msg.length() - 1] == '\n')
			{
				msg.erase(msg.length() - 1);
			}
			if (strncmp(message, "Debug", 5) == 0)
			{
				LOG_TRACE_TAG("Assimp", msg);
			}
			else if (strncmp(message, "info", 4) == 0)
			{
				LOG_INFO_TAG("Assimp", msg);
			}
			else if (strncmp(message, "warn", 4) == 0)
			{
				LOG_WARN_TAG("Assimp", msg);
			}
			else
			{
				LOG_ERROR_TAG("Assimp", msg);
			}
		}
	};
}
