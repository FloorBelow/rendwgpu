struct VertexInput {
    @location(0) position: vec3f,
    @location(1) color: vec3f,
    @location(2) modelx: vec4<f32>,
    @location(3) modely: vec4<f32>,
    @location(4) modelz: vec4<f32>,
    @location(5) modelw: vec4<f32>
};

/**
 * A structure with fields labeled with builtins and locations can also be used
 * as *output* of the vertex shader, which is also the input of the fragment
 * shader.
 */
struct VertexOutput {
    @builtin(position) position: vec4f,
    // The location here does not refer to a vertex attribute, it just means
    // that this field must be handled by the rasterizer.
    // (It can also refer to another field of another struct that would be used
    // as input to the fragment shader.)
    @location(0) color: vec3f,
};

struct Uniforms {
    proj: mat4x4<f32>,
    view: mat4x4<f32>,
    time: f32,
};

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
//@group(0) @binding(1) var<uniform> model: mat4x4<f32>;


@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    let model = mat4x4<f32>(in.modelx, in.modely, in.modelz, in.modelw);
    out.position = uniforms.proj * uniforms.view * model * vec4<f32>(in.position, 1.0);
    out.color = in.color; // forward to the fragment shader
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return vec4f(in.color.xzy, 1.0);
}