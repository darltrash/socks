#ifdef COMPAT_MODE
    #define out varying

    #ifdef VERTEX
        #define in attribute
    #elif PIXEL
        #define in varying
    #endif
#endif

float dither4x4( vec2 position, float brightness ) {
    mat4 dither_table = mat4 (
        0.0625, 0.5625, 0.1875, 0.6875, 
        0.8125, 0.3125, 0.9375, 0.4375, 
        0.2500, 0.7500, 0.1250, 0.6250, 
        1.0000, 0.5000, 0.8750, 0.3750
    );

    ivec2 p = ivec2(mod(position, 4.0));
    
    float a = step(float(p.x), 3.0);
    float limit = mix(0.0, dither_table[p.y][p.x], a);

    return step(limit, brightness);
}

float luma( vec3 color ) {
    return dot(vec3(0.2126729, 0.7151522, 0.0721750), color);
}