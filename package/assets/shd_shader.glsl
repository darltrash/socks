
#define LIGHT_AMOUNT 16

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

    // TODO: Consider unifying vec3 color and vec3 lighting.
    float luma( vec3 color ) {
        return dot(vec3(0.2126729, 0.7151522, 0.0721750), color);
    }

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
            float inv_sqr_law = 1.0 / max(0.9, dist*dist);

            lighting += light_colors[i] * inv_sqr_law;
        }

        // magic sauce
        lighting = mix(lighting, vec3(luma(lighting)), smoothstep(0.95, 1.0, luma(lighting)) * 0.25);
    }
#endif

#ifdef PIXEL
    in vec3 position;
    in vec4 color;
    in vec2 uv;
    in float fog;
    in vec3 lighting;
    
    out vec4 out_color;

    uniform sampler2D image;
    uniform sampler2D lumos;
    uniform vec4 clear;
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

        out_color = o;
    }
#endif
