#ifdef VERTEX
    in vec4 vx_position;
    in vec4 vx_color;
    in vec2 vx_uv;

    out vec4 color;
    out vec2 uv;

    void main() {
        color = vx_color;
        uv = vx_uv;
        gl_Position = vec4(vx_position.xy, 0.0, 1.0);
    }
#endif

#ifdef PIXEL
    in vec4 color;
    in vec2 uv;
    
    out vec4 out_color;

    uniform sampler2D image;
    uniform bool dither;

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

    void main() {
        vec4 o = texture2D(image, uv) * (color / 255.0);

        if (dither4x4(gl_FragCoord.xy, o.a) < 0.5) 
            discard;
            
        out_color = o;
    }
#endif

