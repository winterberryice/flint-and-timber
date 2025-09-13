struct CameraUniform {
    view_proj: mat4x4<f32>,
};
@group(0) @binding(0) var<uniform> camera: CameraUniform;

@group(1) @binding(0) var t_diffuse: texture_2d<f32>;
@group(1) @binding(1) var s_diffuse: sampler;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) tex_coords: vec2<f32>,
    @location(2) normal: vec3<f32>,
    @location(3) light: f32,
};

struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) tex_coords: vec2<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) light: f32,
};

@vertex
fn vs_main(model: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.clip_position = camera.view_proj * vec4<f32>(model.position, 1.0);
    out.tex_coords = model.tex_coords;
    out.normal = model.normal;
    out.light = model.light;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let tex_color = textureSample(t_diffuse, s_diffuse, in.tex_coords);

    // Basic ambient light + directional light from above
    let ambient = 0.4;
    let sun_dir = normalize(vec3<f32>(0.8, 1.0, 0.6));
    let diffuse = max(dot(in.normal, sun_dir), 0.0);
    let total_light = clamp(ambient + diffuse, 0.0, 1.0);

    return vec4<f32>(tex_color.rgb * total_light, tex_color.a);
}
