// Elmo Trolla, 2018
// License: pick one - UNLICENSE (www.unlicense.org) / MIT (opensource.org/licenses/MIT).

#include "renderer.h"

#include "world.h"
#include "jello.h"


Renderer::Renderer() {
	camera_x = 5;
	camera_y = 2; // meters high
	debug_show_invisible_springs = false;
	_scaler = ImVec2(1,1);
}

void Renderer::_update_world_portal(float pix_w, float pix_h, float width_meters) {
	visible_world.Min.x = camera_x - width_meters / 2.f;
	visible_world.Max.x = camera_x + width_meters / 2.f;
	float h_meters = width_meters / pix_w * pix_h;
	visible_world.Min.y = camera_y - h_meters / 2.f;
	visible_world.Max.y = camera_y + h_meters / 2.f;
	_scaler = ImVec2(pixels_w, -pixels_h) / visible_world.GetSize();
}

void Renderer::tick(World* world, float pix_w, float pix_h) {
	pixels_w = pix_w;
	pixels_h = pix_h;

	// rubber-band the camera to world->player_center_mp
	MassPoint* mp = world->player_center_mp;
	camera_x += (mp->x - camera_x) * 0.1;

	_update_world_portal(pix_w, pix_h, 15); // keep always lastparam meters of horizontal space on screen.
}

// in world coordinates.
inline void Renderer::draw_line(float x1, float y1, float x2, float y2, u32 color, float thickness) {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	ImVec2 v1 = proj_vec_world_to_pixel(ImVec2(x1, y1));
	ImVec2 v2 = proj_vec_world_to_pixel(ImVec2(x2, y2));

	draw_list->AddLine(v1, v2, color, thickness);
}

void Renderer::render(World* world) {
	_render_ground(&world->ground);

	for (int ija = 0; ija < world->jelloasms.size(); ija++) {
		JelloAssembly* ja = &world->jelloasms[ija];
		for (int ijo = 0; ijo < ja->jellos.size(); ijo++) {
			_render_jello(&ja->jellos[ijo]);
		}
	}

	_render_cross_object_springs(&world->jelloasms, &world->crossobjectsprings);
}

void Renderer::_render_ground(Ground* ground) {

	// this function is hilarious. just waiting for a refactor.

	f32 world_x_start;
	f32 world_x_end;
	i32 ground_i_first;
	i32 ground_i_last;

	ImRect backup = visible_world;
	f32 backup_ground_start_x = ground->start_x;
	f32 backup_ground_tile_width = ground->TILE_WIDTH;

	f32 world_x_start_pixel = proj_x_world_to_pixel(0);

	ground->TILE_WIDTH *= 1.f;
	f32 scale = 2.f;
	f32 visible_world_width = visible_world.GetWidth();
	f32 visible_world_height = visible_world.GetHeight();
	visible_world.Min.x = camera_x - visible_world_width / 2.f * scale;
	//visible_world.Min.y = camera_y - visible_world_height / 2.f * scale;
	visible_world.Max.x = camera_x + visible_world_width / 2.f * scale;
	//visible_world.Max.y = camera_y + visible_world_height / 2.f * scale;
	_scaler = ImVec2(pixels_w, -pixels_h) / visible_world.GetSize();
	ground->start_x = -visible_world.GetWidth() / 2.f / scale;
	//-ground->TILE_WIDTH * ;


	// find start and end tile
	// TODO: isn't this visible_world.Min.x and Max.x?
//	world_x_start = proj_vec_pixel_to_world(ImVec2(0, 0))[0];
//	world_x_end   = proj_vec_pixel_to_world(ImVec2(pixels_w, 0))[0];
	world_x_start = visible_world.Min.x;
	world_x_end   = visible_world.Max.x;
	// .. convert start/end to ground array indices
	ground_i_first = floorf((world_x_start - ground->start_x) / ground->TILE_WIDTH);
	ground_i_last  = ceilf((world_x_end - ground->start_x) / ground->TILE_WIDTH);
	ground_i_first = clamp_i32(ground_i_first, 0, ELEMENTS_IN_ARRAY(ground->y_background_hills)-1);
	ground_i_last  = clamp_i32(ground_i_last, 0, ELEMENTS_IN_ARRAY(ground->y_background_hills)-1);


	if (world_x_start_pixel > 0) {
		ground_i_first = floorf((proj_x_pixel_to_world(world_x_start_pixel) - ground->start_x) / ground->TILE_WIDTH);
		ground_i_first = clamp_i32(ground_i_first, 0, ELEMENTS_IN_ARRAY(ground->y_background_hills)-1);
	}
//if frontview visible pixel xmin > 0
//if frontview world xmin > -visible width


	// ground is fully out of screen
	//if (ground_i_first == ground_i_last) return;


	// render rock layer
	{
		// f32 dx = ground->TILE_WIDTH;
		// f32 x1 = ground->start_x + ground_i_first * dx;
		// f32 y1 = ground->y_rock_layer[ground_i_first];
		// for (i32 i = ground_i_first + 1; i <= ground_i_last; i++) {
		// 	f32 y2 = ground->y_rock_layer[i];
		// 	f32 x2 = ground->start_x + i * dx;
		// 	draw_line(x1, y1, x2, y2, IM_COL32(100, 100, 100, 255), 2.f);
		// 	y1 = y2;
		// 	x1 = x2;
		// }
	}

	// render background hill layer

	// ground is fully out of screen
	if (ground_i_first < ground_i_last) {


		ImDrawList* dl = ImGui::GetWindowDrawList();
		f32 dx = ground->TILE_WIDTH;
		//u32 col = IM_COL32(50, 50, 50, 255);
		u32 col = IM_COL32(65, 65, 65, 255);
		f32 y_btm_pixel = pixels_h + 10; // move bottom 10 pixels below the screen
		ImVec2 uv(dl->_Data->TexUvWhitePixel); // us white pixel as our texture. this way we can simulate vector graphics with clean colors.

		ImDrawIdx idx = (ImDrawIdx)dl->_VtxCurrentIdx;
		dl->PrimReserve(6 * (ground_i_last - ground_i_first), 2 * (ground_i_last - ground_i_first + 1)); // int idx_count, int vtx_count
		dl->_VtxCurrentIdx += 2 * (ground_i_last - ground_i_first + 1); // we use idx to reduce typing. so we can just finalize the _VtxCurrentIdx right here, no need to wait for until the buffer is filled.

		for (i32 i = ground_i_first; i <= ground_i_last; i++) {
			ImVec2 coord_top = proj_vec_world_to_pixel(ImVec2(ground->start_x + i * dx, ground->y_background_hills[i]));
			ImVec2 coord_btm(coord_top.x, y_btm_pixel);

			//draw_line(x_left, y_left_top, x_right, y_right_top, col, 2.f);

			// pattern: move right, insert top and bottom. move right, take top and bottom. move right, insert top and bottom.

			dl->_VtxWritePtr[0].pos = coord_top;   dl->_VtxWritePtr[0].uv = uv;   dl->_VtxWritePtr[0].col = col;
			dl->_VtxWritePtr[1].pos = coord_btm;   dl->_VtxWritePtr[1].uv = uv;   dl->_VtxWritePtr[1].col = col;
			dl->_VtxWritePtr += 2;

			if (i > ground_i_first) {
				dl->_IdxWritePtr[0] = idx;   dl->_IdxWritePtr[1] = (ImDrawIdx)(idx+3);   dl->_IdxWritePtr[2] = (ImDrawIdx)(idx+1);
				dl->_IdxWritePtr[3] = idx;   dl->_IdxWritePtr[4] = (ImDrawIdx)(idx+2);   dl->_IdxWritePtr[5] = (ImDrawIdx)(idx+3);
				dl->_IdxWritePtr += 6;
				idx += 2;
			}
		}
	}


	visible_world = backup;
	_scaler = ImVec2(pixels_w, -pixels_h) / visible_world.GetSize();
	ground->start_x = backup_ground_start_x;
	ground->TILE_WIDTH = backup_ground_tile_width;


	// find start and end tile
	// TODO: isn't this visible_world.Min.x and Max.x?
	world_x_start = proj_vec_pixel_to_world(ImVec2(0, 0))[0];
	world_x_end   = proj_vec_pixel_to_world(ImVec2(pixels_w, 0))[0];
	// .. convert start/end to ground array indices
	ground_i_first = floorf((world_x_start - ground->start_x) / ground->TILE_WIDTH);
	ground_i_last  = ceilf((world_x_end - ground->start_x) / ground->TILE_WIDTH);
	ground_i_first = clamp_i32(ground_i_first, 0, ground->num_points_x()-1);
	ground_i_last  = clamp_i32(ground_i_last, 0, ground->num_points_x()-1);

	// ground is fully out of screen
	if (ground_i_first == ground_i_last) return;



	// render rock layer
	// Ideas/parts of this code are from the depths of ImGui.  void ImDrawList::PrimRect(const ImVec2& a, const ImVec2& c, ImU32 col)

	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		f32 dx = ground->TILE_WIDTH;
		//u32 col = IM_COL32(70, 70, 70, 255);
		u32 col = IM_COL32(52, 52, 52, 255);
		f32 y_btm_pixel = pixels_h + 10; // move bottom 10 pixels below the screen
		ImVec2 uv(dl->_Data->TexUvWhitePixel); // us white pixel as our texture. this way we can simulate vector graphics with clean colors.

		ImDrawIdx idx = (ImDrawIdx)dl->_VtxCurrentIdx;
		dl->PrimReserve(6 * (ground_i_last - ground_i_first), 2 * (ground_i_last - ground_i_first + 1)); // int idx_count, int vtx_count
		dl->_VtxCurrentIdx += 2 * (ground_i_last - ground_i_first + 1); // we use idx to reduce typing. so we can just finalize the _VtxCurrentIdx right here, no need to wait for until the buffer is filled.

		for (i32 i = ground_i_first; i <= ground_i_last; i++) {
			ImVec2 coord_top = proj_vec_world_to_pixel(ImVec2(ground->start_x + i * dx, ground->y_rock_layer[i]));
			ImVec2 coord_btm(coord_top.x, y_btm_pixel);

			//draw_line(x_left, y_left_top, x_right, y_right_top, col, 2.f);

			// pattern: move right, insert top and bottom. move right, take top and bottom. move right, insert top and bottom.

			dl->_VtxWritePtr[0].pos = coord_top;   dl->_VtxWritePtr[0].uv = uv;   dl->_VtxWritePtr[0].col = col;
			dl->_VtxWritePtr[1].pos = coord_btm;   dl->_VtxWritePtr[1].uv = uv;   dl->_VtxWritePtr[1].col = col;
			dl->_VtxWritePtr += 2;

			if (i > ground_i_first) {
				dl->_IdxWritePtr[0] = idx;   dl->_IdxWritePtr[1] = (ImDrawIdx)(idx+3);   dl->_IdxWritePtr[2] = (ImDrawIdx)(idx+1);
				dl->_IdxWritePtr[3] = idx;   dl->_IdxWritePtr[4] = (ImDrawIdx)(idx+2);   dl->_IdxWritePtr[5] = (ImDrawIdx)(idx+3);
				dl->_IdxWritePtr += 6;
				idx += 2;
			}
		}
	}

	// render dirt layer

	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		f32 dx = ground->TILE_WIDTH;
		//u32 col = IM_COL32(60, 60, 160, 255);
		//u32 col = IM_COL32(140, 100, 80, 255);
		u32 col = IM_COL32(105, 80, 65, 255);
		//f32 y_btm_pixel = pixels_h + 10; // move bottom 10 pixels below the screen
		ImVec2 uv(dl->_Data->TexUvWhitePixel); // us white pixel as our texture. this way we can simulate vector graphics with clean colors.

		ImDrawIdx idx = (ImDrawIdx)dl->_VtxCurrentIdx;
		dl->PrimReserve(6 * (ground_i_last - ground_i_first), 2 * (ground_i_last - ground_i_first + 1)); // int idx_count, int vtx_count
		dl->_VtxCurrentIdx += 2 * (ground_i_last - ground_i_first + 1); // we use idx to reduce typing. so we can just finalize the _VtxCurrentIdx right here, no need to wait for until the buffer is filled.

		for (i32 i = ground_i_first; i <= ground_i_last; i++) {
			ImVec2 coord_top = proj_vec_world_to_pixel(ImVec2(ground->start_x + i * dx, ground->y[i]));
			ImVec2 coord_btm(coord_top.x, proj_y_world_to_pixel(ground->y_rock_layer[i]));

			//draw_line(x_left, y_left_top, x_right, y_right_top, col, 2.f);

			// pattern: move right, insert top and bottom. move right, take top and bottom. move right, insert top and bottom.

			dl->_VtxWritePtr[0].pos = coord_top;   dl->_VtxWritePtr[0].uv = uv;   dl->_VtxWritePtr[0].col = col;
			dl->_VtxWritePtr[1].pos = coord_btm;   dl->_VtxWritePtr[1].uv = uv;   dl->_VtxWritePtr[1].col = col;
			dl->_VtxWritePtr += 2;

			if (i > ground_i_first) {
				dl->_IdxWritePtr[0] = idx;   dl->_IdxWritePtr[1] = (ImDrawIdx)(idx+3);   dl->_IdxWritePtr[2] = (ImDrawIdx)(idx+1);
				dl->_IdxWritePtr[3] = idx;   dl->_IdxWritePtr[4] = (ImDrawIdx)(idx+2);   dl->_IdxWritePtr[5] = (ImDrawIdx)(idx+3);
				dl->_IdxWritePtr += 6;
				idx += 2;
			}
		}
	}

	// render dirt layer top line.. antialiased smoothness.

	{
		//u32 col = IM_COL32(50, 200, 50, 255);
		u32 col = IM_COL32(180, 140, 100, 255);
		f32 dx = ground->TILE_WIDTH;
		f32 x1 = ground->start_x + ground_i_first * dx;
		f32 y1 = ground->y[ground_i_first];
		for (i32 i = ground_i_first + 1; i <= ground_i_last; i++) {
			f32 y2 = ground->y[i];
			f32 x2 = ground->start_x + i * dx;
			draw_line(x1, y1, x2, y2, col, proj_height_world_to_pixel(ground->top_layer_thickness));
			y1 = y2;
			x1 = x2;
		}
	}

}

void Renderer::_render_jello(JelloSingleObject* jello) {
	Array<MassPoint>& mps = jello->masspoints;

	for (int i = 0; i < jello->springs.size(); i++) {
		Spring* s = &jello->springs[i];
		if (s->flags & JELLO_FLAG_ELEMENT_UNUSED) continue;
		//if (s->flags & JELLO_FLAG_ELEMENT_VISIBLE)
		{
			float x1 = mps[s->i_mp1].x;
			float y1 = mps[s->i_mp1].y;
			float x2 = mps[s->i_mp2].x;
			float y2 = mps[s->i_mp2].y;
			_draw_spring(x1, y1, x2, y2, s->color, s->line_width, s->flags);
		}
	}
}

void Renderer::_render_cross_object_springs(Array<JelloAssembly>* jello_assemblys, Array<CrossObjectSpring>* springs) {
	for (int i = 0; i < springs->size(); i++) {
		CrossObjectSpring* s = &(*springs)[i];
		if (s->flags & JELLO_FLAG_ELEMENT_UNUSED) continue;
		if (s->flags & JELLO_FLAG_ELEMENT_VISIBLE) {
			MassPoint* mp1 = &(*jello_assemblys)[s->i_jello_assembly1].jellos[s->i_jello_single_object1].masspoints[s->i_mp1];
			MassPoint* mp2 = &(*jello_assemblys)[s->i_jello_assembly2].jellos[s->i_jello_single_object2].masspoints[s->i_mp2];

			float x1 = mp1->x;
			float y1 = mp1->y;
			float x2 = mp2->x;
			float y2 = mp2->y;
			_draw_spring(x1, y1, x2, y2, s->color, s->line_width, s->flags);
		}
	}
}

inline void Renderer::_draw_spring(f32 x1, f32 y1, f32 x2, f32 y2, u32 color, f32 line_width, u32 flags) {
	if (flags & JELLO_FLAG_SELECTED_IN_EDITOR) {
		draw_line(x1, y1, x2, y2, IM_COL32(255, 100, 100, 255), 3.f);
	} else {
		//if (flags & SPRING_OUTER_EDGE) {
		if (flags & JELLO_FLAG_ELEMENT_VISIBLE) {
			if (flags & SPRING_RENDER_METHOD_2) {
				// darken the color and use it as a slight shadow
				f32 d = 0.2;
				u32 dark_color = IM_COL32(((color>>IM_COL32_R_SHIFT)&0xff)*d, ((color>>IM_COL32_G_SHIFT)&0xff)*d, ((color>>IM_COL32_B_SHIFT)&0xff)*d, (color>>IM_COL32_A_SHIFT)&0xff);
				draw_line(x1, y1, x2, y2, dark_color, proj_height_world_to_pixel(line_width));
				draw_line(x1, y1, x2, y2, color, proj_height_world_to_pixel(line_width*0.9));
			} else {
				draw_line(x1, y1, x2, y2, color, proj_height_world_to_pixel(line_width));
			}
		} else if (debug_show_invisible_springs) {
			draw_line(x1, y1, x2, y2, IM_COL32(95, 95, 95, 255), 1.f);
		}
	}
}
