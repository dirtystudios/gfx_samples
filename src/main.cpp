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
#import <SDL.h>
#import <SDL_syswm.h>

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
    swapchainDesc.height = 1024;
    swapchainDesc.width = 768/2;
    
    gfx::Swapchain* swapchain = renderBackend->createSwapchainForWindow(swapchainDesc, device, (void*)hndl);
    
    gfx::RenderPassInfo renderPassInfo;
    
    gfx::ColorAttachmentDesc colorAttachment;
    colorAttachment.format = swapchain->pixelFormat();
    colorAttachment.loadAction = gfx::LoadAction::Clear;
    colorAttachment.storeAction = gfx::StoreAction::DontCare;
    colorAttachment.clearColor = { 0, 1, 0, 1 };
    colorAttachment.index = 0;
    renderPassInfo.addColorAttachment(colorAttachment);
    gfx::RenderPassId renderPassId = device->CreateRenderPass(renderPassInfo);        
    
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
        gfx::FrameBuffer frameBuffer;        
        frameBuffer.setColorAttachment(backbuffer, 0);
        
        gfx::RenderPassCommandBuffer* renderPassCommandBuffer = commandBuffer->beginRenderPass(renderPassId, frameBuffer, "rawr");
        commandBuffer->endRenderPass(renderPassCommandBuffer);
        
        device->Submit({commandBuffer});
        swapchain->present(backbuffer);        
    }
    
    
    
    
    
    return 0;
}
