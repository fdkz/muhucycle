// Elmo Trolla, 2018
// License: pick one - UNLICENSE (www.unlicense.org) / MIT (opensource.org/licenses/MIT).

#pragma once

#include "sys_globals.h"

#define DIRT_PARTICLE_FLAG_PARTICLE_STOPPED 0x00000001

struct DirtParticle {
	DirtParticle(f32 x, f32 y, f32 vx, f32 vy, f32 m=0.01, u32 color=IM_COL32(200, 200, 200, 255)): x(x), y(y), vx(vx), vy(vy), m(m), color(color), age(-1), flags(0) {}
	f32 x, y;
	f32 vx, vy;
	f32 m;
	u32 color;
	f32 age; // -1 is dead
	u32 flags;
};

struct DirtParticleSystem {
	#define DIRT_PARTICLE_MAX_AGE 1.2f
	f32 ax, ay;
	Array<DirtParticle> particles;
	DirtParticleSystem(): ax(0), ay(0) {}

	void set_acceleration(f32 _ax, f32 _ay) { ax = _ax; ay = _ay; }
	//void append(f32 x, f32 y, f32 vx, f32 vy, f32 m=0.01, u32 color=IM_COL32(80, 60, 40, 255));
	void append(f32 x, f32 y, f32 vx, f32 vy, f32 m=0.01, u32 color=IM_COL32(30, 20, 10, 255));
	void tick(f32 dt);
};

struct Ground {
	#define GROUND_NUM_POINTS 4000
	Ground();
	float y[GROUND_NUM_POINTS]; // dirt layer
	float y_rock_layer[GROUND_NUM_POINTS];

	float y_background_hills[(int)(GROUND_NUM_POINTS*1)];
	float BACKGROUND_HILLS_TILE_WIDTH;

	float top_layer_thickness; // just a visual beautification layer. thin antialiased line on top of the ground.

	float TILE_WIDTH;
	float start_x;
	float start_y;

	DirtParticleSystem dirt_particles;

	int   num_points_x();
	float get_ground_height(float x);
	void  raise_top_ground(f32 x, f32 dy);
};
