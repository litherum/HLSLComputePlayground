//
//  Shaders.metal
//  MSLComputePlayground
//
//  Created by Myles C. Maxfield on 8/29/19.
//  Copyright © 2019 Myles C. Maxfield. All rights reserved.
//

#include <metal_stdlib>
using namespace metal;

kernel void computeShader(device float4x4* inputBuffer [[buffer(0)]], device float4x4* outputBuffer [[buffer(1)]]) {
    outputBuffer[0] = inputBuffer[0] * inputBuffer[1];
}
