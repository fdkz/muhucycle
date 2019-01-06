// Elmo Trolla, 2018
// License: pick one - UNLICENSE (www.unlicense.org) / MIT (opensource.org/licenses/MIT).

#include "ground.h"

#include <new> // placement new

#include "sys_globals.h"


void DirtParticleSystem::append(f32 x, f32 y, f32 vx, f32 vy, f32 m, u32 color) {
	DirtParticle* pp = particles.push_back_notconstructed();
	new (pp) DirtParticle(x, y, vx, vy, m, color);
}

void DirtParticleSystem::tick(f32 dt) {
	for (int i = 0; i < particles.size(); i++) {
		DirtParticle* p = &particles[i];
		p->age += dt;
		if (p->age >= DIRT_PARTICLE_MAX_AGE) {
			p->age = -1;
			// swap the last element with the current dead element, and then remove the last.
			particles[i] = particles.back();
			particles.pop_back();
		} else if (!(p->flags & DIRT_PARTICLE_FLAG_PARTICLE_STOPPED)) {
			p->vx += ax * dt;
			p->vy += ay * dt;
			p->x += p->vx * dt;
			p->y += p->vy * dt;
		}
	}
}


int Ground::num_points_x() { return ELEMENTS_IN_ARRAY(y); }

Ground::Ground() {
	start_x = 0;
	start_y = 0;
	TILE_WIDTH = 0.05;
	top_layer_thickness = 0.02; // 2 cm

	//BACKGROUND_HILLS_TILE_WIDTH = float(ELEMENTS_IN_ARRAY(y)) / ELEMENTS_IN_ARRAY(y_background_hills) * TILE_WIDTH;
	BACKGROUND_HILLS_TILE_WIDTH = TILE_WIDTH;

	f32 y_finedetail[GROUND_NUM_POINTS];

	// build the coarse version
	{
		f32* yy = y;
		const float max_hill = 9.f;
		// fill heightfield with random data
		for (int i = 0; i < num_points_x(); i++) {
			float dy = ((float)random() / RAND_MAX - 0.5f) * max_hill;
			yy[i] = dy;
		}
		// smoothen
		for (int passes = 0; passes < 200; passes++) {
			for (int i = 0; i < num_points_x() - 1; i++) {
				float d = (yy[i+1] - yy[i]) * 0.5f;
				yy[i] += d;
				yy[i+1] -= d;
			}
		}
	}

	// build the fine-detail version. almost invisible.
	{
		f32* yy = y_finedetail;
		const float max_hill = 0.6f;
		// fill heightfield with random data
		for (int i = 0; i < num_points_x(); i++) {
			float dy = ((float)random() / RAND_MAX - 0.5f) * max_hill;
			yy[i] = dy;
		}
		// smoothen
		for (int passes = 0; passes < 40; passes++) {
			for (int i = 0; i < num_points_x() - 1; i++) {
				float d = (yy[i+1] - yy[i]) * 0.5f;
				yy[i] += d;
				yy[i+1] -= d;
			}
		}
	}

	// merge the coarse and fine-detail terrain
	for (int i = 0; i < num_points_x(); i++) {
		y[i] += y_finedetail[i];
	}

	// build the rock layer. just 10 cm below the dirt layer.
	for (int i = 0; i < num_points_x(); i++) {
		y_rock_layer[i] = y[i] - 0.1f;
	}

	// build the background hill layer
	{
		f32* yy = y_background_hills;
		const float max_hill = 9.f;
		// fill heightfield with random data
		for (int i = 0; i < ELEMENTS_IN_ARRAY(y_background_hills); i++) {
			float dy = ((float)random() / RAND_MAX - 0.5f) * max_hill;
			yy[i] = dy + 2.5;
		}
		// smoothen
		for (int passes = 0; passes < 200; passes++) {
			for (int i = 0; i < ELEMENTS_IN_ARRAY(y_background_hills) - 1; i++) {
				float d = (yy[i+1] - yy[i]) * 0.5f;
				yy[i] += d;
				yy[i+1] -= d;
			}
		}
	}
}

float Ground::get_ground_height(float x) {
	x -= start_x;
	int i = x / TILE_WIDTH;
	if (i < 0) return y[0];
	if (i >= num_points_x() - 1) return y[num_points_x() - 1];

	float x1 = i * TILE_WIDTH;
	float x2 = i * TILE_WIDTH + TILE_WIDTH;
	// Dy / Dx = dy / dx ; dy = Dy * dx / Dx
	return (y[i+1] - y[i]) * (x - x1) / (x2 - x1) + y[i];
}

void Ground::raise_top_ground(f32 x, f32 dy) {
	x -= start_x;
	int i = x / TILE_WIDTH;
	if (i < 1) return;
	if (i >= num_points_x() - 1) return;

	y[i]   += dy * 0.6f;
	y[i-1] += dy * 0.2f;
	y[i+1] += dy * 0.2f;
}
