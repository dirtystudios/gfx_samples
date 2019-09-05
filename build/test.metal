//
//  test.metal
//  test
//
//  Created by Eugene Sturm on 8/29/19.
//

#include <metal_stdlib>
using namespace metal;

struct VertexOut
{
    float4 position[[position]];
    float2 uv;
};

vertex VertexOut fsq(uint vid [[vertex_id]])
{
    VertexOut vout;
    vout.uv = float2((vid << 1) & 2, vid & 2);
    vout.position = float4(vout.uv * 2.0f + -1.0f, 0.0f, 1.0f);
    return vout;
}

fragment float4 clear(VertexOut varyingInput[[stage_in]])
{
    return float4(1);
}

kernel void test(uint2 gid [[thread_position_in_grid]])
{
    
}

