// common math, simple 'nuff.

#include "common.h"
#include <math.h>

static f32 to_rad(const f32 deg) {
	return deg * 0.017453293f;
}

f32 clamp(f32 v, f32 low, f32 high) {
	return fmaxf(fminf(v, high), low);
}

f32 lerp(f32 a, f32 b, f32 t) {
	return a * (1.0f - t) + b * t;
}

void mat4_projection(f32 out[16], f32 fovy, f32 aspect, f32 near, f32 far, bool infinite) {
	f32 t = tanf(to_rad(fovy) / 2.0f);
	f32 m22 = infinite ? 1.0f : -(far + near) / (far - near);
	f32 m23 = infinite ? 2.0f * near : -(2.0f * far * near) / (far - near);
	f32 m32 = -1.0f;

	for (int i = 0; i < 16; i++)
        out[i] = 0.0f;

	out[0] = 1.0f / (t * aspect);
	out[5] = 1.0f / t;
	out[10] = m22;
	out[11] = m32;
	out[14] = m23;
}

void mat4_ortho(f32 out[16], f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far) {
	memset(out, 0, sizeof(f32) * 16);
	
	out[0]  = 2.0 / (right - left);
        out[5]  = 2.0 / (top - bottom);
        out[10] = -2.0 / (far - near);
        out[12] = -((right + left) / (right - left));
        out[13] = -((top + bottom) / (top - bottom));
        out[14] = -((far + near) / (far - near));
        out[15] = 1.0;
}

void mat4_lookat(f32 out[16], const f32 eye[3], const f32 at[3], const f32 up[3]) {
	f32 forward[3] = { at[0] - eye[0], at[1] - eye[1], at[2] - eye[2] };
	vec_norm(forward, forward, 3);

	f32 side[3];
	vec3_cross(side, forward, up);
	vec_norm(side, side, 3);

	f32 new_up[3];
	vec3_cross(new_up, side, forward);

	out[3] = out[7] = out[11] = 0.0f;
	out[0] = side[0];
	out[1] = new_up[0];
	out[2] = -forward[0];
	out[4] = side[1];
	out[5] = new_up[1];
	out[6] = -forward[1];
	out[8] = side[2];
	out[9] = new_up[2];
	out[10] = -forward[2];
	out[12] = -vec_dot(side, eye, 3);
	out[13] = -vec_dot(new_up, eye, 3);
	out[14] = vec_dot(forward, eye, 3);
	out[15] = 1.0f;
}

void mat4_mul(f32 out[16], const f32 a[16], const f32 b[16]) {
	out[0] = a[0]  * b[0] + a[1]  * b[4] + a[2]  * b[8]  + a[3]  * b[12];
	out[1] = a[0]  * b[1] + a[1]  * b[5] + a[2]  * b[9]  + a[3]  * b[13];
	out[2] = a[0]  * b[2] + a[1]  * b[6] + a[2]  * b[10] + a[3]  * b[14];
	out[3] = a[0]  * b[3] + a[1]  * b[7] + a[2]  * b[11] + a[3]  * b[15];
	out[4] = a[4]  * b[0] + a[5]  * b[4] + a[6]  * b[8]  + a[7]  * b[12];
	out[5] = a[4]  * b[1] + a[5]  * b[5] + a[6]  * b[9]  + a[7]  * b[13];
	out[6] = a[4]  * b[2] + a[5]  * b[6] + a[6]  * b[10] + a[7]  * b[14];
	out[7] = a[4]  * b[3] + a[5]  * b[7] + a[6]  * b[11] + a[7]  * b[15];
	out[8] = a[8]  * b[0] + a[9]  * b[4] + a[10] * b[8]  + a[11] * b[12];
	out[9] = a[8]  * b[1] + a[9]  * b[5] + a[10] * b[9]  + a[11] * b[13];
	out[10] = a[8]  * b[2] + a[9]  * b[6] + a[10] * b[10] + a[11] * b[14];
	out[11] = a[8]  * b[3] + a[9]  * b[7] + a[10] * b[11] + a[11] * b[15];
	out[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8]  + a[15] * b[12];
	out[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9]  + a[15] * b[13];
	out[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
	out[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
}

void mat4_invert(f32 out[16], const f32 a[16]) {
	f32 tmp[16];
	tmp[0]  =  a[5] * a[10] * a[15] - a[5] * a[11] * a[14] - a[9] * a[6] * a[15] + a[9] * a[7] * a[14] + a[13] * a[6] * a[11] - a[13] * a[7] * a[10];
	tmp[1]  = -a[1] * a[10] * a[15] + a[1] * a[11] * a[14] + a[9] * a[2] * a[15] - a[9] * a[3] * a[14] - a[13] * a[2] * a[11] + a[13] * a[3] * a[10];
	tmp[2]  =  a[1] * a[6]  * a[15] - a[1] * a[7]  * a[14] - a[5] * a[2] * a[15] + a[5] * a[3] * a[14] + a[13] * a[2] * a[7]  - a[13] * a[3] * a[6];
	tmp[3]  = -a[1] * a[6]  * a[11] + a[1] * a[7]  * a[10] + a[5] * a[2] * a[11] - a[5] * a[3] * a[10] - a[9]  * a[2] * a[7]  + a[9]  * a[3] * a[6];
	tmp[4]  = -a[4] * a[10] * a[15] + a[4] * a[11] * a[14] + a[8] * a[6] * a[15] - a[8] * a[7] * a[14] - a[12] * a[6] * a[11] + a[12] * a[7] * a[10];
	tmp[5]  =  a[0] * a[10] * a[15] - a[0] * a[11] * a[14] - a[8] * a[2] * a[15] + a[8] * a[3] * a[14] + a[12] * a[2] * a[11] - a[12] * a[3] * a[10];
	tmp[6]  = -a[0] * a[6]  * a[15] + a[0] * a[7]  * a[14] + a[4] * a[2] * a[15] - a[4] * a[3] * a[14] - a[12] * a[2] * a[7]  + a[12] * a[3] * a[6];
	tmp[7]  =  a[0] * a[6]  * a[11] - a[0] * a[7]  * a[10] - a[4] * a[2] * a[11] + a[4] * a[3] * a[10] + a[8]  * a[2] * a[7]  - a[8]  * a[3] * a[6];
	tmp[8]  =  a[4] * a[9]  * a[15] - a[4] * a[11] * a[13] - a[8] * a[5] * a[15] + a[8] * a[7] * a[13] + a[12] * a[5] * a[11] - a[12] * a[7] * a[9];
	tmp[9]  = -a[0] * a[9]  * a[15] + a[0] * a[11] * a[13] + a[8] * a[1] * a[15] - a[8] * a[3] * a[13] - a[12] * a[1] * a[11] + a[12] * a[3] * a[9];
	tmp[10] =  a[0] * a[5]  * a[15] - a[0] * a[7]  * a[13] - a[4] * a[1] * a[15] + a[4] * a[3] * a[13] + a[12] * a[1] * a[7]  - a[12] * a[3] * a[5];
	tmp[11] = -a[0] * a[5]  * a[11] + a[0] * a[7]  * a[9]  + a[4] * a[1] * a[11] - a[4] * a[3] * a[9]  - a[8]  * a[1] * a[7]  + a[8]  * a[3] * a[5];
	tmp[12] = -a[4] * a[9]  * a[14] + a[4] * a[10] * a[13] + a[8] * a[5] * a[14] - a[8] * a[6] * a[13] - a[12] * a[5] * a[10] + a[12] * a[6] * a[9];
	tmp[13] =  a[0] * a[9]  * a[14] - a[0] * a[10] * a[13] - a[8] * a[1] * a[14] + a[8] * a[2] * a[13] + a[12] * a[1] * a[10] - a[12] * a[2] * a[9];
	tmp[14] = -a[0] * a[5]  * a[14] + a[0] * a[6]  * a[13] + a[4] * a[1] * a[14] - a[4] * a[2] * a[13] - a[12] * a[1] * a[6]  + a[12] * a[2] * a[5];
	tmp[15] =  a[0] * a[5]  * a[10] - a[0] * a[6]  * a[9]  - a[4] * a[1] * a[10] + a[4] * a[2] * a[9]  + a[8]  * a[1] * a[6]  - a[8]  * a[2] * a[5];

	f32 det = a[0] * tmp[0] + a[1] * tmp[4] + a[2] * tmp[8] + a[3] * tmp[12];
	if (det == 0.0f) {
		for (int i = 0; i < 16; i++) {
			out[i] = a[i];
		}
	}
	det = 1.0f / det;

	for (int i = 0; i < 16; i++) {
		out[i] = tmp[i] * det;
	}
}

void mat4_mulvec(f32 out[3], f32 in[3], f32 m[16]) {
	f32 w = 1.0;
        out[0] = in[0] * m[0] + in[1] * m[4] + in[2] * m[8]  + w * m[12];
        out[1] = in[0] * m[1] + in[1] * m[5] + in[2] * m[9]  + w * m[13];
        out[2] = in[0] * m[2] + in[1] * m[6] + in[2] * m[10] + w * m[14];
}

void mat4_from_translation(f32 out[16], f32 vec[3]) {
	f32 m[16] = IDENTITY_MATRIX;
	m[12] = vec[0];
	m[13] = vec[1];
	m[14] = vec[2];
	memcpy(out, m, 16 * sizeof(f32));
}

void mat4_from_scale(f32 out[16], f32 vec[3]) {
	f32 m[16] = IDENTITY_MATRIX;
	m[0] = vec[0];
	m[5] = vec[1];
	m[10] = vec[2];
	memcpy(out, m, 16 * sizeof(f32));
}

void mat4_from_angle_axis(f32 out[16], f32 angle, f32 axis[3]) {
	f32 m[16] = IDENTITY_MATRIX;

	const f32 l = vec_len(axis, 3);
        if (l > 0.001f) {
		const f32 c = cosf(angle);
		const f32 s = sinf(angle);

		const f32 x = axis[0] / l;
		const f32 y = axis[1] / l;
		const f32 z = axis[2] / l;

		m[0] = x*x*(1-c)+c;
		m[1] = y*x*(1-c)+z*s;
		m[2] = x*z*(1-c)-y*s;
		
		m[4] = x*y*(1-c)-z*s;
		m[5] = y*y*(1-c)+c;
		m[6] = y*z*(1-c)+x*s;

		m[8] = x*z*(1-c)+y*s;
		m[9] = y*z*(1-c)-x*s;
		m[10] = z*z*(1-c)+c;
	}

	memcpy(out, m, 16 * sizeof(f32));
}

void mat4_from_euler_angle(f32 out[16], f32 euler[3]) {
	f32 ex[3] = {1.0f, 0.0f, 0.0f};
	f32 ey[3] = {0.0f, 1.0f, 0.0f};
	f32 ez[3] = {0.0f, 0.0f, 1.0f};

	f32 tmp0[16] = IDENTITY_MATRIX;
	mat4_from_angle_axis(tmp0, euler[0], ex);
	f32 tmp1[16] = IDENTITY_MATRIX;
	mat4_from_angle_axis(tmp1, euler[1], ey);
	mat4_mul(out, tmp0, tmp1);
	mat4_from_angle_axis(tmp0, euler[2], ez);
	mat4_mul(out, out, tmp0);
}

f32 vec_dot(const f32 *a, const f32 *b, int len) {
	f32 sum = 0.0;
	for (int i = 0; i < len; i++) {
		sum += a[i] * b[i];
	}
	return sum;
}

f32 vec_len(const f32 *in, int len) {
	return sqrtf(vec_dot(in, in, len));
}

void vec_min(f32 *out, const f32 *a, const f32 *b, int len) {
	for (int i=0; i < len; i++)
		out[i] = min(a[i], b[i]);
};

void vec_max(f32 *out, const f32 *a, const f32 *b, int len) {
	for (int i=0; i < len; i++)
		out[i] = max(a[i], b[i]);
};

void vec_add(f32 *out, f32 *a, f32 *b, int len) {
	for (int i=0; i < len; i++)
		out[i] = a[i] + b[i];
};

void vec_sub(f32 *out, f32 *a, f32 *b, int len) {
	for (int i=0; i < len; i++)
		out[i] = a[i] - b[i];
};

void vec_lerp(f32 *out, f32 *a, f32 *b, f32 t, int len) {
	for (int i=0; i < len; i++)
		out[i] = lerp(a[i], b[i], t);
};

void vec_norm(f32 *out, const f32 *in, int len) {
	f32 vlen = vec_len(in, len);
	if (vlen == 0.0f) {
		for (int i = 0; i < len; i++) {
			out[i] = 0.0f;
		}
		return;
	}
	vlen = 1.0f / vlen;
	for (int i = 0; i < len; i++) {
		out[i] = in[i] * vlen;
	}
}

void vec3_cross(f32 *out, const f32 *a, const f32 *b) {
	out[0] = a[1] * b[2] - a[2] * b[1];
	out[1] = a[2] * b[0] - a[0] * b[2];
	out[2] = a[0] * b[1] - a[1] * b[0];
}


static void f_uglynorm(f32 *out, const f32 *in) {
	f32 t = vec_len(in, 3);
	out[0] = in[0] / t;
	out[1] = in[1] / t;
	out[2] = in[2] / t;
	out[3] = in[3] / t;
}

void frustum_from_mat4(Frustum *f, f32 m[16]) {
	static f32 t[4];

	#define g(p,r,a,b,c,d) \
		t[0] = m[3]  + m[a]; \
		t[1] = m[7]  + m[b]; \
		t[2] = m[11] + m[c]; \
		t[3] = m[15] + m[d]; \
		f_uglynorm(p, t);    \
		t[0] = m[3]  - m[a]; \
		t[1] = m[7]  - m[b]; \
		t[2] = m[11] - m[c]; \
		t[3] = m[15] - m[d]; \
		f_uglynorm(r, t)

	g(f->left,   f->right, 0, 4, 8,  12);
	g(f->bottom, f->top,   1, 5, 9,  13);
	g(f->near,   f->far,   2, 6, 10, 14);
}

bool frustum_vs_aabb(Frustum f, f32 min[3], f32 max[3]) {
	f32 _min[3], _max[3];

	vec_min(_min, min, max, 3); // ensure min-max is respected
	vec_max(_max, min, max, 3);

	f32 m[3], d;

	#define check(p) \
		m[0] = p[0] > 0.0 ? max[0] : min[0]; \
		m[1] = p[1] > 0.0 ? max[1] : min[1]; \
		m[2] = p[2] > 0.0 ? max[2] : min[2]; \
		d = p[0] * m[0] + p[1] * m[1] + p[2] * m[2]; \
		if (d < -p[3]) return false;

	check(f.left);
	check(f.right);
	check(f.bottom);
	check(f.top);
	check(f.near);
	check(f.far);

	return true;
}

bool frustum_vs_sphere(Frustum f, f32 pos[3], f32 radius) {
	f32 d;

	#define fvs_check(p)    \
		d = p[0] * pos[0] + \
			p[1] * pos[1] + \
			p[2] * pos[2] + \
			p[3];           \
		if (d <= -radius) return false

	fvs_check(f.left);
	fvs_check(f.right);
	fvs_check(f.bottom);
	fvs_check(f.top);
	fvs_check(f.far);
	fvs_check(f.near);

	return true;
}

bool frustum_vs_triangle(Frustum f, f32 a[3], f32 b[3], f32 c[3]) {
	f32 _min[3], _max[3];

	for (int i=0; i < 3; i++) {
		_min[i] = min(a[i], min(b[i], c[i]));
		_max[i] = max(a[i], max(b[i], c[i]));
	}

	return frustum_vs_aabb(f, _min, _max);
}
