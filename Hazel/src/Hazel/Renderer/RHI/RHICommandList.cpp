#include "hzpch.h"
#include "RHICommandList.h"
#include "RHI.h"
namespace GameEngine {
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

	void RHICommandList::Execute(RHIFenceRef fence, RHISemaphoreRef waitSemaphore, RHISemaphoreRef signalSemaphore)
	{
		if (!info.byPass)
		{
			// LOG_DEBUG("Recording command list in delay mode.");
			for (int32_t i = 0; i < commands.size(); i++)
			{
				commands[i]->Execute(info.context);
				delete commands[i];
			}
			commands.clear();
		}

		info.context->Execute(fence, waitSemaphore, signalSemaphore);
	}

	void RHICommandImmediateGenerateMips::Execute(RHICommandContextImmediateRef context)
	{
		context->GenerateMips(src);
	}

	void RHICommandListImmediate::GenerateMips(RHITextureRef src)
	{
		ADD_COMMAND_IMMEDIATE(GenerateMips, src);
	}

	void RHICommandListImmediate::TextureBarrier(const RHITextureBarrier& barrier)
	{
		ADD_COMMAND_IMMEDIATE(TextureBarrier, barrier);
	}

	void RHICommandListImmediate::Flush()
	{
		// LOG_DEBUG("RHICommandListImmediate Flushed.");
		for (int32_t i = 0; i < commands.size(); i++)
		{
			commands[i]->Execute(info.context);
			delete commands[i];
		}
		commands.clear();

		info.context->Flush();
	}

	void RHICommandImmediateTextureBarrier::Execute(RHICommandContextImmediateRef context)
	{
		context->TextureBarrier(barrier);
	}

}
