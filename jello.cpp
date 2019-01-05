// Elmo Trolla, 2018
// License: pick one - UNLICENSE (www.unlicense.org) / MIT (opensource.org/licenses/MIT).

#include "jello.h"

#include <math.h>

//#include "imgui.h" // ImVector

#include "sys_globals.h"

/*

notes:

rattad eraldi objektid.
torud eraldi objektid.
kuidas realtime lammutamine k2ib?

lammutamiseks on 2 v6imalust.
    1. kaotan vedru 2ra t2iesti.
    2. loon uue masspunkti, panen m6lemale masspunktile pool massist, ja yhendan yhe vedru uue masspunti kylge.
parim variant oleks vedru 2ra kaotada, kuid see n6uab rohkemate detailidega masinat.


editor:

genereerin editoris yhe ratta. nihutan paika.
genereerin editoris teise ratta. nihutan paika.
joonistan rataste vahele raami (kolmas objekt). nihutan punkte.
yhendan m6ned punktid kokku.

*/


inline void _add_spring_force_to_masspoints(Spring* spring, Array<MassPoint>* out_masspoints);
inline void _add_spring_force_to_masspoints(Spring* spring, MassPoint* out_mp1, MassPoint* out_mp2);
inline void _move_masspoint_according_to_accumulated_forces(MassPoint* out_masspoint, f32 add_ax, f32 add_ay, f32 dt);


inline float dist(float x1, float y1, float x2, float y2)
{
	float dx = x2 - x1; float dy = y2 - y1;
	return sqrt(dx*dx + dy*dy);
}

inline float dist(MassPoint* p1, MassPoint* p2)
{
	float dx = p2->x - p1->x; float dy = p2->y - p1->y;
	return sqrt(dx*dx + dy*dy);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// public interface
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

JelloAssembly::~JelloAssembly() { }

void jello_tick_spring_forces(JelloSingleObject* out_jello) {
	if (out_jello->flags & JELLO_FLAG_ELEMENT_UNUSED) return;

	Array<Spring>&    springs    = out_jello->springs;
	Array<MassPoint>& masspoints = out_jello->masspoints;
	// calculate all spring-forces and save them in masspoints for later use.
	for (int i = 0; i < springs.size(); i++) {
		// euler integrator
		Spring* s = &springs[i];
		if (s->flags & JELLO_FLAG_ELEMENT_UNUSED) continue;
		_add_spring_force_to_masspoints(s, &masspoints);
	}
}

// move masspoints according to the forces acting on each one of them.
// add_fy could be used as gravity.
void jello_tick_positions(JelloSingleObject* out_jello, f32 add_ax, f32 add_ay, f32 dt) {
	if (out_jello->flags & JELLO_FLAG_ELEMENT_UNUSED) return;

	Array<MassPoint>& masspoints = out_jello->masspoints;
	// move masspoints according to the forces acting on each one of them.
	for (int i = 0; i < masspoints.size(); i++) {
		_move_masspoint_according_to_accumulated_forces(&masspoints[i], add_ax, add_ay, dt);
	}
}

void jello_tick_spring_forces(JelloAssembly* out_jello_assembly) {
	if (out_jello_assembly->flags & JELLO_FLAG_ELEMENT_UNUSED) return;

	for (int i = 0; i < out_jello_assembly->jellos.size(); i++) {
		JelloSingleObject* je = &out_jello_assembly->jellos[i];
		if (je->flags & JELLO_FLAG_ELEMENT_UNUSED) continue;
		jello_tick_spring_forces(je);
	}
}

// move masspoints according to the forces acting on each one of them.
// add_ay could be used as gravity.
void jello_tick_positions(JelloAssembly* out_jello_assembly, f32 add_ax, f32 add_ay, f32 dt) {
	if (out_jello_assembly->flags & JELLO_FLAG_ELEMENT_UNUSED) return;
	for (int i = 0; i < out_jello_assembly->jellos.size(); i++) {
		JelloSingleObject* je = &out_jello_assembly->jellos[i];
		if (je->flags & JELLO_FLAG_ELEMENT_UNUSED) continue;
		jello_tick_positions(je, add_ax, add_ay, dt);
	}
}

// TODO: this function, maybe, should not be here
// call this AFTER calling jello_tick_spring_forces, NOT before.
void jello_tick_crossobject_spring_forces(Array<JelloAssembly>* jello_assemblys, Array<CrossObjectSpring>* springs) {
	for (int i = 0; i < springs->size(); i++) {
		CrossObjectSpring* s = &(*springs)[i];
		if (s->flags & JELLO_FLAG_ELEMENT_UNUSED) continue;

		MassPoint* mp1 = &(*jello_assemblys)[s->i_jello_assembly1].jellos[s->i_jello_single_object1].masspoints[s->i_mp1];
		MassPoint* mp2 = &(*jello_assemblys)[s->i_jello_assembly2].jellos[s->i_jello_single_object2].masspoints[s->i_mp2];

		if (s->flags & CROSS_OBJECT_SPRING_HOLD_ZERO_LENGTH) {
			// set position of both ends the same to eliminate any floating point accumulative errors.
			mp1->x = mp2->x;
			mp1->y = mp2->y;

			// This is kind of inverse of what's in _add_spring_force_to_masspoints.
			// Salculate the forces we need to apply to both ends of the zero-length spring to keep the
			// length zero.

			f32 fx = mp1->fx + mp2->fx;
			f32 fy = mp1->fy + mp2->fy;

			f32 ax = fx / (mp1->m + mp2->m);
			f32 ay = fy / (mp1->m + mp2->m);

			mp1->fx = mp1->m * ax;
			mp1->fy = mp1->m * ay;
			mp2->fx = mp2->m * ax;
			mp2->fy = mp2->m * ay;

		} else {
			_add_spring_force_to_masspoints(s, mp1, mp2);
		}
	}
}

void jello_translate(JelloSingleObject* out_jello, f32 dx, f32 dy) {
	for (int i = 0; i < out_jello->masspoints.size(); i++) {
		out_jello->masspoints[i].x += dx;
		out_jello->masspoints[i].y += dy;
	}
}

void jello_translate(JelloAssembly* out_jello_assembly, f32 dx, f32 dy) {
	for (int i = 0; i < out_jello_assembly->jellos.size(); i++) {
		jello_translate(&out_jello_assembly->jellos[i], dx, dy);
	}
}

void jello_scale(JelloSingleObject* out_jello, f32 wmult, f32 hmult) {
	for (int i = 0; i < out_jello->masspoints.size(); i++) {
		out_jello->masspoints[i].x *= wmult;
		out_jello->masspoints[i].y *= hmult;
	}
	jello_recalculate_spring_lengths(out_jello);
}

void jello_recalculate_spring_lengths(JelloSingleObject* out_jello) {
	Array<MassPoint>& masspoints = out_jello->masspoints;
	for (int i = 0; i < out_jello->springs.size(); i++) {
		Spring* s = &out_jello->springs[i];
		out_jello->springs[i].l = dist(&masspoints[s->i_mp1], &masspoints[s->i_mp2]);
	}
}

// TODO: this is not physically correct. have to think a bit more.
void jello_apply_rotational_force(JelloSingleObject* out_jello, f32 x, f32 y, f32 force) {

	for (int i = 0; i < out_jello->masspoints.size(); i++) {
		MassPoint& mp = out_jello->masspoints[i];

		f32 d  = dist(mp.x, mp.y, x, y);
		if (d < 0.001)
			continue;

		// calc unit vector from one coord to other.
		f32 dx = (x - mp.x) / d;
		f32 dy = (y - mp.y) / d;

		// apply force in perpendicular direction
		mp.fx +=  dy * force;
		mp.fy += -dx * force;
	}
}

// --------------------------------------------------------------------------------------------------------------------
// different shape generator functions

// Append rectangular jello to the given jello object.
// x_num, y_num is num of points, not number of squares.
void jello_generate_rectangular(JelloSingleObject* out_jello, u32* out_center_mp_i, f32 spring_len, f32 masspoint_mass, int x_num, int y_num) {
	ASSERT(out_jello);

	int masspoints_start_i = out_jello->masspoints.size();
	//int springs_start_i = out_jello->springs.size();

	Array<Spring>&    springs    = out_jello->springs;
	Array<MassPoint>& masspoints = out_jello->masspoints;

	f32 lw = spring_len; // square size width, height
	f32 lh = spring_len;
	f32 x_org = -(x_num - 1) * lw / 2.f;
	f32 y_org =  (y_num - 1) * lh / 2.f;

	// create the masspoints.
	for (int iy = 0; iy < y_num; iy++) {
		for (int ix = 0; ix < x_num; ix++) {
			MassPoint* mp = masspoints.push_back_notconstructed();
			new (mp) MassPoint(x_org + ix * lw, y_org + iy * lh, masspoint_mass);
		}
	}

	// create and init springs
	for (int iy = 0; iy < y_num; iy++) {
		for (int ix = 0; ix < x_num; ix++) {

			int im = masspoints_start_i + ix + iy * x_num; // recreate masspoint index

			// horizontal springs
			if (ix < x_num - 1) {
				int mp1_i = im;
				int mp2_i = im + 1;
				f32 spring_len = dist(&masspoints[mp1_i], &masspoints[mp2_i]);
				Spring* s = springs.push_back_notconstructed();
				new (s) Spring(mp1_i, mp2_i, spring_len);
				//s->color = makecol(255, 255, 200);
				//s->color = IM_COL32(255, 255, 200);
				if (iy == 0 || iy == y_num - 1)
					s->set_outer_edge(true);
			}

			// vertical springs
			if (iy < y_num - 1) {
				int mp1_i = im;
				int mp2_i = im + x_num;
				f32 spring_len = dist(&masspoints[mp1_i], &masspoints[mp2_i]);
				Spring* s = springs.push_back_notconstructed();
				new (s) Spring(mp1_i, mp2_i, spring_len);
				if (ix == 0 || ix == x_num - 1)
					s->set_outer_edge(true);
			}

			// diagonal springs 1
			if (ix < x_num - 1 && iy < y_num - 1) {
				int mp1_i = im;
				int mp2_i = im + x_num + 1;
				f32 spring_len = dist(&masspoints[mp1_i], &masspoints[mp2_i]);
				Spring* s = springs.push_back_notconstructed();
				new (s) Spring(mp1_i, mp2_i, spring_len);
				//s->set_visible(false)
			}

			// diagonal springs 2
			if (ix < x_num - 1 && iy < y_num - 1) {
				int mp1_i = im + 1;
				int mp2_i = im + x_num;
				f32 spring_len = dist(&masspoints[mp1_i], &masspoints[mp2_i]);
				Spring* s = springs.push_back_notconstructed();
				new (s) Spring(mp1_i, mp2_i, spring_len);
				//s->set_visible(false)
			}

			// diagonal springs 3, over 2 horizontal...
			// .. not implemented ..
		}
	}

	*out_center_mp_i = masspoints_start_i + x_num * y_num / 2;
}

// Append wheel jello to the given jello object.
void jello_generate_wheel(JelloSingleObject* out_jello, u32* out_center_mp_i, f32 radius, int num_spokes, f32 masspoint_mass, f32 x, f32 y) {
	ASSERT(out_jello);

	if (num_spokes < 6) num_spokes = 6; // NOTE: this has to be changed if there are more than 3 rubberlevels

	int masspoints_start_i = out_jello->masspoints.size();

	Array<Spring>&    springs    = out_jello->springs;
	Array<MassPoint>& masspoints = out_jello->masspoints;

	// generate rubber masspoints and the wheel center masspoint
	f32 a = 0;
	for (int i = 0; i < num_spokes; i++) {
		a = i * (360.f / num_spokes);
		f32 dx = radius * sin(a * M_PI / 180.f);
		f32 dy = radius * cos(a * M_PI / 180.f);
		MassPoint* mp = masspoints.push_back_notconstructed();
		new (mp) MassPoint(x + dx, y + dy, masspoint_mass);
	}

	*out_center_mp_i = masspoints.size();
	MassPoint* mp = masspoints.push_back_notconstructed();
	new (mp) MassPoint(x, y, masspoint_mass);

	// generate spokes

	for (int i = 0; i < num_spokes; i++) {
		// generate spoke
		int mp1_i = masspoints_start_i + i;
		int mp2_i = *out_center_mp_i;
		f32 spring_len = dist(&masspoints[mp1_i], &masspoints[mp2_i]);
		Spring* s = springs.push_back_notconstructed();
		new (s) Spring(mp1_i, mp2_i, spring_len);
		//s->k *= 2;
		s->k *= 3;
		s->color = IM_COL32(100, 100, 100, 255);
		s->line_width = 0.002; // 2mm
		if (i == 0 || i == num_spokes / 2) {
			s->color = IM_COL32(150, 150, 150, 255);
		}
	}

	// generate nearest-neighbour rubber

	for (int i = 0; i < num_spokes; i++) {
		// calc spoke address
		int mp1_i = masspoints_start_i + i;
		int mp2_i = *out_center_mp_i;

		// generate rubber that starts from current spoke
		for (int rubberlevel = 3; rubberlevel >= 0; rubberlevel--) {
			int spokestep = rubberlevel;
			if (rubberlevel == 2) spokestep = 3;
			if (rubberlevel == 3) spokestep = 4;

			int mp1_i = masspoints_start_i + i;
			int mp2_i = masspoints_start_i + (i + spokestep + 1) % num_spokes;
			f32 spring_len = dist(&masspoints[mp1_i], &masspoints[mp2_i]);

			Spring* s2 = springs.push_back_notconstructed();
			new (s2) Spring(mp1_i, mp2_i, spring_len);
			//s2->k *= 5;
			s2->line_width = radius / 10 / 3; // 2 cm.
			s2->color = IM_COL32(0, 0, 0, 255);

			if (rubberlevel == 0) {
				s2->set_outer_edge(true);
				s2->k *= 5;
				//s2->line_width = 2;
				s2->color = IM_COL32(30, 30, 30, 255);
			}
			if (rubberlevel == 1) {
				s2->color = IM_COL32(20, 20, 20, 255);
				s2->k *= 3;
			}
			if (rubberlevel == 2) {
				s2->color = IM_COL32(10, 10, 10, 255);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// private functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void _add_spring_force_to_masspoints(Spring* spring, Array<MassPoint>* out_masspoints) {
	Spring& s = *spring;
	_add_spring_force_to_masspoints(spring, &(*out_masspoints)[s.i_mp1], &(*out_masspoints)[s.i_mp2]);
}

inline void _add_spring_force_to_masspoints(Spring* spring, MassPoint* out_mp1, MassPoint* out_mp2) {
	Spring& s = *spring;
	MassPoint& mp1 = *out_mp1;
	MassPoint& mp2 = *out_mp2;

	f32 d = dist(&mp1, &mp2);
	if (d == 0.f) d = 0.1f; // don't know if this helps. unstability comes anyway.

	f32 f = s.k * (d - s.l); // f = -k * delta_x

	// TODO: check if this algo is better
	// # calc force vector xy components using f & d & positions. add forces.
	// #fx = (mp2.x - mp1.x) / d * f
	// #fy = (mp2.y - mp1.y) / d * f
	// xul = (mp2.x - mp1.x) / d
	// yul = (mp2.y - mp1.y) / d
	// damping = self.spring_damping # s.damping
	// if s.damping < 0.: damping = -s.damping
	// fx = xul * (f + damping * xul * ((mp2.x - mp2.prev_x) / dt - (mp1.x - mp1.prev_x) / dt))
	// fy = yul * (f + damping * yul * ((mp2.y - mp2.prev_y) / dt - (mp1.y - mp1.prev_y) / dt))

	// calc force vector xy components using f & d & positions. add forces.
	f32 fx = (mp2.x - mp1.x) / d * f;
	f32 fy = (mp2.y - mp1.y) / d * f;
	mp1.fx += fx;
	mp1.fy += fy;
	mp2.fx -= fx;
	mp2.fy -= fy;

	// mp1.vx *= s.damping;
	// mp1.vy *= s.damping;
	// mp2.vx *= s.damping;
	// mp2.vy *= s.damping;
}

// add_ay could be used as gravity.
inline void _move_masspoint_according_to_accumulated_forces(MassPoint* out_masspoint, f32 add_ax, f32 add_ay, f32 dt) {
	MassPoint &mp = *out_masspoint;

	if (!(mp.flags & MASSPOINT_ANCHOR)) {
		// move the masspoint according to accumulated forces

		// v = v0 + a*dt
		// f = m*a
		// v = v0 + f / m * dt
		f32 ax = mp.fx / mp.m + add_ax;
		f32 ay = mp.fy / mp.m + add_ay;
//		f32 dummy = dt / mp.m;
		mp.vx += ax * dt;
		mp.vy += ay * dt;
		//mp.vx *= damping;
		//mp.vy *= damping;

		// s = v0*t + a*t*t/2
		//mp.x += mp.vx * dt + dummy * mp.fx * dt / 2;
		//mp.y += mp.vy * dt + dummy * mp.fy * dt / 2;
		mp.x += mp.vx * dt + ax * dt * dt / 2.f;
		mp.y += mp.vy * dt + ay * dt * dt / 2.f;

		mp.vx *= 0.9999;
		mp.vy *= 0.9999;
	}
	// clear all forces. cleanup for the next timestep.
	mp.fx = mp.fy = 0;
}
