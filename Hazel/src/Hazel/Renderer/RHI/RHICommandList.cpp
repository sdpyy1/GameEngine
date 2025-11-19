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

	void RHICommandList::BufferBarrier(const RHIBufferBarrier& barrier)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->BufferBarrier(barrier);
		else ADD_COMMAND(BufferBarrier, barrier);
	}

	void RHICommandList::BeginRenderPass(RHIRenderPassRef renderPass)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->BeginRenderPass(renderPass);
		else ADD_COMMAND(BeginRenderPass, renderPass);
	}

	void RHICommandList::EndRenderPass()
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->EndRenderPass();
		else ADD_COMMAND(EndRenderPass);
	}

	void RHICommandList::CopyTexture(RHITextureRef src, TextureSubresourceLayers srcSubresource, RHITextureRef dst, TextureSubresourceLayers dstSubresource)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->CopyTexture(src, srcSubresource, dst, dstSubresource);
		else ADD_COMMAND(CopyTexture, src, srcSubresource, dst, dstSubresource);
	}

	void RHICommandList::GenerateMips(RHITextureRef src)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->GenerateMips(src);
		else ADD_COMMAND(GenerateMips, src);
	}

	void RHICommandList::TextureBarrier(const RHITextureBarrier& barrier)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->TextureBarrier(barrier);
		else ADD_COMMAND(TextureBarrier, barrier);
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

	void RHICommandBufferBarrier::Execute(RHICommandContextRef context)
	{
		context->BufferBarrier(barrier);
	}

	void RHICommandGenerateMips::Execute(RHICommandContextRef context)
	{
		context->GenerateMips(src);
	}

	void RHICommandBeginRenderPass::Execute(RHICommandContextRef context)
	{
		context->BeginRenderPass(renderPass);
	}

	void RHICommandEndRenderPass::Execute(RHICommandContextRef context)
	{
		context->EndRenderPass();
	}

	void RHICommandCopyTexture::Execute(RHICommandContextRef context)
	{
		context->CopyTexture(src, srcSubresource, dst, dstSubresource);
	}

	void RHICommandTextureBarrier::Execute(RHICommandContextRef context)
	{
		context->TextureBarrier(barrier);
	}

}
