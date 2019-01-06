// Elmo Trolla, 2018
// License: pick one - UNLICENSE (www.unlicense.org) / MIT (opensource.org/licenses/MIT).

#define APP_VERSION "0.1.20190105"

//
// 0.1.20190105
//   * no mud. but semipassable sand. episode subtitle: "a day on the beach".
//
// 0.1.20190104
//   * left side of the world is now presentable.
//   * all line widths now in world-space, not in pixel-space. can resize the window, no swearing.
//   * add a slight shadow behind the bicycle frame.
//
// 0.1.20190103
//   * implement half-fake inverse-torque from spinning the wheel - spin the frame in the opposite direction.
//

//
// TODO:
//   * mud. implement mud.
//   * upgrade ground collision test. no sliding, no non-infinite friction at the moment.
//

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h" // for custom graph renderer

#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
//#define SOKOL_IMPL
#define SOKOL_GLES2
//#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_time.h"

#include "imgui-emsc.h"

#include "ground.h"
#include "world.h"
#include "jello.h"
#include "renderer.h"


void draw();

World world;
Renderer renderer;

bool show_test_window = true;


int main() {
	imgui_emsc_init(draw);
	return 0;
}

ImRect get_window_coords() {
	return ImRect(ImGui::GetWindowPos(), ImGui::GetWindowPos()+ImGui::GetWindowSize());
}


// the main function that is called for every new frame.
void draw() {

	// handle keyboard input

	bool keydown = false;
	float force = 7;

	float dt = 1.f / 60;
	int num_iterations = 10;

	for (int i = 0; i < num_iterations; i++) {
		world_tick_physics(&world, dt / num_iterations);

		if (ImGui::IsKeyDown( ImGui::GetKeyIndex(ImGuiKey_LeftArrow) )) {
			world_accelerate_bicycle(&world, force);
			keydown = true;
		} if (ImGui::IsKeyDown( ImGui::GetKeyIndex(ImGuiKey_RightArrow) )) {
			world_accelerate_bicycle(&world, -force);
			keydown = true;
		}
	}

	world_tick_frame(&world, dt);

	// io.KeyMap[ImGuiKey_LeftArrow] = 37;
	// io.KeyMap[ImGuiKey_RightArrow] = 39;
	// io.KeyMap[ImGuiKey_UpArrow] = 38;
	// io.KeyMap[ImGuiKey_DownArrow] = 40;

 	static bool show_main_window = false;
 	static bool show_demo_window = false;

 	ImGui::NewFrame();

 	//
 	// draw the background world inside a fullscreen window.
 	//

 	{
 		// For background color, look into imgui-emsc.cpp, SG_ACTION_CLEAR.

		// if background color is completely transparent, then drawing the background is skipped by imgui. checked that.
		ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0,0,0,0));

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 0);

		float window_w = ImGui::GetIO().DisplaySize.x;
		float window_h = ImGui::GetIO().DisplaySize.y;
		ImGui::SetNextWindowContentSize(ImVec2(window_w, window_h));
		ImGui::SetNextWindowSize(ImVec2(window_w, window_h));
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

		ImGui::Begin("Robota", NULL,
		             ImGuiWindowFlags_NoTitleBar |
		             ImGuiWindowFlags_NoMove |
		             ImGuiWindowFlags_NoResize |
		             ImGuiWindowFlags_NoCollapse |
		             ImGuiWindowFlags_NoSavedSettings |
		             ImGuiWindowFlags_NoBringToFrontOnFocus |
		             ImGuiWindowFlags_NoInputs |
		             ImGuiWindowFlags_NoScrollbar);

		ImRect windim = get_window_coords();

		renderer.tick(&world, windim.GetWidth(), windim.GetHeight());

		{
			// draw the text that appears if the player moves to out of the world to the left.

			ImGuiContext& g = *GImGui;
			f32 font_size = g.FontSize;
			g.FontSize = windim.GetWidth()/4;
			g.DrawListSharedData.FontSize = g.FontSize;

			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			draw_list->AddText(ImVec2(renderer.proj_x_world_to_pixel(0) - g.FontSize/2 * 10.2, renderer.proj_y_world_to_pixel( 1.35 + renderer.proj_height_pixel_to_world(g.FontSize/2) )), IM_COL32(0, 0, 0, 255), "muhucycle");
			// windim.GetHeight()/2 - 250/2
			g.FontSize = font_size;
		}

		renderer.render(&world);


		ImGui::SetCursorPos(ImVec2(windim.Max.x - 150, windim.Min.y));
		ImGui::Text("fps: %.1f (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

		ImGui::End();
		ImGui::PopStyleVar(7);

		ImGui::PopStyleColor();
	}

	//
	// draw debug windows
	//

	{
		ImGui::SetNextWindowSize(ImVec2(270,100), ImGuiSetCond_FirstUseEver);
		ImGui::Begin("dbg window", &show_main_window);

		ImGui::Text("movement: left/right arrows");
		ImGui::Text("version : " APP_VERSION);
		ImGui::Separator();

		if (ImGui::Button("Reset World?")) world_reset(&world);
		ImGui::SliderFloat("gravity", &world.gravity, 0.0f, -20.0f);
		ImGui::Text("cam %.3f %.3f", renderer.camera_x, renderer.camera_y);

		ImGui::Columns(2);
		ImGui::Separator();

		for (int i = 0; i < world.jelloasms.size(); i++) {
			JelloAssembly* ja = &world.jelloasms[i];
			ImGui::PushID(ja);

			//ImGui::AlignTextToFramePadding();  // Text and Tree nodes are less high than regular widgets, here we add vertical spacing to make the tree lines equal high.
			bool jello_assembly_node_open = ImGui::TreeNode("whatsthis", "JelloAssembly_%u", i);
			//ImGui::NextColumn();
			//ImGui::AlignTextToFramePadding();
			//ImGui::NextColumn();
			if (jello_assembly_node_open) {

				bool jellos_node_open = ImGui::TreeNode("whatsthis", "Jellos");
				if (jellos_node_open) {
					for (int k = 0; k < ja->jellos.size(); k++) {
						ImGui::PushID(k);
						ImGui::TreeNodeEx("Jellowhatsthis", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet, "Jello_%d", k);
						ImGui::PopID();
					}
					ImGui::TreePop();
				}

				bool cos_node_open = ImGui::TreeNode("whatsthis2", "CrossObjectSprings");
				if (cos_node_open) {
					for (int k = 0; k < ja->cross_object_springs.size(); k++) {
						ImGui::PushID(k);
						ImGui::TreeNodeEx("Crossowhatsthis", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet, "SrossObjectSpring_%d", k);
						ImGui::PopID();
					}
					ImGui::TreePop();
				}

		                ImGui::TreePop();
			}
			ImGui::PopID();
		}

		ImGui::Columns(1);
		ImGui::Separator();

		ImGui::Checkbox("show invisible springs", &renderer.debug_show_invisible_springs);
		#ifndef IMGUI_DISABLE_DEMO_WINDOWS
		ImGui::Checkbox("show imgui demo window", &show_demo_window);
		#endif
		ImGui::Text("particles: %i", world.ground.dirt_particles.particles.size());

		ImGui::End();
	}

	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

 	ImGui::EndFrame();
	ImGui::Render();
}
