// Elmo Trolla, 2018
// License: pick one - UNLICENSE (www.unlicense.org) / MIT (opensource.org/licenses/MIT).

#pragma once

#include <string> // std::string
#include <math.h> // sqrt
//#include "imgui.h" // ImVector
#include "sys_globals.h" // u32, .. typedefs

// masks:
// 000000ff : universal flags
// 0000ff00 : future
// 00ff0000 : struct-specific flags
// ff000000 : sub-struct-specific flags

#define JELLO_FLAG_ELEMENT_UNUSED     0x00000001 // ie. can be used to almost delete springs. not used in calculations, not rendered.
#define JELLO_FLAG_SELECTED_IN_EDITOR 0x00000002
#define JELLO_FLAG_ELEMENT_VISIBLE    0x00000004

#define MASSPOINT_ANCHOR       0x00010000
#define MASSPOINT_NO_COLLISION 0x00020000

struct MassPoint {
	MassPoint(f32 x, f32 y, f32 m): x(x), y(y), m(m), vx(0), vy(0), fx(0), fy(0), flags(0), color(IM_COL32(240, 240, 240, 255)) {}
	f32 m;
	f32 x, y;
	f32 vx, vy;
	f32 fx, fy;
	u32 color;
	u32 flags; // MASSPOINT_VISIBLE, MASSPOINT_ANCHOR

	void set_visible(bool b){ if (b) flags |= JELLO_FLAG_ELEMENT_VISIBLE; else flags &= ~JELLO_FLAG_ELEMENT_VISIBLE; }
	void set_anchor(bool b) { if (b) flags |= MASSPOINT_ANCHOR;           else flags &= ~MASSPOINT_ANCHOR; }
};

#define SPRING_OUTER_EDGE      0x00020000
#define SPRING_RENDER_METHOD_2 0x00040000

struct Spring {
	Spring(u16 i_mp1, u16 i_mp2, f32 l): i_mp1(i_mp1), i_mp2(i_mp2), l(l), k(1000), damping(0.99992), flags(JELLO_FLAG_ELEMENT_VISIBLE), color(IM_COL32(200, 200, 200, 255)), line_width(1) { }
	f32 l; // rest length
	f32 k; // stiffness
	f32 damping;
	// TODO: test the algo speed with u16, u32 and *MassPoint indexing modes.
	u16 i_mp1;
	u16 i_mp2;
	//MassPoint *p1, *p2;
	// info for visualization. maybe move the visualization info to a different SpringVisual class for better cache streamlining?
	u32 flags; // SPRING_VISIBLE, SPRING_OUTER_EDGE, SPRING_UNUSED, ..
	u32 color;
	f32 line_width;

	void set_visible(bool b)    { if (b) flags |= JELLO_FLAG_ELEMENT_VISIBLE;    else flags &= ~JELLO_FLAG_ELEMENT_VISIBLE; }
	void set_unused(bool b)     { if (b) flags |= JELLO_FLAG_ELEMENT_UNUSED;     else flags &= ~JELLO_FLAG_ELEMENT_UNUSED; }
	void set_outer_edge(bool b) { if (b) flags |= SPRING_OUTER_EDGE;             else flags &= ~SPRING_OUTER_EDGE; }
};

#define CROSS_OBJECT_SPRING_HOLD_ZERO_LENGTH 0x01000000

// spring that connects masspoints from different objects.
struct CrossObjectSpring: public Spring {
	CrossObjectSpring(u16 i_mp1, u16 i_mp2, f32 l): Spring(i_mp1, i_mp2, l) {}
	u16 i_jello_assembly1;
	u16 i_jello_assembly2;
	u16 i_jello_single_object1;
	u16 i_jello_single_object2;
};

// a wheel, a ball, a bicycle frame, ..
struct JelloSingleObject {
	Array<Spring>    springs;
	Array<MassPoint> masspoints;
	u32 flags;
	//u32 parent_id;
	//u32 id;
	std::string class_name; // back_wheel, frame, front_wheel, ..
};

// for example a bicycle that consists of wheels, frame, .. JelloObject-s
struct JelloAssembly {
	~JelloAssembly();
	Array<CrossObjectSpring> cross_object_springs;
	Array<JelloSingleObject> jellos;
	u32 flags;
	//u32 id;
	std::string class_name; // bicycle, car, electric_bicycle, ..
};

// first tick forces, then tick positions.

void jello_tick_spring_forces(JelloSingleObject* out_jello);
void jello_tick_positions(JelloSingleObject* out_jello, f32 add_ax, f32 add_ay, f32 dt);
void jello_tick_spring_forces(JelloAssembly* out_jello_assembly);
void jello_tick_positions(JelloAssembly* out_jello_assembly, f32 add_ax, f32 add_ay, f32 dt);
void jello_tick_crossobject_spring_forces(Array<JelloAssembly>* jello_assemblys, Array<CrossObjectSpring>* springs);

//void jello_handle_crossobjectspring(); ?
// void jello_join_masspoints(MassPoint* mp1, MassPoint* mp2);

void jello_translate(JelloSingleObject* out_jello, f32 dx, f32 dy);
void jello_translate(JelloAssembly* out_jello_assembly, f32 dx, f32 dy);
void jello_scale(JelloSingleObject* out_jello, f32 wmult, f32 hmult);
void jello_recalculate_spring_lengths(JelloSingleObject* out_jello);
void jello_apply_rotational_force(JelloSingleObject* out_jello, f32 x, f32 y, f32 force); // apply force around given coordinates

void jello_generate_rectangular(JelloSingleObject* out_jello, u32* out_center_mp_i, f32 spring_len, f32 masspoint_mass, int x_num, int y_num);
void jello_generate_wheel(JelloSingleObject* out_jello, u32* out_center_mp_i, f32 radius, int num_spokes, f32 masspoint_mass, f32 x, f32 y);

inline f32 jello_masspoint_dist(MassPoint* p1, MassPoint* p2) {
	f32 dx = p2->x - p1->x; f32 dy = p2->y - p1->y;
	return sqrt(dx*dx + dy*dy);
}
