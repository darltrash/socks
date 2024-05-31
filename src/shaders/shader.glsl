
#define LIGHT_AMOUNT 32

#ifdef VERTEX
    in vec4 vx_position;
    in vec2 vx_uv;
    in vec4 vx_color;

    uniform mat4 projection;
    uniform vec2 resolution;
    uniform vec3 target;
    uniform float far;// = 15.0;

    uniform vec3 ambient;// = vec3(0.6, 0.4, 0.8);
    uniform vec3 light_positions[LIGHT_AMOUNT];
    uniform vec3 light_colors[LIGHT_AMOUNT];
    uniform int  light_amount;// = 0;

    uniform int snapping;// = 0;

    out vec3 position;
    out vec4 color;
    out vec2 uv;
    out vec3 lighting;
    out float fog;

    void main() {
        position = vx_position.xyz;
        color = vx_color / 255.0;
        uv = vx_uv;
        
        gl_Position = projection * vx_position;
        gl_Position.xyz /= max(gl_Position.w, 0.00001);
        gl_Position.w = 1.0;
        
        vec2 s = resolution / float(1 + snapping);
        vec2 r = floor((gl_Position.xy * 0.5 + 0.5) * s);
        gl_Position.xy = (r / s) * 2.0 - 1.0;

        fog = clamp(1.0 - (distance(target, position) / far), 0.0, 1.0);
        //fog = fog * fog;

        lighting = ambient;
        for (int i=0; i < light_amount; i++) {
            float dist = distance(light_positions[i], vx_position.xyz);
            float inv_sqr_law = 1.0 / max(0.8, dist*dist);

            lighting += light_colors[i] * inv_sqr_law;
        }

        // magic sauce
        lighting = mix(lighting, vec3(luma(lighting)*1.0), smoothstep(0.45, 1.0, luma(lighting)) * 0.95);
    }
#endif

#ifdef PIXEL
    in vec3 position;
    in vec4 color;
    in vec2 uv;
    in float fog;
    in vec3 lighting;

    uniform sampler2D image;
    uniform sampler2D lumos;
    uniform vec4 clear;
    uniform bool dither;

    void main() {
        vec4 o = texture2D(image, uv) * color; // main texture

        o.a *= min(1.0, length(position)/2.0);

        if (dither4x4(gl_FragCoord.xy, o.a) < 0.5) 
            discard;

        o.rgb *= lighting;

        o.rgb = mix(clear.rgb, o.rgb, fog);

        vec4 l = texture2D(lumos, uv) * color; // glowy things texture

        //if (dither4x4(gl_FragCoord.xy, fog*2.0) < 0.5) 
        //    l.a = 0.0;

        o.rgb += l.rgb * l.a * 0.5;

        gl_FragColor = o;
    }
#endif
