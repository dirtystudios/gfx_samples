//
//  main.cpp
//  render_backend
//
//  Created by Eugene Sturm on 10/7/18.
//  Copyright © 2018 Eugene Sturm. All rights reserved.
//

#include <iostream>
#include "MetalBackend.h"
#include "RenderDevice.h"
#include "RenderPassCommandBuffer.h"
#include "ComputePassCommandBuffer.h"
#import <SDL.h>
#import <SDL_syswm.h>
#include <set>
#include <unordered_set>
#include <queue>

class FrameGraphPassBuilder;
class FrameGraphPass;

using FrameGraphPassSetupDelegate = std::function<void(FrameGraphPassBuilder*)>;
using FrameGraphPassExecutionDelegate = std::function<void(const FrameGraphPass*, gfx::RenderPassCommandBuffer*)>;
using FrameGraphPassGraphicsExecutionDelegate = std::function<void(const FrameGraphPass*, gfx::RenderPassCommandBuffer*)>;
using FrameGraphPassComputeExecutionDelegate = std::function<void(const FrameGraphPass*, gfx::ComputePassCommandBuffer*)>;

#include <iostream>
#include <fstream>

bool readFileContents(const std::string& fpath, std::string* output) {

    std::ifstream fin(fpath, std::ios::in | std::ios::binary);
    
    if (fin.fail()) {
        return false;
    }
    
    output->insert(end(*output), std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>());
    
    return true;
}

enum class SizeType : uint8_t
{
    SwapchainRelative,
    Absolute
};

struct FrameGraphAttachmentDesc
{
    SizeType sizeType { SizeType::SwapchainRelative };
    float width { 1.f };
    float height { 1.f };
    gfx::LoadAction loadAction { gfx::LoadAction::DontCare };
    gfx::StoreAction storeAction { gfx::StoreAction::DontCare };
    gfx::PixelFormat format { gfx::PixelFormat::R8Uint }; // TODO: add undefined?
    
    std::array<float, 4> clearColor { 0.0, 0.0, 0.0, 0.0 };
    float clearDepth { 1.f };
    float clearStencil { 0.f };
};

class FrameGraphPass;

enum class FrameGraphResourceType : uint8_t
{
    Texture,
    DepthStencil,
};

class FrameGraphResource
{
public:
    FrameGraphResource(FrameGraphResourceType type, const FrameGraphAttachmentDesc& desc, const std::string& name = "")
    : _type(type)
    , _desc(desc)
    , _name(name)
    {}
    
    void addWritePass(const FrameGraphPass* pass) { _writePasses.insert(pass); }
    void removeWritePass(const FrameGraphPass* pass) { _writePasses.erase(pass); }
    void addReadPass(const FrameGraphPass* pass) { _readPasses.insert(pass); }
    void removeReadPass(const FrameGraphPass* pass) { _readPasses.erase(pass); }
    
    const std::set<const FrameGraphPass*>& readPasses() const { return _readPasses; }
    const std::set<const FrameGraphPass*>& writePasses() const { return _writePasses; }
    
    FrameGraphResourceType type() const { return _type; }
    const std::string& name() const { return _name; }
    const FrameGraphAttachmentDesc& desc() const { return _desc; }
private:
    FrameGraphResourceType _type;
    FrameGraphAttachmentDesc _desc;
    std::string _name;
    std::set<const FrameGraphPass*> _readPasses;
    std::set<const FrameGraphPass*> _writePasses;
};

class FrameGraphResourceRegistry
{
private:
    std::map<std::string, std::unique_ptr<FrameGraphResource>> _resources;
public:
    FrameGraphResource* getTextureResource(const std::string& name) const
    {
        FrameGraphResource* resource = nullptr;
        auto it = _resources.find(name);
        if (it != end(_resources)) {
            resource = it->second.get();
        }
         
        return resource;
    }
    
    FrameGraphResource* createDepthStencilResource(const std::string& name, const FrameGraphAttachmentDesc& desc)
    {
        FrameGraphResource* resource = getTextureResource(name);
        if (resource == nullptr) {
            resource = new FrameGraphResource(FrameGraphResourceType::DepthStencil, desc, name);
            _resources.emplace(name, resource);
        }
        return resource;
    }
    FrameGraphResource* createTextureResource(const std::string& name, const FrameGraphAttachmentDesc& desc)
    {
        FrameGraphResource* resource = getTextureResource(name);
        if (resource == nullptr) {
            resource = new FrameGraphResource(FrameGraphResourceType::Texture, desc, name);
            _resources.emplace(name, resource);
        }
        
        return resource;
    }
};

class FrameGraphPass
{
private:
    std::string _name;
    std::unordered_set<FrameGraphResource*> _inputs;
    std::unordered_set<FrameGraphResource*> _outputs;
    gfx::RenderPassId _passId { gfx::NULL_ID };
public:
    FrameGraphPass(const std::string& name,
                   std::unordered_set<FrameGraphResource*>&& inputs,
                   std::unordered_set<FrameGraphResource*>&& outputs)
    : _name(name)
    , _inputs(inputs)
    , _outputs(outputs)
    {
        for (FrameGraphResource* resource : _inputs) {
            resource->addReadPass(this);
        }
        
        for (FrameGraphResource* resource : _outputs) {
            resource->addWritePass(this);
        }
    }
    
    void setPassId(gfx::RenderPassId passId) { _passId = passId; }
    gfx::RenderPassId passId() const { return _passId; }
    
    virtual bool isCompute() const { return false; }
    
    virtual void execute(gfx::RenderPassCommandBuffer* commandBuffer) const {}
    virtual void execute(gfx::ComputePassCommandBuffer* commandBuffer) const {}
    
    const std::unordered_set<FrameGraphResource*>& outputs() const { return _outputs; }
    const std::unordered_set<FrameGraphResource*>& inputs() const { return _inputs; }
    
    const std::string& name() const { return _name; }
};

class FrameGraphGraphicsPass : public FrameGraphPass
{
private:
    FrameGraphPassGraphicsExecutionDelegate _delegate;
public:
    FrameGraphGraphicsPass(const std::string& name,
                           std::unordered_set<FrameGraphResource*>&& inputs,
                           std::unordered_set<FrameGraphResource*>&& outputs,
                           FrameGraphPassGraphicsExecutionDelegate delegate)
    : FrameGraphPass(name, std::move(inputs), std::move(outputs))
    , _delegate(delegate)
    {}
    
    virtual void execute(gfx::RenderPassCommandBuffer* commandBuffer) const override { _delegate(this, commandBuffer); }
};

class FrameGraphComputePass : public FrameGraphPass
{
private:
    FrameGraphPassComputeExecutionDelegate _delegate;
public:
    FrameGraphComputePass(const std::string& name,
                           std::unordered_set<FrameGraphResource*>&& inputs,
                           std::unordered_set<FrameGraphResource*>&& outputs,
                           FrameGraphPassComputeExecutionDelegate delegate)
    : FrameGraphPass(name, std::move(inputs), std::move(outputs))
    , _delegate(delegate)
    {}
    
    virtual bool isCompute() const override { return true; }
    virtual void execute(gfx::ComputePassCommandBuffer* commandBuffer) const override { _delegate(this, commandBuffer); }
};

class FrameGraphPassBuilder
{
private:
    std::unordered_set<FrameGraphResource*> _inputResources;
    std::unordered_set<FrameGraphResource*> _outputResources;
    std::string _name;
    FrameGraphPassGraphicsExecutionDelegate _graphicsDelegate;
    FrameGraphPassComputeExecutionDelegate _computeDelegate;
public:
    FrameGraphPassBuilder(const std::string& name, FrameGraphPassExecutionDelegate delegate)
    : _name(name)
    , _graphicsDelegate(delegate)
    {}
    
    FrameGraphPassBuilder(const std::string& name, FrameGraphPassComputeExecutionDelegate delegate)
    : _name(name)
    , _computeDelegate(delegate)
    {}
       
    void read(FrameGraphResource* resource)
    {
        _inputResources.insert(resource);
    }
    
    void write(FrameGraphResource* resource)
    {
        _outputResources.insert(resource);
    }
    
    FrameGraphPass* build()
    {
        if (_graphicsDelegate) {
            return new FrameGraphGraphicsPass(_name, std::move(_inputResources), std::move(_outputResources), _graphicsDelegate);
        } else {
            return new FrameGraphComputePass(_name, std::move(_inputResources), std::move(_outputResources), _computeDelegate);
        }
    }
};


class FrameGraph
{
private:
    FrameGraphResourceRegistry* _registry;
    FrameGraphResource* _backBuffer { nullptr };
    std::vector<FrameGraphPass*> _passes;
    std::map<const FrameGraphResource*, gfx::ResourceId> _resourceCache;
    std::map<const FrameGraphPass*, gfx::RenderPassId> _passCache;
public:
    FrameGraph()
    {
        _registry = new FrameGraphResourceRegistry();
    }
    
    FrameGraphPass* addPass(const std::string& name,
                            FrameGraphPassSetupDelegate setupDelegate,
                            FrameGraphPassExecutionDelegate executionDelegate)
    {
        FrameGraphPassBuilder builder = FrameGraphPassBuilder(name, executionDelegate);
        setupDelegate(&builder);
        FrameGraphPass* pass = builder.build();
        _passes.emplace_back(pass);
        return pass;
    }
    
    void addGraphicsPass(const std::string& name, FrameGraphPassSetupDelegate setupDelegate, FrameGraphPassGraphicsExecutionDelegate executionDelegate)
    {
        FrameGraphPassBuilder builder = FrameGraphPassBuilder(name, executionDelegate);
        setupDelegate(&builder);
        FrameGraphPass* pass = builder.build();
        _passes.emplace_back(pass);
    }
    
    void addComputePass(const std::string& name, FrameGraphPassSetupDelegate setupDelegate, FrameGraphPassComputeExecutionDelegate executionDelegate)
    {
        FrameGraphPassBuilder builder = FrameGraphPassBuilder(name, executionDelegate);
        setupDelegate(&builder);
        FrameGraphPass* pass = builder.build();
        _passes.emplace_back(pass);
    }
    
    void execute(gfx::RenderDevice* device, gfx::CommandBuffer* commandBuffer, gfx::TextureId backbuffer)
    {
        std::vector<FrameGraphPass*> order = bake();
        std::unordered_set<FrameGraphResource*> resources;
        _resourceCache[backBufferResource()] = backbuffer;
           
       for (const FrameGraphPass* pass : order) {
           resources.insert(begin(pass->inputs()), end(pass->inputs()));
           resources.insert(begin(pass->outputs()), end(pass->outputs()));
       }
        
        for (const FrameGraphResource* resource : resources) {
            auto it = _resourceCache.find(resource);
            if (it != end(_resourceCache)) {
                continue;
            }
            
            gfx::TextureUsageFlags flags;
            flags |= resource->readPasses().size() > 0 ? gfx::TextureUsageBitShaderRead : 0;
            flags |= resource->writePasses().size() > 0 ? gfx::TextureUsageBitRenderTarget : 0;
                
            const FrameGraphAttachmentDesc& desc = resource->desc();

            uint32_t width = 0;
            uint32_t height = 0;
            switch(desc.sizeType) {
                case SizeType::SwapchainRelative:
                {
                    width = static_cast<uint32_t>(desc.width * _backBuffer->desc().width);
                    height = static_cast<uint32_t>(desc.height * _backBuffer->desc().height);
                    break;
                }
                case SizeType::Absolute:
                {
                    width = static_cast<uint32_t>(desc.width);
                    height = static_cast<uint32_t>(desc.height);
                    break;

                }
            }
            
            gfx::TextureId texture = device->CreateTexture2D(desc.format, flags, width, height, nullptr, resource->name().c_str());
            _resourceCache[resource] = texture;
        }
        
        for (FrameGraphPass* pass : order) {
            auto it = _passCache.find(pass);
            if (it != end(_passCache)) {
                continue;
            }
            gfx::RenderPassInfo passInfo;
            passInfo.setLabel(pass->name());
            
            size_t index = 0;
            for (FrameGraphResource* resource : pass->outputs()) {
                if (resource->type() == FrameGraphResourceType::Texture) {
                    gfx::ColorAttachmentDesc desc;
                    desc.format = resource->desc().format;
                    desc.loadAction = resource->desc().loadAction;
                    desc.storeAction = resource->desc().storeAction;
                    desc.clearColor = resource->desc().clearColor;
                    desc.index = index++;
                    passInfo.addColorAttachment(desc);
                } else if (resource->type() == FrameGraphResourceType::DepthStencil) {
                    {
                        gfx::DepthAttachmentDesc desc;
                        desc.format = resource->desc().format;
                        desc.loadAction = resource->desc().loadAction;
                        desc.storeAction = resource->desc().storeAction;
                        desc.clearDepth = resource->desc().clearDepth;
                        passInfo.setDepthAttachment(desc);
                    }
                    
                    {
                        gfx::StencilAttachmentDesc desc;
                        desc.format = resource->desc().format;
                        desc.loadAction = resource->desc().loadAction;
                        desc.storeAction = resource->desc().storeAction;
                        desc.clearStencil = resource->desc().clearStencil;
                        passInfo.setStencilAttachment(desc);
                    }
                } else {
                    assert(false);
                }
            }
            
            gfx::RenderPassId passId = device->CreateRenderPass(passInfo);
            _passCache[pass] = passId;
            pass->setPassId(passId);
        }
            
        for (const FrameGraphPass* pass : order) {
            auto passIt = _passCache.find(pass);
            if (passIt == end(_passCache)) {
                assert(false);
            }
            
            if (pass->isCompute()) {
                gfx::ComputePassCommandBuffer* passCommandBuffer = commandBuffer->beginComputePass(pass->name());
                pass->execute(passCommandBuffer);
                commandBuffer->endComputePass(passCommandBuffer);
            } else {
                gfx::FrameBuffer frameBuffer;
                size_t index = 0;

                for (FrameGraphResource* resource : pass->outputs()) {
                   auto resourceIt = _resourceCache.find(resource);
                   if (resourceIt == end(_resourceCache)) {
                       assert(false);
                   }
                   
                   if (resource->type() == FrameGraphResourceType::Texture) {
                       frameBuffer.setColorAttachment(resourceIt->second, index++);
                   } else if (resource->type() == FrameGraphResourceType::DepthStencil) {
                       frameBuffer.setDepthAttachment(resourceIt->second);
                       frameBuffer.setStencilAttachment(resourceIt->second);
                   } else {
                       assert(false);
                   }
                }


                gfx::RenderPassId passId = passIt->second;
                gfx::RenderPassCommandBuffer* passCommandBuffer = commandBuffer->beginRenderPass(passId, frameBuffer, pass->name());
                pass->execute(passCommandBuffer);
                commandBuffer->endRenderPass(passCommandBuffer);
            }
           
        }
    }
    
    FrameGraphResource* setBackBuffer(const std::string& name, const FrameGraphAttachmentDesc& attachmentDesc)
    {
        _backBuffer = _registry->createTextureResource(name, attachmentDesc);
        return _backBuffer;
    }
    
    FrameGraphResource* backBufferResource() const { return _backBuffer; }
    
    FrameGraphResourceRegistry* registry() { return _registry; }

private:
    std::vector<FrameGraphPass*> bake()
    {
        std::vector<FrameGraphPass*> order;
        std::set<FrameGraphPass*> existing;
        
        if (_backBuffer == nullptr) {
            return order;
        }
        
        std::map<FrameGraphResource* , std::set<FrameGraphPass*>> writePasses;
        std::map<FrameGraphResource* , std::set<FrameGraphPass*>> readPasses;
        std::vector<FrameGraphPass*> independentPasses;
        for (FrameGraphPass* pass : _passes) {
            if (pass->inputs().empty() && pass->outputs().empty()) {
                independentPasses.emplace_back(pass);
                continue;
            }
            
            for (FrameGraphResource* resource : pass->outputs()) {
                writePasses[resource].insert(pass);
            }
            
            for (FrameGraphResource* resource : pass->inputs()) {
                readPasses[resource].insert(pass);
            }
        }
        
        std::queue<FrameGraphResource*> q;
        q.push(_backBuffer);
        
        while (q.empty() == false) {
            FrameGraphResource* resource = q.front();
            q.pop();
            
            for (FrameGraphPass* pass : writePasses[resource]) {
                if (existing.count(pass) == 0) {
                    order.emplace_back(pass);
                    existing.insert(pass);
                    
                    for (FrameGraphResource* resource : pass->inputs()) {
                        q.push(resource);
                    }
                }
            }
        }
        
        for (FrameGraphPass* pass : independentPasses) {
            order.emplace_back(pass);
        }
        
        std::reverse(begin(order), end(order));
        return order;
    }
};

class FeatureRenderer
{
private:
    gfx::RenderDevice* _device { nullptr };
public:
    virtual void onFrameBegin() {}
    virtual void onPassBegin(const FrameGraphPass*) {};
    virtual void onPassPrepare(const FrameGraphPass* pass) {};
    virtual void onPassSubmit(const FrameGraphPass* pass, gfx::RenderPassCommandBuffer* commandBuffer) {};
    virtual void onPassEnd(const FrameGraphPass*) {};
    virtual void onFrameEnd() {};
};

class Renderer
{
private:
    gfx::RenderDevice* _device;
    gfx::Swapchain* _swapchain;
    
    FrameGraph _graph;
    std::vector<FeatureRenderer*> _featureRenderers;
    std::map<std::string, std::vector<FeatureRenderer*>> _renderersByPass;
public:
    Renderer()
    {
        auto executionDelegate = [&](const FrameGraphPass* pass, gfx::RenderPassCommandBuffer* commandBuffer)
        {
            auto it = _renderersByPass.find(pass->name());
            if (it != end(_renderersByPass)) {
                for (FeatureRenderer* renderer : it->second) {
                    renderer->onPassBegin(pass);
                }
                
                for (FeatureRenderer* renderer : it->second) {
                    renderer->onPassPrepare(pass);
                }
                
//                for (FeatureRenderer* renderer : it->second) {
//                    renderer->onPassSubmit(pass, commandBuffer);
//                }
                
                for (FeatureRenderer* renderer : it->second) {
                    renderer->onPassEnd(pass);
                }
            }
        };
        
        _graph.addPass("standard", [](FrameGraphPassBuilder* builder) {}, executionDelegate);
    }
    
    void frame()
    {
        
        for (FeatureRenderer* renderer : _featureRenderers) {
            renderer->onFrameBegin();
        }
        
        gfx::TextureId backbuffer = _swapchain->begin();
        gfx::CommandBuffer* commandBuffer = _device->CreateCommandBuffer();
        _graph.execute(_device, commandBuffer, backbuffer);
        _swapchain->present(backbuffer);
        
        for (FeatureRenderer* renderer : _featureRenderers) {
            renderer->onFrameEnd();
        }
        
    }
    
};


int main(int argc, const char * argv[])
{
    std::unique_ptr<gfx::RenderBackend> renderBackend(new gfx::MetalBackend());
    gfx::RenderDevice* device = renderBackend->getRenderDevice();
    
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0) {
        return -1;
    }
    
    SDL_Window* window = SDL_CreateWindow("Rawr", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_RESIZABLE);
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    SDL_GetWindowWMInfo(window, &info);
    NSWindow* hndl = (NSWindow*)info.info.cocoa.window;
    
    gfx::SwapchainDesc swapchainDesc;
    swapchainDesc.format = gfx::PixelFormat::BGRA8Unorm;
    swapchainDesc.width = 1024;
    swapchainDesc.height = 768;
    
    gfx::Swapchain* swapchain = renderBackend->createSwapchainForWindow(swapchainDesc, device, (void*)hndl);

    FrameGraphAttachmentDesc finalDesc, depthDesc;
    finalDesc.sizeType = SizeType::Absolute;
    finalDesc.width = swapchainDesc.height;
    finalDesc.height = swapchainDesc.width;
    finalDesc.format = swapchain->pixelFormat();
    finalDesc.loadAction = gfx::LoadAction::Clear;
    finalDesc.clearColor = { 0, 1, 0, 1 };
    finalDesc.storeAction = gfx::StoreAction::DontCare;
    
    depthDesc.format = gfx::PixelFormat::Depth32FloatStencil8;
    depthDesc.loadAction = gfx::LoadAction::Clear;
    depthDesc.storeAction = gfx::StoreAction::Store;
    
    
    FrameGraph graph;
    graph.setBackBuffer("final", finalDesc);
    FrameGraphResource* depthStencilResource = graph.registry()->createDepthStencilResource("depth", depthDesc);
    FrameGraphResource* backbufferIntermediate = graph.registry()->createTextureResource("aa", finalDesc);
    
    auto pass0Setup = [&](FrameGraphPassBuilder* builder)
    {
        
    };
    
    auto pass0Execution = [=](const FrameGraphPass* pass, gfx::ComputePassCommandBuffer* commandBuffer)
    {
        gfx::ComputePipelineStateDesc cpsd;
        cpsd.computeShader = device->GetShader(gfx::ShaderType::ComputeShader, "test");
        
        gfx::PipelineStateId ps = device->CreateComputePipelineState(cpsd);
        
        commandBuffer->setPipelineState(ps);
    };
    
    auto pass1Setup = [&](FrameGraphPassBuilder* builder)
    {
        builder->write(backbufferIntermediate);
        builder->write(depthStencilResource);
    };
    auto pass1Execution = [&](const FrameGraphPass* pass, gfx::RenderPassCommandBuffer* commandBuffer)
    {
        gfx::PipelineStateDesc psd;
        psd.renderPass = pass->passId();
        psd.vertexShader = device->GetShader(gfx::ShaderType::VertexShader, "fsq");
        psd.pixelShader = device->GetShader(gfx::ShaderType::PixelShader, "clear");
        psd.topology = gfx::PrimitiveType::Triangles;
        psd.rasterState.cullMode = gfx::CullMode::Back;
        psd.rasterState.windingOrder = gfx::WindingOrder::FrontCCW;
        psd.rasterState.fillMode = gfx::FillMode::Solid;
        
        gfx::PipelineStateId psId = device->CreatePipelineState(psd);
        
        commandBuffer->setViewport(0, 0, 50, 50);
        commandBuffer->setPipelineState(psId);
        commandBuffer->drawPrimitives(0, 3);
    };
    
    
    auto pass2Setup = [&](FrameGraphPassBuilder* builder)
    {
        builder->read(backbufferIntermediate);
        builder->write(graph.backBufferResource());
        builder->write(depthStencilResource);
    };
    auto pass2Execution = [&](const FrameGraphPass* pass, gfx::RenderPassCommandBuffer* commandBuffer)
    {
        gfx::PipelineStateDesc psd;
        psd.renderPass = pass->passId();
        psd.vertexShader = device->GetShader(gfx::ShaderType::VertexShader, "fsq");
        psd.pixelShader = device->GetShader(gfx::ShaderType::PixelShader, "clear");
        psd.topology = gfx::PrimitiveType::Triangles;
        psd.rasterState.cullMode = gfx::CullMode::Back;
        psd.rasterState.windingOrder = gfx::WindingOrder::FrontCCW;
        psd.rasterState.fillMode = gfx::FillMode::Solid;

        gfx::PipelineStateId psId = device->CreatePipelineState(psd);

        commandBuffer->setViewport(0, 0, 50, 50);
        commandBuffer->setPipelineState(psId);
        commandBuffer->drawPrimitives(0, 3);
    };
    
    gfx::ShaderData data;
    std::string output;
    readFileContents("/Users/sturm/src/github/gfx_samples/build/test.metal", &output);
    data.data = output.data();
    data.len = output.length();
    data.type = gfx::ShaderDataType::Source;
    
    device->AddOrUpdateShaders({data});
    
    graph.addGraphicsPass("pass1", pass1Setup, pass1Execution);
    graph.addGraphicsPass("pass2", pass2Setup, pass2Execution);
    graph.addComputePass("pass0", pass0Setup, pass0Execution);
    
    bool shouldQuit = false;
    while (shouldQuit == false) {
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
                case SDL_QUIT: {
                    shouldQuit = true;
                    break;
                }
                case SDL_WINDOWEVENT: {
                    if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                        const uint32_t w = (uint32_t)e.window.data1;
                        const uint32_t h = (uint32_t)e.window.data2;
                        
                        swapchain->resize(w, h);
                    }
                    break;
                }
            }
        }
        
        gfx::TextureId backbuffer = swapchain->begin();
        gfx::CommandBuffer* commandBuffer = device->CreateCommandBuffer();
        graph.execute(device, commandBuffer, backbuffer);
        device->Submit({commandBuffer});
        swapchain->present(backbuffer);
    }
    
    
    
    
    
    return 0;
}
