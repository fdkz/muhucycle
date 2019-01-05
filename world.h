// Elmo Trolla, 2018
// License: pick one - UNLICENSE (www.unlicense.org) / MIT (opensource.org/licenses/MIT).

#pragma once

#include "jello.h"
#include "ground.h"

#include "sys_globals.h"


struct World {
	World();
	~World();
	void _destroy_jellos();

	Ground ground;
	Array<JelloAssembly> jelloasms;
	Array<CrossObjectSpring> crossobjectsprings;
	float gravity;

	u32 front_wheel_center_mp_i;
	u32 back_wheel_center_mp_i;

	MassPoint* player_center_mp;
};

void world_tick(World* out_world, float dt);
void world_reset(World* out_world);
void world_accelerate_bicycle(World* out_world, float force);

