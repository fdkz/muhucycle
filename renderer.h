// Elmo Trolla, 2018
// License: pick one - UNLICENSE (www.unlicense.org) / MIT (opensource.org/licenses/MIT).

#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h" // ImVec2
#include "imgui_internal.h" // ImRect

#include "sys_globals.h"

struct World;
struct Ground;
struct JelloSingleObject;
struct JelloAssembly;
struct CrossObjectSpring;


struct Renderer {
	Renderer();
	ImRect visible_world; // updated according to camera movement and window size changes.
	float pixels_w; // user should update these according to window size.
	float pixels_h;
	float camera_x; // meters
	float camera_y;
	bool debug_show_invisible_springs;
	ImVec2 _scaler;

	void _update_world_portal(float pix_w, float pix_h, float width_meters);
	void tick(World* world, float pix_w, float pix_h);

	// project coord from world space to pixel space
	// in a better world this would be done in vertex shader, but we don't have time for that now.
	// pixels: top-left is 0, 0.
	inline ImVec2 proj_vec_world_to_pixel(const ImVec2& v) const;
	// project coord from pixel space to world space
	inline ImVec2 proj_vec_pixel_to_world(const ImVec2& v) const;

	inline f32 proj_x_world_to_pixel(const f32& v) const;
	inline f32 proj_y_world_to_pixel(const f32& v) const;
	inline f32 proj_x_pixel_to_world(const f32& v) const;
	inline f32 proj_y_pixel_to_world(const f32& v) const;
	inline f32 proj_height_world_to_pixel(const f32& v) const;
	inline f32 proj_width_world_to_pixel (const f32& v) const;
	inline f32 proj_height_pixel_to_world(const f32& v) const;
	inline f32 proj_width_pixel_to_world (const f32& v) const;

	// in world coordinates.
	inline void draw_line(float x1, float y1, float x2, float y2, u32 color=IM_COL32(255, 0, 0, 255), float thickness=1.f);

	void render(World* world);

	void _render_ground(Ground* ground);
	void _render_jello(JelloSingleObject* jello);
	void _render_cross_object_springs(Array<JelloAssembly>* jello_assemblys, Array<CrossObjectSpring>* springs);
	void _draw_spring(f32 x1, f32 y1, f32 x2, f32 y2, u32 color, f32 line_width, u32 flags);
};

inline ImVec2 Renderer::proj_vec_world_to_pixel(const ImVec2& v) const { return (v - visible_world.Min) * _scaler + ImVec2(0, pixels_h); }
inline ImVec2 Renderer::proj_vec_pixel_to_world(const ImVec2& v) const { return visible_world.Min + (v - ImVec2(0, pixels_h)) / _scaler; }

inline f32 Renderer::proj_x_world_to_pixel(const f32& v) const { return (v - visible_world.Min.x) * _scaler.x; }
inline f32 Renderer::proj_y_world_to_pixel(const f32& v) const { return (v - visible_world.Min.y) * _scaler.y + pixels_h; }
inline f32 Renderer::proj_x_pixel_to_world(const f32& v) const { return visible_world.Min.x + v / _scaler.x; }
inline f32 Renderer::proj_y_pixel_to_world(const f32& v) const { return visible_world.Min.y + (v - pixels_h) / _scaler.y; }
inline f32 Renderer::proj_height_world_to_pixel(const f32& v) const { return v * _scaler.x; }
inline f32 Renderer::proj_width_world_to_pixel (const f32& v) const { return v * _scaler.y; }
inline f32 Renderer::proj_height_pixel_to_world(const f32& v) const { return v / _scaler.x; }
inline f32 Renderer::proj_width_pixel_to_world (const f32& v) const { return v / _scaler.y; }
