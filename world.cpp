// Elmo Trolla, 2018
// License: pick one - UNLICENSE (www.unlicense.org) / MIT (opensource.org/licenses/MIT).

#include "world.h"

#include "sys_globals.h"


inline float dist(float x1, float y1, float x2, float y2) {
	float dx = x2 - x1; float dy = y2 - y1;
	return sqrt(dx*dx + dy*dy);
}


// // TODO: move masspoint to line normal, not just to current terrain height. also add a friction parameter.
// void collide_jello_with_ground(JelloSingleObject* out_jello, Ground* ground) {
// 	for (int i = 0; i < out_jello->masspoints.size(); i++) {
// 		MassPoint& mp = out_jello->masspoints[i];
// 		if (mp.flags & MASSPOINT_NO_COLLISION) continue;

// 		float y = ground->get_ground_height(mp.x);
// 		// just move the masspoint back onto the ground if it's below the ground.
// 		if (mp.y < y) {
// 			mp.y = y;
// 			mp.vy = 0.f;
// 			mp.vx = 0.f;
// 		}
// 	}
// }

// collides masspoints with the ground AND generates dirt particles if needed.
void collide_jello_with_ground(JelloSingleObject* out_jello, Ground* ground) {

	for (int i = 0; i < out_jello->masspoints.size(); i++) {
		MassPoint& mp = out_jello->masspoints[i];
		if (mp.flags & MASSPOINT_NO_COLLISION) continue;

		float y = ground->get_ground_height(mp.x);
		// just move the masspoint back onto the ground if it's below the ground.
		if (mp.y < y) {
			mp.y = y;

			f32 vsquare = mp.vx * mp.vx + mp.vy * mp.vy;

			//if (vsquare > 1.8*1.8f) {
			if (vsquare > 0.8*0.8f) {
			//if (mp.vx * mp.vx + mp.vy * mp.vy > 0.01*0.01) {
				// if speed was more than 0.2 m/s
				if (ground->dirt_particles.particles.size() < 2000) { // limit to 500 particles
					f32 v = sqrtf(vsquare);
					f32 rnd;

					// generate 5 slightly randomized particles per event.
					for (int k = 0; k < 10; k++) {
						f32 rnd1 = ((f32)rand() / (f32)(RAND_MAX) * 2.f - 1.f); // -1..+1
						f32 rnd2 = ((f32)rand() / (f32)(RAND_MAX) * 2.f - 1.f); // -1..+1
						f32 rnd3 = ((f32)rand() / (f32)(RAND_MAX)); // 0..+1

						f32 dvx = rnd1 * v * 0.2;
						f32 dvy = rnd2 * v * 0.2;

						u32 col = IM_COL32(30+rnd3*20, 20, 10, 255 - (rnd3)*190);
						col = IM_COL32((rnd3*.5+.5)*105, (rnd3*.5+.5)*80, (rnd3*.5+.5)*65, 255);
						ground->dirt_particles.append(mp.x, mp.y, mp.vx + dvx, -mp.vy + dvy, 0.01, col);

						ground->raise_top_ground(mp.x, -0.0001);
					}
				}
			}

			mp.vy = 0.f;
			mp.vx = 0.f;
		}
	}
}

void collide_jello_with_ground(JelloAssembly* out_jello_assembly, Ground* ground) {
	for (int i = 0; i < out_jello_assembly->jellos.size(); i++) {
		collide_jello_with_ground(&out_jello_assembly->jellos[i], ground);
	}
}

World::World() { world_reset(this); }
World::~World() { }

void world_tick_physics(World* out_world, float dt) {
	out_world->ground.dirt_particles.set_acceleration(0, out_world->gravity);

	for (int i = 0; i < out_world->jelloasms.size(); i++) {
		jello_tick_spring_forces(&out_world->jelloasms[i]);
		jello_tick_crossobject_spring_forces(&out_world->jelloasms, &out_world->jelloasms[i].cross_object_springs);
		jello_tick_positions(&out_world->jelloasms[i], 0, out_world->gravity, dt);

		collide_jello_with_ground(&out_world->jelloasms[i], &out_world->ground);
	}
}

void world_tick_frame(World* out_world, float dt) {
	out_world->ground.dirt_particles.tick(dt);

	// stop all particles that are below the ground

	DirtParticleSystem* ps = &out_world->ground.dirt_particles;

	for (int i = 0; i < ps->particles.size(); i++) {
		DirtParticle* p = &ps->particles[i];

		if (!(p->flags & DIRT_PARTICLE_FLAG_PARTICLE_STOPPED)) {
			float y = out_world->ground.get_ground_height(p->x);
			if (p->y < y) {
				p->flags |= DIRT_PARTICLE_FLAG_PARTICLE_STOPPED;
				p->y = y;
				p->age = DIRT_PARTICLE_MAX_AGE;
				// TODO: raise ground

				out_world->ground.raise_top_ground(p->x, 0.0001);
			}
		}
	}
}

void _generate_fatbike(World* out_world, JelloAssembly* out_ja) {
	JelloAssembly* ja = out_ja;

	f32 wheel_radius = 0.6;
	JelloSingleObject* front_wheel;
	JelloSingleObject* back_wheel;
	JelloSingleObject* frame;
	int front_wheel_i;
	int back_wheel_i;
	int frame_i;
	u32 front_wheel_center_mp_i;
	u32 back_wheel_center_mp_i;

	Array<MassPoint>* frame_masspoints;

	// generate wheels

	{
		front_wheel = ja->jellos.push_back_notconstructed();
		new (front_wheel) JelloSingleObject();
		front_wheel_i = ja->jellos.size() - 1;

		back_wheel = ja->jellos.push_back_notconstructed();
		new (back_wheel) JelloSingleObject();
		back_wheel_i = ja->jellos.size() - 1;

		u32 num_spokes = 24;
		f32 masspoint_mass = 1.;

		jello_generate_wheel(front_wheel, &front_wheel_center_mp_i, wheel_radius, num_spokes, masspoint_mass, 0, 0);
		jello_generate_wheel(back_wheel, &back_wheel_center_mp_i, wheel_radius, num_spokes, masspoint_mass, 0, 0);

		out_world->front_wheel_center_mp_i = front_wheel_center_mp_i;
		out_world->back_wheel_center_mp_i = back_wheel_center_mp_i;
	}

	// generate frame

	u32 player_center_jelloasm_i;
	u32 player_center_jello_i;
	u32 player_center_mp_i;

	{
		frame = ja->jellos.push_back_notconstructed();
		new (frame) JelloSingleObject();
		frame_i = ja->jellos.size() - 1;

		Array<MassPoint>* masspoints = &frame->masspoints;
		f32 m = 1.;

		MassPoint* mp;
		#define APPEND_MP(x, y, m) mp = masspoints->push_back_notconstructed(); new (mp) MassPoint(x, y, m);

		APPEND_MP( 0,   0,   m); // 0 // center
		player_center_jelloasm_i = 0;
		player_center_jello_i = frame_i;
		player_center_mp_i = masspoints->size() - 1;
		APPEND_MP(-7.2, 0.8, m); // 1 // back wheel connection
		APPEND_MP(10,   0.5, m); // 2 // front wheel connection
		APPEND_MP(-1.5, 5.3, m); // 3 // start of saddle pole
		APPEND_MP( 6.5, 8  , m); // 4 // start of guidebar

		APPEND_MP( 6,   9.5, m); // 5 // guidebar left
		APPEND_MP( 8,  10  , m); // 6 // guidebar right
		APPEND_MP(-2.8, 9.5, m); // 7 // saddle midpoint
		APPEND_MP(-5,   9.5, m); // 8 // saddle left
		APPEND_MP(-1,   9.5, m); // 9 // saddle right

		// TODO: m/10.f crashes! with integer overflow. why?
		APPEND_MP(-7.2*2, 0.8*2, m); // 10 // back counterlever for applying engine acceleration
		///APPEND_MP(-7.2*2, 0.8*2, m/10); // 10 // back counterlever for applying engine acceleration
		mp->flags = MASSPOINT_NO_COLLISION;

		Array<Spring>* springs = &frame->springs;

		Spring* s;
		u32 mp1_i, mp2_i;
		f32 k = 1000 * 100;
		#define APPEND_SPRING(mp1_i, mp2_i, _k, colr, line_wdth, flgs) s = springs->push_back_notconstructed(); new (s) Spring(mp1_i, mp2_i, jello_masspoint_dist(&(*masspoints)[mp1_i], &(*masspoints)[mp2_i])); s->flags = flgs; s->k = _k; s->color = colr; s->line_width = line_wdth;

		// frame
		//u32 color = IM_COL32(255*0.8, 200*0.8, 100*0.8, 255);
		u32 color = IM_COL32(248*0.8, 85*1.3, 30*1.4, 255);
		float line_width = 0.05; // was 5
		APPEND_SPRING(0, 1, k, color, line_width, SPRING_OUTER_EDGE | SPRING_RENDER_METHOD_2 | JELLO_FLAG_ELEMENT_VISIBLE); // 0
		APPEND_SPRING(1, 3, k, color, line_width, SPRING_OUTER_EDGE | SPRING_RENDER_METHOD_2 | JELLO_FLAG_ELEMENT_VISIBLE); // 1
		APPEND_SPRING(3, 0, k, color, line_width, SPRING_OUTER_EDGE | SPRING_RENDER_METHOD_2 | JELLO_FLAG_ELEMENT_VISIBLE); // 2
		APPEND_SPRING(0, 4, k, color, line_width, SPRING_OUTER_EDGE | SPRING_RENDER_METHOD_2 | JELLO_FLAG_ELEMENT_VISIBLE);
		APPEND_SPRING(3, 4, k, color, line_width, SPRING_OUTER_EDGE | SPRING_RENDER_METHOD_2 | JELLO_FLAG_ELEMENT_VISIBLE);
		APPEND_SPRING(4, 2, k, color, line_width, SPRING_OUTER_EDGE | SPRING_RENDER_METHOD_2 | JELLO_FLAG_ELEMENT_VISIBLE);
		APPEND_SPRING(0, 2, k, color, line_width, 0); s->set_visible(false); // 6 // support spring

		//APPEND_SPRING(3, 7, k, color, line_width, SPRING_OUTER_EDGE | JELLO_FLAG_ELEMENT_VISIBLE);

		// saddle
		color = IM_COL32(20, 20, 20, 255);
		APPEND_SPRING(3, 7, k, color, line_width, SPRING_OUTER_EDGE | JELLO_FLAG_ELEMENT_VISIBLE);
		APPEND_SPRING(8, 7, k, color, line_width, SPRING_OUTER_EDGE | JELLO_FLAG_ELEMENT_VISIBLE);
		APPEND_SPRING(7, 9, k, color, line_width, SPRING_OUTER_EDGE | JELLO_FLAG_ELEMENT_VISIBLE);
		APPEND_SPRING(1, 7, k*2., color, 1, 0); s->set_visible(false); // support spring
		APPEND_SPRING(3, 8, k/2., color, 1, 0); s->set_visible(false); // support spring
		APPEND_SPRING(3, 9, k/2., color, 1, 0); s->set_visible(false); // support spring

		// guidebar
		color = IM_COL32(20, 20, 20, 255);
		APPEND_SPRING(4, 5, k, color, line_width, SPRING_OUTER_EDGE | JELLO_FLAG_ELEMENT_VISIBLE);
		APPEND_SPRING(5, 6, k, color, line_width, SPRING_OUTER_EDGE | JELLO_FLAG_ELEMENT_VISIBLE);
		APPEND_SPRING(5, 3, k/2., color, 1, 0); s->set_visible(false); // support spring
		APPEND_SPRING(6, 4, k/2., color, 1, 0); s->set_visible(false); // support spring

		// invisible back counterlever. connected to back wheel and saddle
		APPEND_SPRING(7, 10, k, color, 1, 0); s->set_visible(false);
		APPEND_SPRING(1, 10, k, color, 1, 0); s->set_visible(false);

		f32 d = 0.8 / 7.2;
		jello_scale(frame, d, d);
	}

	// move wheels to correct positions
	jello_translate(front_wheel, frame->masspoints[2].x, frame->masspoints[2].y);
	jello_translate(back_wheel,  frame->masspoints[1].x, frame->masspoints[1].y);

	// connect wheels to the frame

	CrossObjectSpring* framespring1 = ja->cross_object_springs.push_back_notconstructed();
	new (framespring1) CrossObjectSpring(back_wheel_center_mp_i, 1, 0);
	framespring1->flags |= CROSS_OBJECT_SPRING_HOLD_ZERO_LENGTH;
	framespring1->i_jello_assembly1 = 0;
	framespring1->i_jello_assembly2 = 0;
	framespring1->i_jello_single_object1 = back_wheel_i;
	framespring1->i_jello_single_object2 = frame_i;

	CrossObjectSpring* framespring2 = ja->cross_object_springs.push_back_notconstructed();
	new (framespring2) CrossObjectSpring(front_wheel_center_mp_i, 2, 0);
	framespring2->flags |= CROSS_OBJECT_SPRING_HOLD_ZERO_LENGTH;
	framespring2->i_jello_assembly1 = 0;
	framespring2->i_jello_assembly2 = 0;
	framespring2->i_jello_single_object1 = front_wheel_i;
	framespring2->i_jello_single_object2 = frame_i;

	jello_translate(out_ja, 17, 1);

	out_world->player_center_mp = &frame->masspoints[player_center_mp_i];
}

void world_reset(World* out_world) {
	out_world->player_center_mp = NULL;
	out_world->gravity = -5.f;
	out_world->jelloasms.clear();

	// generate bicycle

	JelloAssembly* ja = out_world->jelloasms.push_back_notconstructed();
	new (ja) JelloAssembly();

	_generate_fatbike(out_world, ja);


	return;

	if (0) {
		JelloSingleObject* front_wheel = ja->jellos.push_back_notconstructed();
		new (front_wheel) JelloSingleObject();

		JelloSingleObject* back_wheel = ja->jellos.push_back_notconstructed();
		new (back_wheel) JelloSingleObject();

	//	JelloSingleObject* third_wheel = ja->jellos.push_back_notconstructed();
	//	new (third_wheel) JelloSingleObject();

		//JelloSingleObject* back_wheel = ja->jellos.push_back_notconstructed();
		//new (back_wheel) JelloSingleObject();

		f32 radius = 0.6;
		u32 spokes = 25;
		f32 masspoint_mass = 1.;
		f32 x = 0;
		f32 y = 3;

		u32 dummy;

		jello_generate_wheel(front_wheel, &out_world->front_wheel_center_mp_i, radius, spokes, masspoint_mass, x, y);
		x += 2;
		jello_generate_wheel(back_wheel, &dummy, radius, spokes, masspoint_mass, x, y);


		CrossObjectSpring* framespring1 = out_world->crossobjectsprings.push_back_notconstructed();
		new (framespring1) CrossObjectSpring(out_world->front_wheel_center_mp_i, dummy, 2);
		framespring1->i_jello_assembly1 = 0;
		framespring1->i_jello_assembly2 = 0;
		framespring1->i_jello_single_object1 = 0;
		framespring1->i_jello_single_object2 = 1;
	}

	{
		f32 radius = 0.6;
		u32 spokes = 25;
		f32 masspoint_mass = 1.;
		f32 x = 0;
		f32 y = 3;
		f32 dx = 2;

		u32 prev_wheel_jello_i;
		u32 prev_wheel_center_mp_i;

		int numwheels = 5;
		for (int i = 0; i < numwheels; i++) {
			JelloSingleObject* wheel = ja->jellos.push_back_notconstructed();
			new (wheel) JelloSingleObject();
			u32 wheel_jello_i = ja->jellos.size() - 1;
			u32 wheel_center_mp_i;
			jello_generate_wheel(wheel, &wheel_center_mp_i, radius, spokes, masspoint_mass, x, y);

			if (i == 0) {
				out_world->front_wheel_center_mp_i = wheel_center_mp_i;
			}
			x += dx;

			if (i > 0) {
				CrossObjectSpring* framespring = out_world->crossobjectsprings.push_back_notconstructed();
				new (framespring) CrossObjectSpring(prev_wheel_center_mp_i, wheel_center_mp_i, dx);

				framespring->i_jello_assembly1 = 0;
				framespring->i_jello_assembly2 = 0;
				framespring->i_jello_single_object1 = prev_wheel_jello_i;
				framespring->i_jello_single_object2 = wheel_jello_i;
				framespring->k *= 10;
			}

			if (i == numwheels / 2) {
				out_world->player_center_mp = &ja->jellos[wheel_jello_i].masspoints[wheel_center_mp_i];
			}

			prev_wheel_center_mp_i = wheel_center_mp_i;
			prev_wheel_jello_i = wheel_jello_i;
		}
	}
}

void world_accelerate_bicycle(World* out_world, float force) {
	// TODO: separate bicycle from world.
	IM_ASSERT(out_world);
	IM_ASSERT(out_world->jelloasms.size());

	// JelloSingleObject* front_wheel = &out_world->jelloasms[0].jellos[0];
	// MassPoint* front_wheel_center_mp = &front_wheel->masspoints[out_world->front_wheel_center_mp_i];
	// jello_apply_rotational_force(front_wheel, front_wheel_center_mp->x, front_wheel_center_mp->y, force);

	JelloSingleObject* back_wheel = &out_world->jelloasms[0].jellos[1];
	JelloSingleObject* frame = &out_world->jelloasms[0].jellos[2];

	MassPoint* back_wheel_center_mp = &back_wheel->masspoints[out_world->back_wheel_center_mp_i];
	jello_apply_rotational_force(back_wheel, back_wheel_center_mp->x, back_wheel_center_mp->y, force);

	// apply counter-force to the bicycle frame. use only one point of the frame.

	// // find mass center of the frame.
	// f32 massx = 0.f, massy = 0.f;
	// for (int i = 0; i < frame->masspoints.size(); i++) {
	// 	MassPoint& mp = frame->masspoints[i];
	// 	massx += mp.x;
	// 	massy += mp.y;
	// }
	// massx /= frame->masspoints.size();
	// massy /= frame->masspoints.size();


	// 10 and 0 are the masspoints we need to apply the acceleration to.

	int num_wheel_masspoints = back_wheel->masspoints.size() - 1; // minus center masspoint
	force *= -num_wheel_masspoints / 2.;

	force *= 0.5; // some fake physics. frame has less damping than the 20-masspoint wheel? don't know.. but anyway make the force applicable to the frame a good deal smaller than the force applied to the wheel.

	MassPoint* player_center_mp = out_world->player_center_mp;
	MassPoint* back_lever_mp = &frame->masspoints[10];

	f32 d = jello_masspoint_dist(player_center_mp, back_wheel_center_mp);
	if (d < 0.001)
		return;

	// assume d is the same for both "levers"

	MassPoint& mp = *back_wheel_center_mp;
	// calc unit vector from one coord to other.
	f32 dx = (mp.x - player_center_mp->x) / d;
	f32 dy = (mp.y - player_center_mp->y) / d;

	// apply force in perpendicular direction
	player_center_mp->fx +=  dy * force;
	player_center_mp->fy += -dx * force;

	back_lever_mp->fx += -dy * force;
	back_lever_mp->fy +=  dx * force;

	// return;


	// int num_wheel_masspoints = back_wheel->masspoints.size() - 1; // minus center masspoint
	// force *= num_wheel_masspoints;
	// force = force;

	// MassPoint& mp = *back_wheel_center_mp;
	// MassPoint* player_center_mp = out_world->player_center_mp;

	// f32 d = jello_masspoint_dist(player_center_mp, back_wheel_center_mp);
	// if (d < 0.001)
	// 	return;

	// // calc unit vector from one coord to other.
	// f32 dx = (mp.x - player_center_mp->x) / d;
	// f32 dy = (mp.y - player_center_mp->y) / d;

	// // apply force in perpendicular direction
	// player_center_mp->fx +=  dy * force;
	// player_center_mp->fy += -dx * force;
}

