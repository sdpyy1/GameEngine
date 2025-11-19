#include "hzpch.h"
#include "RDGBuilder.h"
#include "Hazel/Renderer/RHI/RHIResource.h"
#include "RDGPool.h"
#include "Hazel/Core/Application.h"
namespace GameEngine { 

    RDGPassNodeRef RDGBlackBoard::Pass(std::string name)
    {
        auto found = passes.find(name);
        if (found != passes.end()) {
            return found->second;
        }
        return nullptr;
    }

    RDGBufferNodeRef RDGBlackBoard::Buffer(std::string name)
    {
        auto found = buffers.find(name);
        if (found != buffers.end()) {
            return found->second;
        }
        return nullptr;
    }

    RDGTextureNodeRef RDGBlackBoard::Texture(std::string name)
    {
        auto found = textures.find(name);
        if (found != textures.end()) {
            return found->second;
        }
        return nullptr;
    }

    void RDGBlackBoard::AddPass(RDGPassNodeRef pass)
    {
        passes[pass->Name()] = pass;
    }

    void RDGBlackBoard::AddBuffer(RDGBufferNodeRef buffer)
    {
        buffers[buffer->Name()] = buffer;
    }

    void RDGBlackBoard::AddTexture(RDGTextureNodeRef texture)
    {
        textures[texture->Name()] = texture;
    }

    RDGTextureBuilder RDGBuilder::CreateTexture(std::string name)
    {
        RDGTextureNodeRef textureNode = graph->CreateNode<RDGTextureNode>(name);
        blackBoard.AddTexture(textureNode);
        return RDGTextureBuilder(this, textureNode);
    }

    RDGBufferBuilder RDGBuilder::CreateBuffer(std::string name)
    {
        RDGBufferNodeRef bufferNode = graph->CreateNode<RDGBufferNode>(name);
        blackBoard.AddBuffer(bufferNode);
        return RDGBufferBuilder(this, bufferNode);
    }

    RDGRenderPassBuilder RDGBuilder::CreateRenderPass(std::string name)
    {
        RDGRenderPassNodeRef passNode = graph->CreateNode<RDGRenderPassNode>(name);
        blackBoard.AddPass(passNode);
        passes.push_back(passNode);
        return RDGRenderPassBuilder(this, passNode);
    }

    RDGComputePassBuilder RDGBuilder::CreateComputePass(std::string name)
    {
        RDGComputePassNodeRef passNode = graph->CreateNode<RDGComputePassNode>(name);
        blackBoard.AddPass(passNode);
        passes.push_back(passNode);
        return RDGComputePassBuilder(this, passNode);
    }

    RDGRayTracingPassBuilder RDGBuilder::CreateRayTracingPass(std::string name)
    {
        RDGRayTracingPassNodeRef passNode = graph->CreateNode<RDGRayTracingPassNode>(name);
        blackBoard.AddPass(passNode);
        passes.push_back(passNode);
        return RDGRayTracingPassBuilder(this, passNode);
    }

    RDGPresentPassBuilder RDGBuilder::CreatePresentPass(std::string name)
    {
        RDGPresentPassNodeRef passNode = graph->CreateNode<RDGPresentPassNode>(name);
        blackBoard.AddPass(passNode);
        passes.push_back(passNode);
        return RDGPresentPassBuilder(this, passNode);
    }

    RDGCopyPassBuilder RDGBuilder::CreateCopyPass(std::string name)
    {
        RDGCopyPassNodeRef passNode = graph->CreateNode<RDGCopyPassNode>(name);
        blackBoard.AddPass(passNode);
        passes.push_back(passNode);
        return RDGCopyPassBuilder(this, passNode);
    }

    RDGTextureHandle RDGBuilder::GetTexture(std::string name)
    {
        auto node = blackBoard.Texture(name);
        if (node == nullptr)
        {
            LOG_ERROR("Unable to find RDG resource [{}], please check name!", name.c_str());
            return RDGTextureHandle(UINT32_MAX);
        }
        return node->GetHandle();
    }

    RDGBufferHandle RDGBuilder::GetBuffer(std::string name)
    {
        auto node = blackBoard.Buffer(name);
        if (node == nullptr)
        {
            LOG_ERROR("Unable to find RDG resource [{}], please check name!", name.c_str());
            return RDGBufferHandle(UINT32_MAX);
        }
        return node->GetHandle();
    }

    void RDGBuilder::Execute()
    {
        // TODO 还没做剔除
        for (auto& pass : passes)
        {
            if (pass->isCulled || !pass) continue;
            // printf("rdg executing pass: %s\n", pass->Name().c_str());

            switch (pass->NodeType()) {
            case RDG_PASS_NODE_TYPE_RENDER:         ExecutePass(dynamic_cast<RDGRenderPassNodeRef>(pass));          break;
            case RDG_PASS_NODE_TYPE_COMPUTE:        ExecutePass(dynamic_cast<RDGComputePassNodeRef>(pass));         break;
            case RDG_PASS_NODE_TYPE_RAY_TRACING:    ExecutePass(dynamic_cast<RDGRayTracingPassNodeRef>(pass));      break;
            case RDG_PASS_NODE_TYPE_PRESENT:        ExecutePass(dynamic_cast<RDGPresentPassNodeRef>(pass));         break;
            case RDG_PASS_NODE_TYPE_COPY:           ExecutePass(dynamic_cast<RDGCopyPassNodeRef>(pass));            break;
            default:                                LOG_ERROR("Unsupported RDG pass type!");
            }
        }

        for (auto& pass : passes)   // 释放池化资源
        {
            //ReleaseResource(pass);
            for (auto& descriptor : pass->pooledDescriptorSets)  // 池化的view在pass结束后就可以释放，但是描述符得全部执行完再释放？
            {
                RDGDescriptorSetPool::Get(APP_FRAMEINDEX)->Release({ descriptor.first }, pass->rootSignature, descriptor.second);
            }
        }
    }

    void RDGBuilder::CreateInputBarriers(RDGPassNodeRef pass)
    {
        pass->ForEachTexture([&](RDGTextureEdgeRef edge, RDGTextureNodeRef texture) {

            if (edge->IsOutput()) return;
            RHIResourceState previousState = PreviousState(texture, pass, edge->subresource, false);
            //if(previousState != edge->state)  // 状态一样也加屏障？ 比如连续两个UAV读写的情况？
            {
                RHITextureBarrier barrier = {
                    Resolve(texture),        // 第一个成员: texture
                    previousState,           // 第二个成员: srcState
                    edge->state,             // 第三个成员: dstState
                    edge->subresource        // 第四个成员: subresource
                };
                command->TextureBarrier(barrier);

                // printf("rdg resource %lld, raw: %s barrier: %d to %d\n", (int64_t)texture->texture.get(), ToHex((uint64_t)texture->texture->RawHandle(), false).c_str(), (uint32_t)previousState, (uint32_t)edge->state);
            }
            });

        pass->ForEachBuffer([&](RDGBufferEdgeRef edge, RDGBufferNodeRef buffer) {

            if (edge->IsOutput()) return;
            RHIResourceState previousState = PreviousState(buffer, pass, false);
            //if(previousState != edge->state)  // 状态一样也加屏障？ 比如连续两个UAV读写的情况？
            {
                RHIBufferBarrier barrier = {
                    Resolve(buffer),  // 第一个成员: buffer
                    previousState,    // 第二个成员: srcState
                    edge->state,      // 第三个成员: dstState
                    edge->offset,     // 第四个成员: offset
                    edge->size        // 第五个成员: size
                };
                command->BufferBarrier(barrier);

                // printf("rdg resource %lld, raw: %s barrier: %d to %d\n", (int64_t)buffer->buffer.get(), ToHex((uint64_t)buffer->buffer->RawHandle(), false).c_str(), (uint32_t)previousState, (uint32_t)edge->state);
            }
            });
    }

    void RDGBuilder::CreateOutputBarriers(RDGPassNodeRef pass)
    {
        pass->ForEachTexture([&](RDGTextureEdgeRef edge, RDGTextureNodeRef texture) {

            if (!edge->IsOutput()) return;
            RHIResourceState previousState = PreviousState(texture, pass, edge->subresource, true);
            //if(previousState != edge->state)  // 状态一样也加屏障？ 比如连续两个UAV读写的情况？
            {
                RHITextureBarrier barrier = {
                    Resolve(texture),   // texture
                    previousState,      // srcState
                    edge->state,        // dstState
                    edge->subresource   // subresource
                };
                command->TextureBarrier(barrier);

                // printf("rdg resource %lld, raw: %s barrier: %d to %d\n", (int64_t)texture->texture.get(), ToHex((uint64_t)texture->texture->RawHandle(), false).c_str(), (uint32_t)previousState, (uint32_t)edge->state);
            }
            });

        pass->ForEachBuffer([&](RDGBufferEdgeRef edge, RDGBufferNodeRef buffer) {

            if (!edge->IsOutput()) return;
            RHIResourceState previousState = PreviousState(buffer, pass, true);
            //if(previousState != edge->state)  // 状态一样也加屏障？ 比如连续两个UAV读写的情况？
            {
                RHIBufferBarrier barrier = {
                    Resolve(buffer),  // 对应 .buffer
                    previousState,    // 对应 .srcState
                    edge->state,      // 对应 .dstState
                    edge->offset,     // 对应 .offset
                    edge->size        // 对应 .size
                };
                command->BufferBarrier(barrier);

                // printf("rdg resource %lld, raw: %s barrier: %d to %d\n", (int64_t)buffer->buffer.get(), ToHex((uint64_t)buffer->buffer->RawHandle(), false).c_str(), (uint32_t)previousState, (uint32_t)edge->state);
            }
            });
    }

    void RDGBuilder::PrepareDescriptorSet(RDGPassNodeRef pass)
    {
        // ForEachTexture会遍历每一个输入的纹理，再遍历每一个输出的纹理
        pass->ForEachTexture([&](RDGTextureEdgeRef edge, RDGTextureNodeRef texture) {

            if (edge->IsOutput()) return;    // 作为output声明时不需要view  TODO:???
            RHITextureViewInfo info;
            info.texture = Resolve(texture); // 创建资源
            info.format = texture->info.format;
            info.viewType = edge->viewType;
            info.subresource = edge->subresource;
            RHITextureViewRef view = RDGTextureViewPool::Get()->Allocate(info).textureView;
            pass->pooledViews.push_back(view);

            // 看这个纹理对应的资源描述符创建没有
            if (pass->descriptorSets[edge->set] == nullptr && pass->rootSignature != nullptr)
            {
                auto descriptor = RDGDescriptorSetPool::Get(APP_FRAMEINDEX)->Allocate(pass->rootSignature, edge->set).descriptor;
                pass->descriptorSets[edge->set] = descriptor;
                pass->pooledDescriptorSets.push_back({ descriptor, edge->set });
            }

            // 更新资源描述符
            if ((edge->asShaderRead || edge->asShaderReadWrite) &&
                pass->descriptorSets[edge->set] != nullptr)
            {
                RHIDescriptorUpdateInfo updateInfo;
                updateInfo.binding = edge->binding;
                updateInfo.index = edge->index;
                updateInfo.resourceType = edge->type;
                updateInfo.textureView = view;
                pass->descriptorSets[edge->set]->UpdateDescriptor(updateInfo);
            }
            });
        // Buffer同Textrue一样
        pass->ForEachBuffer([&](RDGBufferEdgeRef edge, RDGBufferNodeRef buffer) {

            if (pass->descriptorSets[edge->set] == nullptr && pass->rootSignature != nullptr)
            {
                auto descriptor = RDGDescriptorSetPool::Get(APP_FRAMEINDEX)->Allocate(pass->rootSignature, edge->set).descriptor;
                pass->descriptorSets[edge->set] = descriptor;
                pass->pooledDescriptorSets.push_back({ descriptor, edge->set });
            }

            if ((edge->asShaderRead || edge->asShaderReadWrite) &&
                pass->descriptorSets[edge->set] != nullptr)
            {
                RHIDescriptorUpdateInfo updateInfo;
                updateInfo.binding = edge->binding;
                updateInfo.index = edge->index;
                updateInfo.resourceType = edge->type;
                updateInfo.buffer = Resolve(buffer);
                updateInfo.bufferOffset = edge->offset;
                updateInfo.bufferRange = edge->size;

                pass->descriptorSets[edge->set]->UpdateDescriptor(updateInfo);
            }
            });

    }

    void RDGBuilder::PrepareRenderTarget(RDGRenderPassNodeRef pass, RHIRenderPassInfo& renderPassInfo)
    {
        // 单独处理RT
        pass->ForEachTexture([&](RDGTextureEdgeRef edge, RDGTextureNodeRef texture) {

            if (edge->IsOutput()) return;                            
            if (!(edge->asColor || edge->asDepthStencil)) return;
            RHITextureViewInfo info;
            info.texture = Resolve(texture);
            info.format = texture->info.format;
            info.viewType = edge->viewType;
            info.subresource = edge->subresource;
           
            RHITextureViewRef view = RDGTextureViewPool::Get()->Allocate(info).textureView;
            pass->pooledViews.push_back(view);

            if (edge->asColor)
            {
                renderPassInfo.extent = { texture->info.extent.width, texture->info.extent.height };
                renderPassInfo.layers = edge->subresource.layerCount > 0 ? edge->subresource.layerCount : 1;
                AttachmentInfo info;
                info.textureView = view;
                info.loadOp = edge->loadOp;
                info.storeOp = edge->storeOp;
                info.clearColor = edge->clearColor;
                renderPassInfo.colorAttachments[edge->binding] = info;
            }
            else if (edge->asDepthStencil)
            {
                renderPassInfo.extent = { texture->info.extent.width, texture->info.extent.height };
                renderPassInfo.layers = edge->subresource.layerCount > 0 ? edge->subresource.layerCount : 1;
                AttachmentInfo info;
                info.textureView = view;
                info.loadOp = edge->loadOp;
                info.storeOp = edge->storeOp;
                info.clearDepth = edge->clearDepth;
                info.clearStencil = edge->clearStencil;
                renderPassInfo.depthStencilAttachment = info;
            }
            });
    }

    void RDGBuilder::ReleaseResource(RDGPassNodeRef pass)
    {
        pass->ForEachTexture([&](RDGTextureEdgeRef edge, RDGTextureNodeRef texture) {
            if (IsLastUsedPass(texture, pass, edge->IsOutput())) Release(texture, edge->state);
            });

        pass->ForEachBuffer([&](RDGBufferEdgeRef edge, RDGBufferNodeRef buffer) {
            if (IsLastUsedPass(buffer, pass, edge->IsOutput())) Release(buffer, edge->state);
            });

        for (auto& view : pass->pooledViews)
        {
            RDGTextureViewPool::Get()->Release({ view });
        }
        pass->pooledViews.clear();
    }

    void RDGBuilder::ExecutePass(RDGRenderPassNodeRef pass)
    {
        // 根据各个资源依赖的edge收集描述符更新信息以及framebuffer信息，
        // 调用Resolve()来分配和获取实际的RHI资源，资源将在最后一个使用的pass之后返回资源池
        // 处理状态转换的屏障

        PrepareDescriptorSet(pass);   // 根据这个Pass需要的资源进行实际创建 + 更新描述符

        RHIRenderPassInfo renderPassInfo = {};
        PrepareRenderTarget(pass, renderPassInfo);   // 准备RenderPass需要的信息

        RHIRenderPassRef renderPass = APP_DYNAMCIRHI->CreateRenderPass(renderPassInfo);   // 创建RenderPass和FrameBuffer

        // command->PushEvent(pass->Name(), { 0.0f, 0.0f, 0.0f });

        CreateInputBarriers(pass);

        command->BeginRenderPass(renderPass);

        RDGPassContext context;   // 在这准备的Context
        context.command = command;
        context.builder = this;
        context.descriptors = pass->descriptorSets;
        context.passIndex[0] = pass->passIndex[0];
        context.passIndex[1] = pass->passIndex[1];
        context.passIndex[2] = pass->passIndex[2];
        pass->execute(context);

        command->EndRenderPass();

        CreateOutputBarriers(pass);

        ReleaseResource(pass);

        // command->PopEvent();
    }

    void RDGBuilder::ExecutePass(RDGComputePassNodeRef pass)
    {

        PrepareDescriptorSet(pass);

        // command->PushEvent(pass->Name(), { 1.0f, 0.0f, 0.0f });

        CreateInputBarriers(pass);

        RDGPassContext context;
        context.command = command;
        context.builder = this;
        context.descriptors = pass->descriptorSets;
        context.passIndex[0] = pass->passIndex[0];
        context.passIndex[1] = pass->passIndex[1];
        context.passIndex[2] = pass->passIndex[2];
        pass->execute(context);

        CreateOutputBarriers(pass);

        ReleaseResource(pass);

        // command->PopEvent();
    }

    void RDGBuilder::ExecutePass(RDGRayTracingPassNodeRef pass)
    {

        PrepareDescriptorSet(pass);

        // command->PushEvent(pass->Name(), { 0.0f, 1.0f, 0.0f });

        CreateInputBarriers(pass);

        RDGPassContext context = {
            context.command = command,
            context.builder = this,
            context.descriptors = pass->descriptorSets
        };
        context.passIndex[0] = pass->passIndex[0];
        context.passIndex[1] = pass->passIndex[1];
        context.passIndex[2] = pass->passIndex[2];
        pass->execute(context);

        CreateOutputBarriers(pass);

        ReleaseResource(pass);

        // command->PopEvent();
    }

    void RDGBuilder::ExecutePass(RDGPresentPassNodeRef pass)
    {
        RDGTextureNodeRef presentTexture;
        RDGTextureNodeRef texture;
        TextureSubresourceLayers subresource;

        auto edges = pass->InEdges<RDGTextureEdge>();
        if (edges[0]->asPresent)
        {
            presentTexture = edges[0]->From<RDGTextureNode>();
            texture = edges[1]->From<RDGTextureNode>();
            subresource = edges[1]->subresource.aspect == TEXTURE_ASPECT_NONE ?
                Resolve(texture)->GetDefaultSubresourceLayers() : edges[1]->subresourceLayer;
        }
        else
        {
            presentTexture = edges[1]->From<RDGTextureNode>();
            texture = edges[0]->From<RDGTextureNode>();
            subresource = edges[0]->subresource.aspect == TEXTURE_ASPECT_NONE ?
                Resolve(texture)->GetDefaultSubresourceLayers() : edges[0]->subresourceLayer;
        }

        // command->PushEvent(pass->Name(), { 0.0f, 0.0f, 1.0f });

        CreateInputBarriers(pass);

        command->TextureBarrier({ Resolve(presentTexture), RESOURCE_STATE_PRESENT, RESOURCE_STATE_TRANSFER_DST });
        command->CopyTexture(Resolve(texture), subresource,
            Resolve(presentTexture), { TEXTURE_ASPECT_COLOR, 0, 0, 1 });
        command->TextureBarrier({ Resolve(presentTexture), RESOURCE_STATE_TRANSFER_DST, RESOURCE_STATE_PRESENT });

        CreateOutputBarriers(pass);

        ReleaseResource(pass);

        // command->PopEvent();
    }

    void RDGBuilder::ExecutePass(RDGCopyPassNodeRef pass)
    {
        RDGTextureNodeRef from;
        RDGTextureNodeRef to;
        TextureSubresourceLayers fromSubresource;
        TextureSubresourceLayers toSubresource;
        pass->ForEachTexture([&](RDGTextureEdgeRef edge, RDGTextureNodeRef texture) {

            if (edge->asTransferSrc)
            {
                from = texture;
                fromSubresource = edge->subresourceLayer;
            }
            else if (edge->asTransferDst)
            {
                to = texture;
                toSubresource = edge->subresourceLayer;
            }
            });

        // command->PushEvent(pass->Name(), { 1.0f, 1.0f, 0.0f });

        CreateInputBarriers(pass);

        if (from && to)
        {
            command->CopyTexture(Resolve(from), fromSubresource,
                Resolve(to), toSubresource);

            if (pass->generateMip)
            {
                RHITextureBarrier barrier = {
                    barrier.texture = Resolve(to),
                    barrier.srcState = RESOURCE_STATE_TRANSFER_DST,
                    barrier.dstState = RESOURCE_STATE_TRANSFER_SRC,
                    barrier.subresource = {}
                };
                command->TextureBarrier(barrier);
                command->GenerateMips(Resolve(to)); // 默认纹理处于src状态，需要手动加屏障

                barrier = {
                    barrier.texture = Resolve(to),
                    barrier.srcState = RESOURCE_STATE_TRANSFER_SRC,
                    barrier.dstState = RESOURCE_STATE_TRANSFER_DST,
                    barrier.subresource = {}
                };
                command->TextureBarrier(barrier);
            }
        }

        CreateOutputBarriers(pass);

        ReleaseResource(pass);

        // command->PopEvent();
    }

    RHITextureRef RDGBuilder::Resolve(RDGTextureNodeRef textureNode)
    {
        if (textureNode->texture == nullptr)
        {
            auto pooledTexture = RDGTexturePool::Get()->Allocate(textureNode->info);
            textureNode->texture = pooledTexture.texture;
            textureNode->initState = pooledTexture.state;

            // printf("rdg resource %s allocated: %lld, raw: %s\n", textureNode->Name().c_str(), (int64_t)textureNode->texture.get(), ToHex((uint64_t)textureNode->texture->RawHandle(), false).c_str());
        }

        // // printf("rdg resource %s raw: %s\n", textureNode->Name().c_str(), ToHex((uint64_t)textureNode->texture->RawHandle(), false).c_str());


        return textureNode->texture;
    }

    RHIBufferRef RDGBuilder::Resolve(RDGBufferNodeRef bufferNode)
    {
        if (bufferNode->buffer == nullptr)
        {
            auto pooledBuffer = RDGBufferPool::Get()->Allocate(bufferNode->info);
            bufferNode->buffer = pooledBuffer.buffer;
            bufferNode->initState = pooledBuffer.state;

            // printf("rdg resource %s allocated: %lld, raw: %s\n", bufferNode->Name().c_str(), (int64_t)bufferNode->buffer.get(), ToHex((uint64_t)bufferNode->buffer->RawHandle(), false).c_str());
        }

        // // printf("rdg resource %s raw: %s\n", bufferNode->Name().c_str(), ToHex((uint64_t)bufferNode->buffer->RawHandle(), false).c_str());

        return bufferNode->buffer;
    }

    void RDGBuilder::Release(RDGTextureNodeRef textureNode, RHIResourceState state)
    {
        if (textureNode->IsImported()) return;
        if (textureNode->texture)
        {
            // printf("rdg resource %s released: %lld, raw: %s\n", textureNode->Name().c_str(), (int64_t)textureNode->texture.get(), ToHex((uint64_t)textureNode->texture->RawHandle(), false).c_str());

            RDGTexturePool::Get()->Release({ textureNode->texture, state });
            textureNode->texture = nullptr;
            textureNode->initState = RESOURCE_STATE_UNDEFINED;
        }

    }

    void RDGBuilder::Release(RDGBufferNodeRef bufferNode, RHIResourceState state)
    {
        if (bufferNode->IsImported()) return;
        if (bufferNode->buffer)
        {
            // printf("rdg resource %s released: %lld, raw: %s\n", bufferNode->Name().c_str(), (int64_t)bufferNode->buffer.get(), ToHex((uint64_t)bufferNode->buffer->RawHandle(), false).c_str());

            RDGBufferPool::Get()->Release({ bufferNode->buffer, state });
            bufferNode->buffer = nullptr;
            bufferNode->initState = RESOURCE_STATE_UNDEFINED;
        }
    }

    RHIResourceState RDGBuilder::PreviousState(RDGTextureNodeRef textureNode, RDGPassNodeRef passNode, TextureSubresourceRange subresource, bool output)
    {
        Resolve(textureNode);
        NodeID currentID = passNode->ID();
        NodeID previousID = UINT32_MAX;

        RHIResourceState previousState = textureNode->initState;        // 若没有前序引用，那状态就是资源本身的初始状态

        textureNode->ForEachPass([&](RDGTextureEdgeRef edge, RDGPassNodeRef pass) {

            bool isOutputFirst = output ? !edge->IsOutput() : edge->IsOutput();
            bool isPrevoiusPass = output ? pass->ID() <= currentID : pass->ID() < currentID;
            bool isSubresourceCovered = subresource.IsDefault() ||                      // 无状态地追踪整个子资源状态实在有些困难，现在支持的方法是：
                edge->subresource.IsDefault() ||                // 1. 若目标状态是默认范围，那只追踪前序最近的状态
                subresource == edge->subresource;               // 2. 若目标状态是子范围，那必须追踪前序最近的完全一致的子范围/默认范围的状态
            // 再合理利用output的手动屏障，应该能够完成全部子范围的管理
            if (!(isPrevoiusPass && isSubresourceCovered)) return;
            if (pass->ID() > previousID || previousID == UINT32_MAX)     // 不同的前序pass，取最后一个的状态
            {
                previousState = edge->state;
                previousID = pass->ID();
            }
            else if (pass->ID() == previousID &&                         // 同一个前序pass，考虑取输入or输出状态
                isOutputFirst)
            {
                previousState = edge->state;
                previousID = pass->ID();
            }
            });

        return previousState;
    }

    RHIResourceState RDGBuilder::PreviousState(RDGBufferNodeRef bufferNode, RDGPassNodeRef passNode, uint32_t offset, uint32_t size, bool output)
    {
        Resolve(bufferNode);
        NodeID currentID = passNode->ID();
        NodeID previousID = UINT32_MAX;

        RHIResourceState previousState = bufferNode->initState;         // 若没有前序引用，那状态就是资源本身的初始状态

        bufferNode->ForEachPass([&](RDGBufferEdgeRef edge, RDGPassNodeRef pass) {

            bool isOutputFirst = output ? !edge->IsOutput() : edge->IsOutput();
            bool isPrevoiusPass = output ? pass->ID() <= currentID : pass->ID() < currentID;
            bool isSubresourceCovered = (offset == 0 && size == 0) ||
                (edge->offset == 0 && edge->size == 0) ||
                (offset == edge->offset && size == edge->size);  // 同texture里的策略

            if (!(isPrevoiusPass && isSubresourceCovered)) return;
            if (pass->ID() > previousID || previousID == UINT32_MAX)     // 不同的前序pass，取最后一个的状态
            {
                previousState = edge->state;
                previousID = pass->ID();
            }
            else if (pass->ID() == previousID &&                         // 同一个前序pass，考虑取输入or输出状态
                isOutputFirst)
            {
                previousState = edge->state;
                previousID = pass->ID();
            }
            });

        return previousState;
    }

    bool RDGBuilder::IsLastUsedPass(RDGTextureNodeRef textureNode, RDGPassNodeRef passNode, bool output)
    {
        NodeID currentID = passNode->ID();
        bool last = true;

        textureNode->ForEachPass([&](RDGTextureEdgeRef edge, RDGPassNodeRef pass) {
            if (pass->ID() > currentID) last = false;
            if (!output && pass->ID() == currentID && edge->IsOutput()) last = false;
            });

        return last;
    }

    bool RDGBuilder::IsLastUsedPass(RDGBufferNodeRef bufferNode, RDGPassNodeRef passNode, bool output)
    {
        NodeID currentID = passNode->ID();
        bool last = true;

        bufferNode->ForEachPass([&](RDGBufferEdgeRef edge, RDGPassNodeRef pass) {
            if (pass->ID() > currentID) last = false;
            if (!output && pass->ID() == currentID && edge->IsOutput()) last = false;
            });

        return last;
    }

    RDGTextureBuilder& RDGTextureBuilder::Import(RHITextureRef texture, RHIResourceState initState)
    {
        this->texture->isImported = true;
        this->texture->texture = texture;
        this->texture->info = texture->GetInfo();
        this->texture->initState = initState;
        return *this;
    }

    RDGTextureBuilder& RDGTextureBuilder::Exetent(Extent3D extent)
    {
        texture->info.extent = extent;
        return *this;
    }

    RDGTextureBuilder& RDGTextureBuilder::Format(RHIFormat format)
    {
        texture->info.format = format;
        return *this;
    }

    RDGTextureBuilder& RDGTextureBuilder::MemoryUsage(enum MemoryUsage memoryUsage)
    {
        texture->info.memoryUsage = memoryUsage;
        return  *this;
    }

    RDGTextureBuilder& RDGTextureBuilder::AllowReadWrite()
    {
        texture->info.type |= RESOURCE_TYPE_RW_TEXTURE;
        //texture->initState = RESOURCE_STATE_UNORDERED_ACCESS;
        return  *this;
    }

    RDGTextureBuilder& RDGTextureBuilder::AllowRenderTarget()
    {
        texture->info.type |= RESOURCE_TYPE_RENDER_TARGET;
        //texture->initState = RESOURCE_STATE_COLOR_ATTACHMENT;
        return  *this;
    }

    RDGTextureBuilder& RDGTextureBuilder::AllowDepthStencil()
    {
        texture->info.type |= RESOURCE_TYPE_RENDER_TARGET;
        //texture->initState = RESOURCE_STATE_DEPTH_STENCIL_ATTACHMENT;
        return  *this;
    }

    RDGTextureBuilder& RDGTextureBuilder::MipLevels(uint32_t mipLevels)
    {
        texture->info.mipLevels = mipLevels;
        return *this;
    }

    RDGTextureBuilder& RDGTextureBuilder::ArrayLayers(uint32_t arrayLayers)
    {
        texture->info.arrayLayers = arrayLayers;
        return *this;
    }

    RDGBufferBuilder& RDGBufferBuilder::Import(RHIBufferRef buffer, RHIResourceState initState)
    {
        this->buffer->isImported = true;
        this->buffer->buffer = buffer;
        this->buffer->info = buffer->GetInfo();
        this->buffer->initState = initState;
        return *this;
    }

    RDGBufferBuilder& RDGBufferBuilder::Size(uint32_t size)
    {
        buffer->info.size = size;
        return *this;
    }

    RDGBufferBuilder& RDGBufferBuilder::MemoryUsage(enum MemoryUsage memoryUsage)
    {
        buffer->info.memoryUsage = memoryUsage;
        return  *this;
    }

    RDGBufferBuilder& RDGBufferBuilder::AllowVertexBuffer()
    {
        buffer->info.type |= RESOURCE_TYPE_VERTEX_BUFFER;
        //buffer->initState = RESOURCE_STATE_VERTEX_BUFFER;
        return *this;
    }

    RDGBufferBuilder& RDGBufferBuilder::AllowIndexBuffer()
    {
        buffer->info.type |= RESOURCE_TYPE_INDEX_BUFFER;
        //buffer->initState = RESOURCE_STATE_INDEX_BUFFER;
        return *this;
    }

    RDGBufferBuilder& RDGBufferBuilder::AllowReadWrite()
    {
        buffer->info.type |= RESOURCE_TYPE_RW_BUFFER;
        //buffer->initState = RESOURCE_STATE_UNORDERED_ACCESS;
        return *this;
    }

    RDGBufferBuilder& RDGBufferBuilder::AllowRead()
    {
        buffer->info.type |= RESOURCE_TYPE_UNIFORM_BUFFER;
        //buffer->initState = RESOURCE_STATE_SHADER_RESOURCE;
        return *this;
    }

    RDGRenderPassBuilder& RDGRenderPassBuilder::PassIndex(uint32_t x, uint32_t y, uint32_t z)
    {
        pass->passIndex[0] = x;
        pass->passIndex[1] = y;
        pass->passIndex[2] = z;
        return *this;
    }

    RDGRenderPassBuilder& RDGRenderPassBuilder::RootSignature(RHIRootSignatureRef rootSignature)
    {
        pass->rootSignature = rootSignature;
        return *this;
    }

    RDGRenderPassBuilder& RDGRenderPassBuilder::DescriptorSet(uint32_t set, RHIDescriptorSetRef descriptorSet)
    {
        pass->descriptorSets[set] = descriptorSet;
        return *this;
    }

    RDGRenderPassBuilder& RDGRenderPassBuilder::Read(uint32_t set, uint32_t binding, uint32_t index, RDGBufferHandle buffer, uint32_t offset, uint32_t size)
    {
        RDGBufferEdgeRef edge = graph->CreateEdge<RDGBufferEdge>();
        edge->state = RESOURCE_STATE_SHADER_RESOURCE;
        edge->offset = offset;
        edge->size = size;
        edge->asShaderRead = true;
        edge->set = set;
        edge->binding = binding;
        edge->index = index;
        edge->type = RESOURCE_TYPE_UNIFORM_BUFFER;

        graph->Link(graph->GetNode(buffer.ID()), pass, edge); // 连接Buffer结点和当前Pass结点，在边中存储读Shader

        return *this;
    }

    RDGRenderPassBuilder& RDGRenderPassBuilder::Read(uint32_t set, uint32_t binding, uint32_t index, RDGTextureHandle texture, TextureViewType viewType, TextureSubresourceRange subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_SHADER_RESOURCE;
        edge->subresource = subresource;
        edge->asShaderRead = true;
        edge->set = set;
        edge->binding = binding;
        edge->index = index;
        edge->type = RESOURCE_TYPE_TEXTURE;
        edge->viewType = viewType;

        graph->Link(graph->GetNode(texture.ID()), pass, edge);

        return *this;
    }

    RDGRenderPassBuilder& RDGRenderPassBuilder::ReadWrite(uint32_t set, uint32_t binding, uint32_t index, RDGBufferHandle buffer, uint32_t offset, uint32_t size)
    {
        RDGBufferEdgeRef edge = graph->CreateEdge<RDGBufferEdge>();
        edge->state = RESOURCE_STATE_UNORDERED_ACCESS; // UAV（随机读写视图）
        edge->offset = offset;
        edge->size = size;
        edge->asShaderReadWrite = true;
        edge->set = set;
        edge->binding = binding;
        edge->index = index;
        edge->type = RESOURCE_TYPE_RW_BUFFER;

        graph->Link(pass, graph->GetNode(buffer.ID()), edge);   // 从Pass连向资源，表示会写入它

        return *this;
    }

    RDGRenderPassBuilder& RDGRenderPassBuilder::ReadWrite(uint32_t set, uint32_t binding, uint32_t index, RDGTextureHandle texture, TextureViewType viewType, TextureSubresourceRange subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_UNORDERED_ACCESS; // UAV（随机读写视图）
        edge->subresource = subresource;
        edge->asShaderReadWrite = true;
        edge->set = set;
        edge->binding = binding;
        edge->index = index;
        edge->type = RESOURCE_TYPE_RW_TEXTURE;
        edge->viewType = viewType;

        graph->Link(pass, graph->GetNode(texture.ID()), edge); // 从Pass连向资源，表示会写入它

        return *this;
    }

    RDGRenderPassBuilder& RDGRenderPassBuilder::Color(uint32_t binding, RDGTextureHandle texture,
        AttachmentLoadOp load,
        AttachmentStoreOp store,
        Color4 clearColor,
        TextureSubresourceRange subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_COLOR_ATTACHMENT;
        edge->loadOp = load;
        edge->storeOp = store;
        edge->clearColor = clearColor;
        edge->subresource = subresource;
        edge->asColor = true;
        edge->binding = binding;  // 这传递Binding干毛
        edge->viewType = subresource.layerCount > 1 ? VIEW_TYPE_2D_ARRAY : VIEW_TYPE_2D;

        graph->Link(pass, graph->GetNode(texture.ID()), edge);  // 输出

        return *this;
    }

    RDGRenderPassBuilder& RDGRenderPassBuilder::DepthStencil(RDGTextureHandle texture,
        AttachmentLoadOp load,
        AttachmentStoreOp store,
        float clearDepth,
        uint32_t clearStencil,
        TextureSubresourceRange subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_DEPTH_STENCIL_ATTACHMENT;
        edge->loadOp = load;
        edge->storeOp = store;
        edge->clearDepth = clearDepth;
        edge->clearStencil = clearStencil;
        edge->subresource = subresource;
        edge->asDepthStencil = true;
        edge->viewType = subresource.layerCount > 1 ? VIEW_TYPE_2D_ARRAY : VIEW_TYPE_2D;

        graph->Link(pass, graph->GetNode(texture.ID()), edge);

        return *this;
    }

    RDGRenderPassBuilder& RDGRenderPassBuilder::OutputRead(RDGBufferHandle buffer, uint32_t offset, uint32_t size)
    {
        RDGBufferEdgeRef edge = graph->CreateEdge<RDGBufferEdge>();
        edge->state = RESOURCE_STATE_SHADER_RESOURCE;
        edge->offset = offset;
        edge->size = size;
        edge->asOutputRead = true;
        edge->type = RESOURCE_TYPE_BUFFER;

        graph->Link(pass, graph->GetNode(buffer.ID()), edge);

        return *this;
    }

    RDGRenderPassBuilder& RDGRenderPassBuilder::OutputRead(RDGTextureHandle texture, TextureSubresourceRange subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_SHADER_RESOURCE;
        edge->subresource = subresource;
        edge->asOutputReadWrite = true;
        edge->type = RESOURCE_TYPE_TEXTURE;

        graph->Link(pass, graph->GetNode(texture.ID()), edge);

        return *this;
    }

    RDGRenderPassBuilder& RDGRenderPassBuilder::OutputReadWrite(RDGBufferHandle buffer, uint32_t offset, uint32_t size)
    {
        RDGBufferEdgeRef edge = graph->CreateEdge<RDGBufferEdge>();
        edge->state = RESOURCE_STATE_UNORDERED_ACCESS;
        edge->offset = offset;
        edge->size = size;
        edge->asOutputReadWrite = true;
        edge->type = RESOURCE_TYPE_RW_BUFFER;

        graph->Link(pass, graph->GetNode(buffer.ID()), edge);

        return *this;
    }

    RDGRenderPassBuilder& RDGRenderPassBuilder::OutputReadWrite(RDGTextureHandle texture, TextureSubresourceRange subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_UNORDERED_ACCESS;
        edge->subresource = subresource;
        edge->asOutputReadWrite = true;
        edge->type = RESOURCE_TYPE_RW_TEXTURE;

        graph->Link(pass, graph->GetNode(texture.ID()), edge);

        return *this;
    }

    RDGRenderPassBuilder& RDGRenderPassBuilder::Execute(const RDGPassExecuteFunc& execute)
    {
        pass->execute = execute;
        return *this;
    }

    RDGComputePassBuilder& RDGComputePassBuilder::PassIndex(uint32_t x, uint32_t y, uint32_t z)
    {
        pass->passIndex[0] = x;
        pass->passIndex[1] = y;
        pass->passIndex[2] = z;
        return *this;
    }

    RDGComputePassBuilder& RDGComputePassBuilder::RootSignature(RHIRootSignatureRef rootSignature)
    {
        pass->rootSignature = rootSignature;
        return *this;
    }

    RDGComputePassBuilder& RDGComputePassBuilder::DescriptorSet(uint32_t set, RHIDescriptorSetRef descriptorSet)
    {
        pass->descriptorSets[set] = descriptorSet;
        return *this;
    }

    RDGComputePassBuilder& RDGComputePassBuilder::Read(uint32_t set, uint32_t binding, uint32_t index, RDGBufferHandle buffer, uint32_t offset, uint32_t size)
    {
        RDGBufferEdgeRef edge = graph->CreateEdge<RDGBufferEdge>();
        edge->state = RESOURCE_STATE_SHADER_RESOURCE;
        edge->offset = offset;
        edge->size = size;
        edge->asShaderRead = true;
        edge->set = set;
        edge->binding = binding;
        edge->index = index;
        edge->type = RESOURCE_TYPE_UNIFORM_BUFFER;

        graph->Link(graph->GetNode(buffer.ID()), pass, edge);

        return *this;
    }

    RDGComputePassBuilder& RDGComputePassBuilder::Read(uint32_t set, uint32_t binding, uint32_t index, RDGTextureHandle texture, TextureViewType viewType, TextureSubresourceRange subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_SHADER_RESOURCE;
        edge->subresource = subresource;
        edge->asShaderRead = true;
        edge->set = set;
        edge->binding = binding;
        edge->index = index;
        edge->type = RESOURCE_TYPE_TEXTURE;
        edge->viewType = viewType;

        graph->Link(graph->GetNode(texture.ID()), pass, edge);

        return *this;
    }

    RDGComputePassBuilder& RDGComputePassBuilder::ReadWrite(uint32_t set, uint32_t binding, uint32_t index, RDGBufferHandle buffer, uint32_t offset, uint32_t size)
    {
        RDGBufferEdgeRef edge = graph->CreateEdge<RDGBufferEdge>();
        edge->state = RESOURCE_STATE_UNORDERED_ACCESS;
        edge->offset = offset;
        edge->size = size;
        edge->asShaderReadWrite = true;
        edge->set = set;
        edge->binding = binding;
        edge->index = index;
        edge->type = RESOURCE_TYPE_RW_BUFFER;

        graph->Link(pass, graph->GetNode(buffer.ID()), edge);

        return *this;
    }

    RDGComputePassBuilder& RDGComputePassBuilder::ReadWrite(uint32_t set, uint32_t binding, uint32_t index, RDGTextureHandle texture, TextureViewType viewType, TextureSubresourceRange subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_UNORDERED_ACCESS;
        edge->subresource = subresource;
        edge->asShaderReadWrite = true;
        edge->set = set;
        edge->binding = binding;
        edge->index = index;
        edge->type = RESOURCE_TYPE_RW_TEXTURE;
        edge->viewType = viewType;

        graph->Link(pass, graph->GetNode(texture.ID()), edge);

        return *this;
    }

    RDGComputePassBuilder& RDGComputePassBuilder::OutputRead(RDGBufferHandle buffer, uint32_t offset, uint32_t size)
    {
        RDGBufferEdgeRef edge = graph->CreateEdge<RDGBufferEdge>();
        edge->state = RESOURCE_STATE_SHADER_RESOURCE;
        edge->offset = offset;
        edge->size = size;
        edge->asOutputRead = true;
        edge->type = RESOURCE_TYPE_BUFFER;

        graph->Link(pass, graph->GetNode(buffer.ID()), edge);

        return *this;
    }

    RDGComputePassBuilder& RDGComputePassBuilder::OutputRead(RDGTextureHandle texture, TextureSubresourceRange subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_SHADER_RESOURCE;
        edge->subresource = subresource;
        edge->asOutputReadWrite = true;
        edge->type = RESOURCE_TYPE_TEXTURE;

        graph->Link(pass, graph->GetNode(texture.ID()), edge);

        return *this;
    }

    RDGComputePassBuilder& RDGComputePassBuilder::OutputReadWrite(RDGBufferHandle buffer, uint32_t offset, uint32_t size)
    {
        RDGBufferEdgeRef edge = graph->CreateEdge<RDGBufferEdge>();
        edge->state = RESOURCE_STATE_UNORDERED_ACCESS;
        edge->offset = offset;
        edge->size = size;
        edge->asOutputReadWrite = true;
        edge->type = RESOURCE_TYPE_RW_BUFFER;

        graph->Link(pass, graph->GetNode(buffer.ID()), edge);

        return *this;
    }

    RDGComputePassBuilder& RDGComputePassBuilder::OutputReadWrite(RDGTextureHandle texture, TextureSubresourceRange subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_UNORDERED_ACCESS;
        edge->subresource = subresource;
        edge->asOutputReadWrite = true;
        edge->type = RESOURCE_TYPE_RW_TEXTURE;

        graph->Link(pass, graph->GetNode(texture.ID()), edge);

        return *this;
    }

    RDGComputePassBuilder& RDGComputePassBuilder::OutputIndirectDraw(RDGBufferHandle buffer, uint32_t offset, uint32_t size)
    {
        RDGBufferEdgeRef edge = graph->CreateEdge<RDGBufferEdge>();
        edge->state = RESOURCE_STATE_INDIRECT_ARGUMENT;
        edge->offset = offset;
        edge->size = size;
        edge->asOutputIndirectDraw = true;
        edge->type = RESOURCE_TYPE_INDIRECT_BUFFER;

        graph->Link(pass, graph->GetNode(buffer.ID()), edge);

        return *this;
    }

    RDGComputePassBuilder& RDGComputePassBuilder::Execute(const RDGPassExecuteFunc& execute)
    {
        pass->execute = execute;
        return *this;
    }

    RDGRayTracingPassBuilder& RDGRayTracingPassBuilder::PassIndex(uint32_t x, uint32_t y, uint32_t z)
    {
        pass->passIndex[0] = x;
        pass->passIndex[1] = y;
        pass->passIndex[2] = z;
        return *this;
    }

    RDGRayTracingPassBuilder& RDGRayTracingPassBuilder::RootSignature(RHIRootSignatureRef rootSignature)
    {
        pass->rootSignature = rootSignature;
        return *this;
    }

    RDGRayTracingPassBuilder& RDGRayTracingPassBuilder::DescriptorSet(uint32_t set, RHIDescriptorSetRef descriptorSet)
    {
        pass->descriptorSets[set] = descriptorSet;
        return *this;
    }

    RDGRayTracingPassBuilder& RDGRayTracingPassBuilder::Read(uint32_t set, uint32_t binding, uint32_t index, RDGBufferHandle buffer, uint32_t offset, uint32_t size)
    {
        RDGBufferEdgeRef edge = graph->CreateEdge<RDGBufferEdge>();
        edge->state = RESOURCE_STATE_SHADER_RESOURCE;
        edge->offset = offset;
        edge->size = size;
        edge->asShaderRead = true;
        edge->set = set;
        edge->binding = binding;
        edge->index = index;
        edge->type = RESOURCE_TYPE_UNIFORM_BUFFER;

        graph->Link(graph->GetNode(buffer.ID()), pass, edge);

        return *this;
    }

    RDGRayTracingPassBuilder& RDGRayTracingPassBuilder::Read(uint32_t set, uint32_t binding, uint32_t index, RDGTextureHandle texture, TextureViewType viewType, TextureSubresourceRange subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_SHADER_RESOURCE;
        edge->subresource = subresource;
        edge->asShaderRead = true;
        edge->set = set;
        edge->binding = binding;
        edge->index = index;
        edge->type = RESOURCE_TYPE_TEXTURE;
        edge->viewType = viewType;

        graph->Link(graph->GetNode(texture.ID()), pass, edge);

        return *this;
    }

    RDGRayTracingPassBuilder& RDGRayTracingPassBuilder::ReadWrite(uint32_t set, uint32_t binding, uint32_t index, RDGBufferHandle buffer, uint32_t offset, uint32_t size)
    {
        RDGBufferEdgeRef edge = graph->CreateEdge<RDGBufferEdge>();
        edge->state = RESOURCE_STATE_UNORDERED_ACCESS;
        edge->offset = offset;
        edge->size = size;
        edge->asShaderReadWrite = true;
        edge->set = set;
        edge->binding = binding;
        edge->index = index;
        edge->type = RESOURCE_TYPE_RW_BUFFER;

        graph->Link(pass, graph->GetNode(buffer.ID()), edge);

        return *this;
    }

    RDGRayTracingPassBuilder& RDGRayTracingPassBuilder::ReadWrite(uint32_t set, uint32_t binding, uint32_t index, RDGTextureHandle texture, TextureViewType viewType, TextureSubresourceRange subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_UNORDERED_ACCESS;
        edge->subresource = subresource;
        edge->asShaderReadWrite = true;
        edge->set = set;
        edge->binding = binding;
        edge->index = index;
        edge->type = RESOURCE_TYPE_RW_TEXTURE;
        edge->viewType = viewType;

        graph->Link(pass, graph->GetNode(texture.ID()), edge);

        return *this;
    }

    RDGRayTracingPassBuilder& RDGRayTracingPassBuilder::OutputRead(RDGBufferHandle buffer, uint32_t offset, uint32_t size)
    {
        RDGBufferEdgeRef edge = graph->CreateEdge<RDGBufferEdge>();
        edge->state = RESOURCE_STATE_SHADER_RESOURCE;
        edge->offset = offset;
        edge->size = size;
        edge->asOutputRead = true;
        edge->type = RESOURCE_TYPE_BUFFER;

        graph->Link(pass, graph->GetNode(buffer.ID()), edge);

        return *this;
    }

    RDGRayTracingPassBuilder& RDGRayTracingPassBuilder::OutputRead(RDGTextureHandle texture, TextureSubresourceRange subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_SHADER_RESOURCE;
        edge->subresource = subresource;
        edge->asOutputReadWrite = true;
        edge->type = RESOURCE_TYPE_TEXTURE;

        graph->Link(pass, graph->GetNode(texture.ID()), edge);

        return *this;
    }

    RDGRayTracingPassBuilder& RDGRayTracingPassBuilder::OutputReadWrite(RDGBufferHandle buffer, uint32_t offset, uint32_t size)
    {
        RDGBufferEdgeRef edge = graph->CreateEdge<RDGBufferEdge>();
        edge->state = RESOURCE_STATE_UNORDERED_ACCESS;
        edge->offset = offset;
        edge->size = size;
        edge->asOutputReadWrite = true;
        edge->type = RESOURCE_TYPE_RW_BUFFER;

        graph->Link(pass, graph->GetNode(buffer.ID()), edge);

        return *this;
    }

    RDGRayTracingPassBuilder& RDGRayTracingPassBuilder::OutputReadWrite(RDGTextureHandle texture, TextureSubresourceRange subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_UNORDERED_ACCESS;
        edge->subresource = subresource;
        edge->asOutputReadWrite = true;
        edge->type = RESOURCE_TYPE_RW_TEXTURE;

        graph->Link(pass, graph->GetNode(texture.ID()), edge);

        return *this;
    }

    RDGRayTracingPassBuilder& RDGRayTracingPassBuilder::Execute(const RDGPassExecuteFunc& execute)
    {
        pass->execute = execute;
        return *this;
    }

    RDGPresentPassBuilder& RDGPresentPassBuilder::Texture(RDGTextureHandle texture, TextureSubresourceLayers subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_TRANSFER_SRC;
        edge->subresourceLayer = subresource;

        graph->Link(graph->GetNode(texture.ID()), pass, edge);

        return *this;
    }

    RDGPresentPassBuilder& RDGPresentPassBuilder::PresentTexture(RDGTextureHandle texture)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_PRESENT;
        edge->asPresent = true;

        graph->Link(graph->GetNode(texture.ID()), pass, edge);

        return *this;
    }

    RDGCopyPassBuilder& RDGCopyPassBuilder::From(RDGTextureHandle texture, TextureSubresourceLayers subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_TRANSFER_SRC;
        edge->subresourceLayer = subresource;
        edge->asTransferSrc = true;

        graph->Link(graph->GetNode(texture.ID()), pass, edge);

        return *this;
    }

    RDGCopyPassBuilder& RDGCopyPassBuilder::To(RDGTextureHandle texture, TextureSubresourceLayers subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_TRANSFER_DST;
        edge->subresourceLayer = subresource;
        edge->asTransferDst = true;

        graph->Link(pass, graph->GetNode(texture.ID()), edge);

        return *this;
    }

    RDGCopyPassBuilder& RDGCopyPassBuilder::GenerateMips()
    {
        pass->generateMip = true;
        return *this;
    }

    RDGCopyPassBuilder& RDGCopyPassBuilder::OutputRead(RDGTextureHandle texture, TextureSubresourceLayers subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_UNORDERED_ACCESS;
        edge->subresourceLayer = subresource;
        edge->asOutputRead = true;
        edge->type = RESOURCE_TYPE_TEXTURE;

        graph->Link(pass, graph->GetNode(texture.ID()), edge);

        return *this;
    }

    RDGCopyPassBuilder& RDGCopyPassBuilder::OutputReadWrite(RDGTextureHandle texture, TextureSubresourceLayers subresource)
    {
        RDGTextureEdgeRef edge = graph->CreateEdge<RDGTextureEdge>();
        edge->state = RESOURCE_STATE_UNORDERED_ACCESS;
        edge->subresourceLayer = subresource;
        edge->asOutputReadWrite = true;
        edge->type = RESOURCE_TYPE_RW_TEXTURE;

        graph->Link(pass, graph->GetNode(texture.ID()), edge);

        return *this;
    }

}