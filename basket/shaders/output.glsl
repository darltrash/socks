#ifdef VERTEX
    in vec4 vx_position;
    in vec2 vx_uv;
    out vec2 uv;

    uniform vec2 resolution;
    uniform vec2 real_resolution;

    float minimum(vec2 x) {
        return min(x.x, x.y);
    }

    void main() {
        uv = vx_uv;

        vec2 o = vx_position.xy;
        //float s = floor(minimum(real_resolution / resolution));
        //o += 1.0 - (s/2.0);

        gl_Position = vx_position;
    }
#endif

#ifdef PIXEL
    in vec2 uv;

    uniform sampler2D image;
    uniform bool dither;
    uniform int scale;

    vec2 curve(vec2 uv) {
        uv = (uv - 0.5) * 2.0;
        uv *= 1.1;	
        uv.x *= 1.0 + pow((abs(uv.y) / 9.0), 2.0);
        uv.y *= 1.0 + pow((abs(uv.x) / 8.0), 2.0);
        uv  = (uv / 2.0) + 0.5;
        uv =  uv * 0.92 + 0.04;
        return uv;
    }

    float inside(vec2 v, vec2 bottomLeft, vec2 topRight) {
        vec2 s = step(bottomLeft, v) - step(topRight, v);
        return s.x * s.y;   
    }

    void main() {
        float i = inside(uv, vec2(0.0), vec2(1.0));

        if (!dither) {
            gl_FragColor = texture2D(image, uv) * i;
            return;
        }

        vec4 o = vec4(1.0);

        o.rg = texture2D(image, uv).rg;
        o.b = texture2D(image, uv+vec2(0.001, 0.0005)).b;

        vec3 c = o.rgb * 8.0 * i;

        vec2 f = gl_FragCoord.xy/float(scale);

        c.r += dither4x4(f, fract(c.r));
        c.g += dither4x4(f, fract(c.g));
        c.b += dither4x4(f, fract(c.b));

        gl_FragColor = vec4(floor(c)/8.0, 1.0);   
    }
#endif

