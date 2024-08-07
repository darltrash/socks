type Vec3 = number[];

function vec3_sub(a: Vec3, b: Vec3): Vec3 {
    return [
        a[0] - b[0],
        a[1] - b[1],
        a[2] - b[2],
	]
}

function vec3_add(a: Vec3, b: Vec3): Vec3 {
    return [
        a[0] + b[0],
        a[1] + b[1],
        a[2] + b[2],
	]
}

function vec3_mul(a: Vec3, b: Vec3): Vec3 {
    return [
        a[0] * b[0],
        a[1] * b[1],
        a[2] * b[2],
	]
}

function vec3_scale(a: Vec3, b: number): Vec3 {
    return [
        a[0] + b,
        a[1] + b,
        a[2] + b,
	];
}

function vec3_div(a: Vec3, b: Vec3): Vec3 {
    return [
        a[0] / b[0],
        a[1] / b[1],
        a[2] / b[2],
	]
}

function vec3_inverse(a: Vec3): Vec3 {
    return [
        1 / a[0],
        1 / a[1],
        1 / a[2],
	]
}

function vec3_cross(a: Vec3, b: Vec3): Vec3 {
    return [
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]
	]
}

function vec3_dot(a: Vec3, b: Vec3): number {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

function vec3_length2(a: Vec3): number {
    return Math.pow(a[0], 2) + Math.pow(a[1], 2) + Math.pow(a[2], 2);
}

function vec3_length(a: Vec3) {
	return Math.sqrt(vec3_length2(a))
}

function vec3_normalize(a: Vec3): Vec3 {
    let l = vec3_length(a);

    if (l == 0)
        return [0, 0, 0];

    return [
        a[0] / l,
        a[1] / l,
        a[2] / l,
	]
}

function vec3_clone (a: Vec3): Vec3 {
    return [ a[0], a[1], a[2] ]
}

function clamp(v: number, l: number, h: number) {
	return Math.max(l, Math.min(h, v));
}

type Plane = {
	normal: Vec3;
	position: Vec3;
};

function signed_distance(plane: Plane, base_point: Vec3): number {
	const d = -vec3_dot(plane.normal, plane.position);
	return vec3_dot(base_point, plane.normal) + d;
}

function triangle_intersects_point(point: Vec3, v0: Vec3, v1: Vec3, v2: Vec3): boolean {
	const u = vec3_sub(v1, v0);
	const v = vec3_sub(v2, v0);
	const w = vec3_sub(point, v0);

	const vw = vec3_cross(v, w);
	const vu = vec3_cross(v, u);

	if (vec3_dot(vw, vu) < 0.0) {
		return false;
	}

	const uw = vec3_cross(u, w);
	const uv = vec3_cross(u, v);

	if (vec3_dot(uw, uv) < 0.0) {
		return false;
	}

	const d: number = 1 / vec3_length(uv);
	const r: number = vec3_length(vw) * d;
	const t: number = vec3_length(uw) * d;

	return (r + t) <= 1;
}

function get_lowest_root(root: {v: number}, a: number, b: number, c: number, max: number): boolean {
	// check if solution exists
	const determinant: number = b*b - 4.0*a*c;

	// if negative there is no solution
	if (determinant < 0.0) {
		return false;
	}

	// calculate two roots
	const sqrtD: number = Math.sqrt(determinant);
	let r1: number = (-b - sqrtD) / (2*a);
	let r2: number = (-b + sqrtD) / (2*a);

	// set x1 <= x2
	if (r1 > r2) {
		const temp: number = r2;
		r2 = r1;
		r1 = temp;
	}

	// get lowest root
	if (r1 > 0 && r1 < max) {
		root.v = r1;
		return true;
	}

	if (r2 > 0 && r2 < max) {
		root.v = r2;
		return true;
	}

	// no solutions
	return false;
}

function check_triangle(packet: Packet, p1: Vec3, p2: Vec3, p3: Vec3, id: number) {
	const pn: Vec3 = vec3_normalize(vec3_cross(vec3_sub(p2, p1), vec3_sub(p3, p1)));

	// only check front facing triangles
	if (vec3_dot(pn, packet.e_norm_velocity) > 0.0) {
		//return packet;
	}

	// get interval of plane intersection
	let t0: number = 0.0;
	let embedded_in_plane: boolean = false;

	// signed distance from sphere to point on plane
	const signed_dist_to_plane: number = vec3_dot(packet.e_base_point, pn) - vec3_dot(pn, p1);

	// cache this as we will reuse
	const normal_dot_vel = vec3_dot(pn, packet.e_velocity);

	// if sphere is moving parallel to plane
	if (normal_dot_vel == 0.0) {
		if (Math.abs(signed_dist_to_plane) >= 1.0) {
			// no collision possible
			return packet;
		} else {
			// sphere is in plane in whole range [0..1]
			embedded_in_plane = true;
			t0 = 0.0;
		}
	} else {
		// N dot D is not 0, calc intersect interval
		const nvi = 1.0 / normal_dot_vel;
		t0 = (-1.0 - signed_dist_to_plane) * nvi;
		let t1 = ( 1.0 - signed_dist_to_plane) * nvi;

		// swap so t0 < t1
		if (t0 > t1) {
			const temp = t1;
			t1 = t0;
			t0 = temp;
		}

		// check that at least one result is within range
		if (t0 > 1.0 || t1 < 0.0) {
			// both values outside range [0,1] so no collision
			return packet;
		}

		t0 = clamp(t0, 0.0, 1.0);
	}

	// time to check for a collision
	let collision_point: Vec3 = [0.0, 0.0, 0.0];
	let found_collision: boolean = false;
	let t: number = 1.0;

	// first check collision with the inside of the triangle
	if (!embedded_in_plane) {
		let plane_intersect: Vec3 = vec3_sub(packet.e_base_point, pn);
		const temp: Vec3 = vec3_scale(packet.e_velocity, t0);
		plane_intersect = vec3_add(plane_intersect, temp);

		if (triangle_intersects_point(plane_intersect, p1, p2, p3)) {
			found_collision = true;
			t = t0;
			collision_point = plane_intersect;
		}
	}

	// no collision yet, check against points and edges
	if (!found_collision) {
		const velocity_sq_length = vec3_length2(packet.e_velocity);
		const a: number = velocity_sq_length;
		const new_t = { v: 0.0 };

		// equation is a*t^2 + b*t + c = 0
		// check against points
		function check_point(collision_point: Vec3, p: Vec3) {
			// sketchy: should this be two vars and not just the one?
			const b = 2.0 * vec3_dot(packet.e_velocity, vec3_sub(packet.e_base_point, p));
			const c = vec3_length2(vec3_sub(p, packet.e_base_point)) - 1.0;
			if (get_lowest_root(new_t, a, b, c, t)) {
				t = new_t.v;
				found_collision = true;
				collision_point = p;
			}
			return collision_point;
		}

		// p1
		collision_point = check_point(collision_point, p1);

		// p2
		if (!found_collision) {
			collision_point = check_point(collision_point, p2);
		}

		// p3
		if (!found_collision) {
			collision_point = check_point(collision_point, p3);
		}

		// check against edges
		function check_edge(collision_point: Vec3, pa: Vec3, pb: Vec3) {
			const edge = vec3_sub(pb, pa);
			const base_to_vertex = vec3_sub(pa, packet.e_base_point);
			const edge_sq_length = vec3_length2(edge);
			const edge_dot_velocity = vec3_dot(edge, packet.e_velocity);
			const edge_dot_base_to_vertex = vec3_dot(edge, base_to_vertex);

			// calculate params for equation
			const a = edge_sq_length * -velocity_sq_length + edge_dot_velocity * edge_dot_velocity;
			const b = edge_sq_length * (2.0 * vec3_dot(packet.e_velocity, base_to_vertex)) - 2.0 * edge_dot_velocity * edge_dot_base_to_vertex;
			const c = edge_sq_length * (1.0 - vec3_length2(base_to_vertex)) + edge_dot_base_to_vertex * edge_dot_base_to_vertex;

			// do we collide against infinite edge
			if (get_lowest_root(new_t, a, b, c, t)) {
				// check if intersect is within line segment
				const f = (edge_dot_velocity * new_t.v - edge_dot_base_to_vertex) / edge_sq_length;
				if (f >= 0.0 && f <= 1.0) {
					t = new_t.v;
					found_collision = true;
					collision_point = vec3_add(pa, vec3_scale(edge, f));
				}
			}

			return collision_point;
		}
		collision_point = check_edge(collision_point, p1, p2); // p1 -> p2
		collision_point = check_edge(collision_point, p2, p3); // p2 -> p3
		collision_point = check_edge(collision_point, p3, p1); // p3 -> p1
	}

	// set results
	if (found_collision) {
		// distance to collision, t is time of collision
		const dist_to_coll = t * vec3_length(packet.e_velocity);

		// are we the closest hit?
		if (!packet.found_collision || dist_to_coll < packet.nearest_distance) {
			packet.nearest_distance = dist_to_coll;
			packet.intersect_point  = collision_point;
			packet.intersect_time   = t;
			packet.found_collision  = true;
			packet.id = id;
		}
	}

	return packet;
}

type Packet = {
	// r3 (world) space
	r3_position:     Vec3,
	r3_velocity:     Vec3,

	// ellipsoid space
	e_radius:        Vec3,
	e_inv_radius:    Vec3,
	e_position:      Vec3,
	e_velocity:      Vec3,
	e_norm_velocity: Vec3,
	e_base_point:    Vec3,

	// internal hit information
	found_collision:  boolean,
	nearest_distance: number,
	intersect_point:  Vec3,
	intersect_time:   number,
	id: number,

	// user hit information
	contacts: Array<{
		id: number,
		plane: Plane
	}>
}

type TriQueryCb = (tris:Vec3[], min:Vec3, max:Vec3, vel:Vec3)=>void;

// This implements the improvements to Kasper Fauerby's "Improved Collision
// detection and Response" proposed by Jeff Linahan's "Improving the Numerical
// Robustness of Sphere Swept Collision Detection"
// const VERY_CLOSE_DIST: number = 0.005;
const VERY_CLOSE_DIST: number = 0.00125;

function collide_with_world(packet: Packet, e_position: Vec3, e_velocity: Vec3, tris: Vec3[], ids: number[] | null) {
	let first_plane: Plane | null = null;
	let dest: Vec3 = vec3_add(e_position, e_velocity);
	// let speed: Float = min(1.0, length(e_velocity));
	const speed = 1.0;

	// check for collision
	for (let i = 0; i < 3; i++) {
		packet.e_norm_velocity = vec3_clone(e_velocity);
		packet.e_norm_velocity = vec3_normalize(packet.e_norm_velocity);
		packet.e_velocity = vec3_clone(e_velocity);
		packet.e_base_point = vec3_clone(e_position);
		packet.found_collision = false;
		packet.nearest_distance = 1e20;

		// check for collision
		check_collision(packet, tris, ids);

		// no collision
		if (!packet.found_collision) {
			return dest;
		}

		let touch_point = vec3_add(e_position, vec3_scale(e_velocity, packet.intersect_time));

		let pn = vec3_normalize(vec3_sub(touch_point, packet.intersect_point));
		let p: Plane = {
			position: packet.intersect_point,
			normal: pn
		}
		let n = vec3_normalize(vec3_div(p.normal, packet.e_radius));

		let dist = vec3_length(e_velocity) * packet.intersect_time;
		let short_dist = Math.max(dist - speed * VERY_CLOSE_DIST, 0.0);

		let nvel = vec3_normalize(e_velocity);
		e_position = vec3_add(e_position, vec3_scale(nvel, short_dist))

		packet.contacts.push({
			id: packet.id,
			plane: {
				position: vec3_mul(p.position, packet.e_radius),
				normal: n
			}
		});

		if (i == 0) {
			let long_radius = 1.0 + speed * VERY_CLOSE_DIST;
			first_plane = p;

			dest = vec3_sub(dest, vec3_scale(first_plane.normal, (signed_distance(first_plane, dest) - long_radius)));
			e_velocity = vec3_sub(dest, e_position);
		} else if (i == 1 && first_plane) {
			let second_plane = p;
			let crease = vec3_normalize(vec3_cross(first_plane.normal, second_plane.normal));
			let dis = vec3_dot(vec3_sub(dest, e_position), crease);
			e_velocity = vec3_scale(crease, dis);
			dest = vec3_add(e_position, e_velocity);
		}
	}

	return e_position;
}

function check_collision(packet: Packet, tris: Vec3[], ids: number[] | null) {
	let inv_radius = packet.e_inv_radius;
	for (let i = 0; i < tris.length / 3; i++) {
		const idx = i * 3;
		const v0 = tris[idx];
		const v1 = tris[idx+1];
		const v2 = tris[idx+2];
		// const vn = tris[idx+3];

		// for (tri in tris) {
		// Collision.check_triangle(
		// 	packet,
		// 	tri.v0 / packet.e_radius,
		// 	tri.v1 / packet.e_radius,
		// 	tri.v2 / packet.e_radius
		// );
		check_triangle(
			packet,
			vec3_mul(v0, inv_radius),
			vec3_mul(v1, inv_radius),
			vec3_mul(v2, inv_radius),
			ids ? ids[i] : 0
		);
	}
}

function sub_update(packet: Packet, position: Vec3, gravity: Vec3, tris: Vec3[], ids: number[] | null) {
	packet.e_velocity = vec3_scale(packet.e_velocity, 0.5);

	// convert to e-space
	let e_position = vec3_clone(packet.e_position);
	let e_velocity = vec3_clone(packet.e_velocity);

	// do velocity iteration
	let final_position = collide_with_world(packet, e_position, e_velocity, tris, ids);

	// convert velocity to e-space
	e_velocity = vec3_add(e_velocity, vec3_div(gravity, packet.e_radius));

	// do gravity iteration
	final_position = collide_with_world(packet, final_position, e_velocity, tris, ids);

	// convert back to r3-space
	packet.r3_position = vec3_mul(final_position, packet.e_radius);
	packet.r3_velocity = vec3_sub(packet.r3_position, position);
}

export function response_check(tri_cache: Vec3[], id_cache: number[] | null, position: Vec3, velocity: Vec3, radius: Vec3, query?: TriQueryCb) {
	let packet: Packet = {
		r3_position: position,
		r3_velocity: velocity,
		e_radius: radius,
		e_inv_radius: vec3_inverse(radius),
		e_position: vec3_div(position, radius),
		e_velocity: vec3_div(velocity, radius),
		e_norm_velocity: [0, 0, 0],
		e_base_point: [0, 0, 0],
		found_collision: false,
		nearest_distance: 0.0,
		intersect_point: [0, 0, 0],
		intersect_time: 0.0,
		id: 0,
		contacts: []
	};
	let r3_position = vec3_mul(packet.e_position, packet.e_radius);
	let query_radius = packet.e_radius;
	let min = vec3_sub(r3_position, query_radius);
	let max = vec3_add(r3_position, query_radius);
	if (query) {
		tri_cache = response_get_tris(min, max, packet.r3_velocity, query);
		id_cache = null;
		if (tri_cache.length > 0 && (tri_cache.length % 3) != 0) {
			throw "tri cache must be filled with a multiple of 3 values (v0, v1, v2)";
		}
	}
	check_collision(packet, tri_cache, id_cache);

	if (packet.found_collision) {
		let touch_point = vec3_add(packet.e_position, vec3_scale(packet.e_velocity, packet.intersect_time));
		packet.contacts.push({
			id: packet.id,
			plane: {
				position: vec3_mul(packet.intersect_point, packet.e_radius),
				normal: vec3_normalize(vec3_div(vec3_sub(touch_point, packet.intersect_point), packet.e_radius))
			}
		});
	}
	return {
		position: packet.intersect_point,
		contacts: packet.contacts
	};
}

export function response_get_tris(position: Vec3, velocity: Vec3, radius: Vec3, query: TriQueryCb): Vec3[] {
	// NB: scale the octree query by velocity to make sure we still get the
	// triangles we need at high velocities. without this, long falls will
	// jam you into the floor. A bit (25%) of padding is added so I can
	// sleep at night.
	let scale = Math.max(1.5, vec3_length(velocity)) * 1.25;
	let r3_position = position;
	let query_radius = vec3_scale(radius, scale);
	let min = vec3_sub(r3_position, query_radius);
	let max = vec3_add(r3_position, query_radius);
	const tri_cache: Vec3[] = [];
	query(tri_cache, min, max, velocity);
	if (tri_cache.length > 0 && (tri_cache.length % 3) != 0) {
		throw "tri cache must be filled with a multiple of 3 values (x, y, z)";
	}
	return tri_cache;
}

export function response_update(tri_cache: Vec3[], id_cache: number[] | null, position: Vec3, velocity: Vec3, radius: Vec3, gravity: Vec3, query?: TriQueryCb, substeps: number = 1) {
	velocity = vec3_scale(velocity, 1 / substeps);
	gravity = vec3_scale(gravity, 1 / substeps);

	if (query) {
		tri_cache = response_get_tris(position, velocity, radius, query);
		id_cache = null;
		if (tri_cache.length > 0 && (tri_cache.length % 3) != 0) {
			throw "tri cache must be filled with a multiple of 3 values (x, y, z)";
		}
	}

	let base_position = position;
	let contacts: Array<{ id: number, plane: Plane }> = [];
	for (let i = 0; i < substeps; i++) {
		let packet: Packet = {
			r3_position: position,
			r3_velocity: velocity,
			e_radius: radius,
			e_inv_radius: vec3_inverse(radius),
			e_position: vec3_div(position, radius),
			e_velocity: vec3_div(velocity, radius),
			e_norm_velocity: [0, 0, 0],
			e_base_point: [0, 0, 0],
			found_collision: false,
			nearest_distance: 0.0,
			intersect_point: [0, 0, 0],
			intersect_time: 0.0,
			id: 0,
			contacts: contacts
		};
		sub_update(packet, packet.r3_position, gravity, tri_cache, id_cache);
		position = packet.r3_position;
		velocity = packet.r3_velocity;
	}

	return {
		position: position,
		velocity: vec3_sub(position, base_position),
		contacts: contacts
	};
}