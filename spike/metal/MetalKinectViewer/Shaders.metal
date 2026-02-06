#include <metal_stdlib>
using namespace metal;

// Vertex output
struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

// Vertex shader: fullscreen triangle
vertex VertexOut vertexShader(uint vertexID [[vertex_id]]) {
    VertexOut out;

    // Generate fullscreen triangle
    // Vertex 0: (-1, -1) bottom-left
    // Vertex 1: (3, -1) far right
    // Vertex 2: (-1, 3) far top
    float2 positions[3] = {
        float2(-1.0, -1.0),
        float2(3.0, -1.0),
        float2(-1.0, 3.0)
    };

    float2 texCoords[3] = {
        float2(0.0, 1.0),
        float2(2.0, 1.0),
        float2(0.0, -1.0)
    };

    out.position = float4(positions[vertexID], 0.0, 1.0);
    out.texCoord = texCoords[vertexID];

    return out;
}

// Fragment shader: solid color (for M1 validation)
fragment float4 fragmentShader(VertexOut in [[stage_in]]) {
    // Green color to verify rendering works
    return float4(0.0, 1.0, 0.0, 1.0);
}

// Fragment shader: textured (for M3 - RGB display)
fragment float4 textureFragment(VertexOut in [[stage_in]],
                                texture2d<float> rgbTexture [[texture(0)]],
                                sampler textureSampler [[sampler(0)]]) {
    // Sample RGB texture
    float4 color = rgbTexture.sample(textureSampler, in.texCoord);
    return color;
}

// Fragment shader: depth visualization (for M4)
fragment float4 depthFragment(VertexOut in [[stage_in]],
                              texture2d<uint> depthTexture [[texture(0)]],
                              sampler textureSampler [[sampler(0)]]) {
    // Sample depth texture (R16Uint)
    uint2 texCoord = uint2(in.texCoord * float2(depthTexture.get_width(), depthTexture.get_height()));
    uint depthValue = depthTexture.read(texCoord).r;

    // Normalize 11-bit depth (0-2047) to grayscale (0-1)
    // Invert so closer = brighter
    float normalized = 1.0 - (float(depthValue) / 2047.0);

    // Return grayscale
    return float4(normalized, normalized, normalized, 1.0);
}
