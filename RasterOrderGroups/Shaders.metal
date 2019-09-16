//
//  Shaders.metal
//  RasterOrderGroups
//
//  Created by Myles C. Maxfield on 9/16/19.
//  Copyright © 2019 Myles C. Maxfield. All rights reserved.
//

#include <metal_stdlib>
using namespace metal;

struct VertexShaderInput {
    float2 pos [[attribute(0)]];
};

struct VertexShaderOutput {
    float4 position [[position]];
    float2 tex [[id(0)]];
};

struct Uniforms {
    uint width;
    uint height;
};

vertex VertexShaderOutput vertexShader(VertexShaderInput input [[stage_in]], constant Uniforms& uniforms [[buffer(1)]]) {
    VertexShaderOutput output;
    output.position = float4(input.pos, 0, 1);
    output.tex = (input.pos + 1) / 2 * float2(uniforms.width, uniforms.height);
    return output;
}

struct PixelShaderInput {
    float2 tex [[id(0)]];
};

fragment void fragmentShader(PixelShaderInput input [[stage_in]], texture2d<uint, access::read_write> theTexture [[texture(0), raster_order_group(0)]]) {
    theTexture.write(uint4(theTexture.read(uint2(input.tex)).x + 1, 0, 0, 1), uint2(input.tex));
}

