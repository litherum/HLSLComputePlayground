//
//  AppDelegate.swift
//  MSLComputePlayground
//
//  Created by Myles C. Maxfield on 8/29/19.
//  Copyright © 2019 Myles C. Maxfield. All rights reserved.
//

import Cocoa

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate {

    @IBOutlet weak var window: NSWindow!


    func applicationDidFinishLaunching(_ aNotification: Notification) {
        let device = MTLCreateSystemDefaultDevice()!
        let function = device.makeDefaultLibrary()!.makeFunction(name: "computeShader")!
        let pipelineState = try! device.makeComputePipelineState(function: function)
        var input = Array(repeating: Float(0), count: 32)
        for i in 0 ..< input.count {
            input[i] = Float(i)
        }
        let numFloats = 16
        let inputBuffer = device.makeBuffer(bytes: &input, length: input.count * MemoryLayout<Float>.size, options: .storageModeShared)!
        let outputBuffer = device.makeBuffer(length: numFloats * MemoryLayout<Float>.size, options: .storageModeShared)!
        let commandQueue = device.makeCommandQueue()!
        let commandBuffer = commandQueue.makeCommandBuffer()!
        let commandEncoder = commandBuffer.makeComputeCommandEncoder()!
        commandEncoder.setComputePipelineState(pipelineState)
        commandEncoder.setBuffers([inputBuffer, outputBuffer], offsets: [0, 0], range: 0 ..< 2)
        commandEncoder.dispatchThreadgroups(MTLSize(width: 1, height: 1, depth: 1), threadsPerThreadgroup: MTLSize(width: 1, height: 1, depth: 1))
        commandEncoder.endEncoding()
        commandBuffer.addCompletedHandler {(commandBuffer) in
            let memory = outputBuffer.contents().bindMemory(to: Float.self, capacity: numFloats)
            for i in 0 ..< numFloats {
                print("\(i): \(memory[i])")
            }
        }
        commandBuffer.commit()
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        // Insert code here to tear down your application
    }


}

