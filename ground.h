// Elmo Trolla, 2018
// License: pick one - UNLICENSE (www.unlicense.org) / MIT (opensource.org/licenses/MIT).

#pragma once


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

	int   num_points_x();
	float get_ground_height(float x);
};
