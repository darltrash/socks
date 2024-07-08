#ifdef VERTEX
    in vec4 vx_position;
    in vec4 vx_color;
    in vec2 vx_uv;

    out vec4 color;
    out vec2 uv;

    uniform vec2 resolution;

    void main() {
        color = vx_color;
        uv = vx_uv;
        vec2 pos = vx_position.xy / (resolution / 2.0);
        gl_Position = vec4(pos * vec2(1.0, -1.0), -vx_position.z/5.0, 1.0);
    }
#endif

#ifdef PIXEL
    in vec4 color;
    in vec2 uv;

    uniform sampler2D image;
    uniform bool dither;

    void main() {
        vec4 o = texture2D(image, uv) * (color / 255.0);

        if (dither4x4(gl_FragCoord.xy, o.a) < 0.5) 
            discard;
            
        gl_FragColor = o;
    }
#endif

