#include "hzpch.h"
#include "RHICommandList.h"
#include "RHI.h"
namespace Hazel {
	void RHICommandBeginCommand::Execute(RHICommandContextRef context)
	{
		context->BeginCommand();
	}

	void RHICommandEndCommand::Execute(RHICommandContextRef context)
	{
		context->EndCommand();
	}

	void RHICommandList::BeginCommand()
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->BeginCommand();
		else ADD_COMMAND(BeginCommand);
	}

	void RHICommandList::EndCommand()
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->EndCommand();
		else ADD_COMMAND(EndCommand);
	}

	void RHICommandImmediateGenerateMips::Execute(RHICommandContextImmediateRef context)
	{
		context->GenerateMips(src);
	}

	void RHICommandListImmediate::GenerateMips(RHITextureRef src)
	{
		ADD_COMMAND_IMMEDIATE(GenerateMips, src);

	}

}
