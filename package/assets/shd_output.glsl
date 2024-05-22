#ifdef VERTEX
    in vec4 vx_position;
    out vec2 uv;

    void main() {
        uv = vx_position.xy * 0.5 + 0.5;
        gl_Position = vx_position;
    }
#endif

#ifdef PIXEL
    in vec2 uv;
    
    out vec4 out_color;

    uniform sampler2D image;
    uniform bool dither;
    uniform int scale;

    vec2 curve(vec2 uv)
    {
        uv = (uv - 0.5) * 2.0;
        uv *= 1.1;	
        uv.x *= 1.0 + pow((abs(uv.y) / 9.0), 2.0);
        uv.y *= 1.0 + pow((abs(uv.x) / 8.0), 2.0);
        uv  = (uv / 2.0) + 0.5;
        uv =  uv * 0.92 + 0.04;
        return uv;
    }

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
        if (!dither) {
            out_color = texture2D(image, uv);
            return;
        }

        vec4 o = vec4(1.0);

        o.rg = texture2D(image, uv).rg;
        o.b = texture2D(image, uv+vec2(0.001, 0.0005)).b;
        
        vec3 c = o.rgb * 8.0;

        vec2 f = gl_FragCoord.xy/float(scale);

        c.r += dither4x4(f, fract(c.r));
        c.g += dither4x4(f, fract(c.g));
        c.b += dither4x4(f, fract(c.b));

        out_color = vec4(floor(c)/8.0, 1.0);   
    }
#endif

