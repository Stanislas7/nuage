$FG_GLSL_VERSION

layout(location = 0) out float fragColor;

in VS_OUT {
    float flogz;
} fs_in;

// logarithmic_depth.glsl
float logdepth_encode(float z);

void main()
{
    // Mark highlighted objects with 1.
    // The pass takes care of clearing the buffer 0.
    fragColor = 1.0;
    gl_FragDepth = logdepth_encode(fs_in.flogz);
}
