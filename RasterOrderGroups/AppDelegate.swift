//
//  AppDelegate.swift
//  RasterOrderGroups
//
//  Created by Myles C. Maxfield on 9/16/19.
//  Copyright © 2019 Myles C. Maxfield. All rights reserved.
//

import Cocoa
import Metal

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate {
    @IBOutlet weak var window: NSWindow!

    func applicationDidFinishLaunching(_ aNotification: Notification) {
        let width = 16
        let height = 16

        var device: MTLDevice!
        for candidate in MTLCopyAllDevices() {
            if candidate.areRasterOrderGroupsSupported {
                device = candidate
                break
            }
        }
        assert(device != nil)
        let library = device.makeDefaultLibrary()!
        let vertexShader = library.makeFunction(name: "vertexShader");
        let fragmentShader = library.makeFunction(name: "fragmentShader");
        
        let renderPipelineDescriptor = MTLRenderPipelineDescriptor()
        renderPipelineDescriptor.vertexFunction = vertexShader
        renderPipelineDescriptor.fragmentFunction = fragmentShader
        let vertexDescriptor = MTLVertexDescriptor()
        vertexDescriptor.attributes[0].format = .float2
        vertexDescriptor.attributes[0].offset = 0
        vertexDescriptor.attributes[0].bufferIndex = 0
        vertexDescriptor.layouts[0].stride = MemoryLayout<Float>.size * 2
        renderPipelineDescriptor.vertexDescriptor = vertexDescriptor
        renderPipelineDescriptor.inputPrimitiveTopology = .triangle
        let pipelinePipelineState = try! device.makeRenderPipelineState(descriptor: renderPipelineDescriptor)

        let commandQueue = device.makeCommandQueue()!
        let commandBuffer = commandQueue.makeCommandBuffer()!

        let overlappingTriangleCount = 100

        var vertexData = Array(repeating: SIMD2<Float>(), count: 6 * overlappingTriangleCount)
        for i in 0 ..< overlappingTriangleCount {
            vertexData[6 * i + 0] = SIMD2<Float>(-1, -1)
            vertexData[6 * i + 1] = SIMD2<Float>(-1, 1)
            vertexData[6 * i + 2] = SIMD2<Float>(1, -1)
            vertexData[6 * i + 3] = SIMD2<Float>(1, -1)
            vertexData[6 * i + 4] = SIMD2<Float>(-1, 1)
            vertexData[6 * i + 5] = SIMD2<Float>(1, 1)
        }
        let vertexBuffer = device.makeBuffer(bytes: vertexData, length: MemoryLayout<SIMD2<Float>>.size * vertexData.count, options: .storageModeShared)!

        let textureDescriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .r32Uint, width: width, height: height, mipmapped: false)
        textureDescriptor.usage = [.shaderRead, .shaderWrite]
        let texture = device.makeTexture(descriptor: textureDescriptor)!
        let textureData = Array(repeating: UInt32(Float(UInt8.max) * 0.25), count: width * height)
        texture.replace(region: MTLRegion(origin: MTLOrigin(x: 0, y: 0, z: 0), size: MTLSize(width: width, height: height, depth: 1)), mipmapLevel: 0, withBytes: textureData, bytesPerRow: MemoryLayout<UInt32>.size * width)

        let renderPassDescriptor = MTLRenderPassDescriptor()
        renderPassDescriptor.renderTargetWidth = width
        renderPassDescriptor.renderTargetHeight = height
        renderPassDescriptor.defaultRasterSampleCount = 1
        let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor)!
        renderEncoder.setViewport(MTLViewport(originX: 0, originY: 0, width: Double(width), height: Double(height), znear: 0, zfar: 1))
        renderEncoder.setRenderPipelineState(pipelinePipelineState)
        renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        let uniforms = [UInt32(width), UInt32(height)]
        renderEncoder.setVertexBytes(uniforms, length: MemoryLayout<UInt32>.size * uniforms.count, index: 1)
        renderEncoder.setFragmentTexture(texture, index: 0)
        renderEncoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 6 * overlappingTriangleCount)
        renderEncoder.endEncoding()
        let blitEncoder = commandBuffer.makeBlitCommandEncoder()!
        blitEncoder.synchronize(resource: texture)
        blitEncoder.endEncoding()
        commandBuffer.addCompletedHandler {(commandBuffer) in
            var result = Array(repeating: UInt32(), count: width * height)
            texture.getBytes(&result, bytesPerRow: MemoryLayout<UInt32>.size * width, from: MTLRegion(origin: MTLOrigin(x: 0, y: 0, z: 0), size: MTLSize(width: width, height: height, depth: 1)), mipmapLevel: 0)
            print("\(result)")
        }
        commandBuffer.commit()
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        // Insert code here to tear down your application
    }


}

