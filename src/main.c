char* SHADER_PATHS[RM_COUNT] = {
	[RM_NORMAL] = "shaders/main.spv",
	[RM_DEBUG] = "shaders/debug.spv"
};

void register_mesh(Mesh mesh, const char* name) {
	global->meshes[global->num_meshes] = mesh;
	ASSERT(strlen(name) <= NAME_LEN);
	snprintf(global->mesh_names[global->num_meshes], NAME_LEN, name);
	global->num_meshes++;
}

MeshID get_mesh_id(const char* name) {
	FOR(mesh_index, global->num_meshes) {
		if (strcmp(global->mesh_names[mesh_index], name) != 0) {
			continue;
		}

		return mesh_index;
	}

	TRAP("Mesh \"%s\" not found", name);
	return 0;
}

Object* register_object(const char* name) {
	ASSERT(strlen(name) <= NAME_LEN);
	snprintf(global->object_names[global->num_objects], NAME_LEN, name);

	Object* object = &global->objects[global->num_objects++];
	object->scale = vec3(1, 1, 1);
	object->scene_id = global->current_scene_id;
	object->material = (Material){
		.color = vec3(1, 1, 1),
		.roughness = 0.5f
	};

	return object;
}

Object* get_object(const char* name) {
	FOR(object_index, global->num_objects) {
		if (strcmp(global->object_names[object_index], name) != 0) {
			continue;
		}

		return &global->objects[object_index];
	}

	TRAP("Object \"%s\" not found", name);
	return 0;
}

void draw_frame(VulkanState* vulkan_state, uint32 current_frame, RendererState renderer_state) {
	/*================================*/
	/*      SEND OBJECTS TO GPU       */
	/*================================*/

	RenderObject* render_objects = alloc(global->num_objects * sizeof(RenderObject));

	global->renderer_state.num_objects = global->num_objects;
	uint light_index = 0;
	uint non_light_index = 0;
	FOR(object_index, global->num_objects) {
		Object* object = global->objects + object_index;

		if (object->scene_id != global->current_scene_id) continue;

		Mat4 translation = {
			1, 0, 0, object->position.x,
			0, 1, 0, object->position.y,
			0, 0, 1, object->position.z,
			0, 0, 0, 1
		};

		Mat4 inv_translation = {
			1, 0, 0, -object->position.x,
			0, 1, 0, -object->position.y,
			0, 0, 1, -object->position.z,
			0, 0, 0, 1
		};

		Mat4 rot_x = mat4_rot_x(object->orientation.x);
		Mat4 rot_y = mat4_rot_y(object->orientation.y);
		Mat4 rot_z = mat4_rot_z(object->orientation.z);
		Mat4 rotation = mat4_mul(mat4_mul(rot_y, rot_z), rot_x);

		Mat4 inv_rotation = mat4_transpose(rotation);

		Mat4 scale = {
			object->scale.x, 0, 0, 0,
			0, object->scale.y, 0, 0,
			0, 0, object->scale.z, 0,
			0, 0, 0, 1
		};

		Mat4 inv_scale = {
			1.0f / object->scale.x, 0, 0, 0,
			0, 1.0f / object->scale.y, 0, 0,
			0, 0, 1.0f / object->scale.z, 0,
			0, 0, 0, 1
		};

		Mat4 transform = mat4_mul(mat4_mul(translation, rotation), scale);
		Mat4 inv_transform = mat4_mul(inv_scale, mat4_mul(inv_rotation, inv_translation));

		// Pick a slot for the object
		// If emissive start from front, otherwise start from back
		RenderObject* render_object;
		Vec3 color = object->material.color;
		if (color.r > 1 + 1e-5 || color.g > 1 + 1e-5 || color.b > 1 + 1e-5)
		{
			render_object = &render_objects[light_index];
			light_index++;
		}
		else
		{
			render_object = &render_objects[global->num_objects - non_light_index - 1];
			non_light_index++;
		}

		render_object->transform = transform;
		render_object->inv_transform = inv_transform;

		render_object->bvh_root_offset = global->meshes[object->mesh_id].root_node_offset;
		render_object->triangle_offset = global->meshes[object->mesh_id].triangle_offset;

		render_object->material = object->material;
	}

	global->renderer_state.num_lights = light_index;

	memcpy(
			global->vulkan_state.object_buffers_mapped[global->current_frame],
			render_objects,
			global->num_objects * sizeof(RenderObject));

	/*======================================*/
	/*               COMPUTE 				*/
	/*======================================*/

	memcpy(vulkan_state->renderer_state_buffers_mapped[current_frame], &global->renderer_state, sizeof(RendererState));

	VK_ASSERT(vkWaitForFences(vulkan_state->device, 1, &vulkan_state->compute_in_flight_fences[current_frame], VK_TRUE, UINT64_MAX));
	VK_ASSERT(vkResetFences(vulkan_state->device, 1, &vulkan_state->compute_in_flight_fences[current_frame]));

	VK_ASSERT(vkResetCommandBuffer(vulkan_state->compute_command_buffers[current_frame], 0));
	VK_ASSERT(vkBeginCommandBuffer(vulkan_state->compute_command_buffers[current_frame], &vulkan_state->command_buffer_begin_info));

	vkCmdBindPipeline(
			vulkan_state->compute_command_buffers[current_frame],
			VK_PIPELINE_BIND_POINT_COMPUTE,
			vulkan_state->compute_pipeline);

	vkCmdBindDescriptorSets(
			vulkan_state->compute_command_buffers[current_frame],
			VK_PIPELINE_BIND_POINT_COMPUTE,
			vulkan_state->compute_pipeline_layout,
			0, 1,
			&vulkan_state->compute_descriptor_sets[current_frame],
			0, NULL);

	const uint32 local_size = 8;
	vkCmdDispatch(
			vulkan_state->compute_command_buffers[current_frame],
			320 / local_size + (320 % local_size != 0),
			180 / local_size + (180 % local_size != 0),
			1);

	VK_ASSERT(vkEndCommandBuffer(vulkan_state->compute_command_buffers[current_frame]));

	transition_image_layout(
		vulkan_state,
		vulkan_state->render_texture_image,
		vulkan_state->surface_format.format,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL);

	VkSemaphore compute_signal_semaphores[] = {
		vulkan_state->compute_finished_semaphores[current_frame]
	};

	VkSubmitInfo compute_submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &vulkan_state->compute_command_buffers[current_frame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = compute_signal_semaphores
	};

	VK_ASSERT(vkQueueSubmit(
			vulkan_state->graphics_queue,
			1, &compute_submit_info,
			vulkan_state->compute_in_flight_fences[current_frame]));

	/*======================================*/
	/*               GRAPHICS 				*/
	/*======================================*/

	VK_ASSERT(vkWaitForFences(vulkan_state->device, 1, &vulkan_state->in_flight_fences[current_frame], VK_TRUE, UINT64_MAX));
	VK_ASSERT(vkResetFences(vulkan_state->device, 1, &vulkan_state->in_flight_fences[current_frame]));

	uint32 image_index;
	vkAcquireNextImageKHR(
			vulkan_state->device,
			vulkan_state->swapchain,
			UINT64_MAX,
			vulkan_state->image_available_semaphores[current_frame],
			VK_NULL_HANDLE,
			&image_index);

	vulkan_state->render_pass_begin_info.framebuffer = vulkan_state->framebuffers[image_index];

	VK_ASSERT(vkResetCommandBuffer(vulkan_state->command_buffers[current_frame], 0));
	VK_ASSERT(vkBeginCommandBuffer(vulkan_state->command_buffers[current_frame], &vulkan_state->command_buffer_begin_info));

	vkCmdBeginRenderPass(
			vulkan_state->command_buffers[current_frame],
			&vulkan_state->render_pass_begin_info,
			VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(
			vulkan_state->command_buffers[current_frame],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			vulkan_state->graphics_pipeline);

	vkCmdSetViewport(vulkan_state->command_buffers[current_frame], 0, 1, &vulkan_state->viewport);
	vkCmdSetScissor(vulkan_state->command_buffers[current_frame], 0, 1, &vulkan_state->scissor);

	vkCmdBindDescriptorSets(
			vulkan_state->command_buffers[current_frame],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			vulkan_state->graphics_pipeline_layout,
			0,
			1, &vulkan_state->descriptor_sets[current_frame],
			0, NULL);

	vkCmdDraw(vulkan_state->command_buffers[current_frame], 6, 1, 0, 0);

	vkCmdEndRenderPass(vulkan_state->command_buffers[current_frame]);

	VK_ASSERT(vkEndCommandBuffer(vulkan_state->command_buffers[current_frame]));

	VkSemaphore wait_semaphores[] = {
		vulkan_state->compute_finished_semaphores[current_frame],
		vulkan_state->image_available_semaphores[current_frame]
	};

	VkSemaphore signal_semaphores[] = {
		vulkan_state->render_finished_semaphores[current_frame]
	};

	VkPipelineStageFlags wait_stages[] = {
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 2,
		.pWaitSemaphores = wait_semaphores,
		.pWaitDstStageMask = wait_stages,
		.commandBufferCount = 1,
		.pCommandBuffers = &vulkan_state->command_buffers[current_frame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signal_semaphores
	};

	transition_image_layout(
			vulkan_state,
			vulkan_state->render_texture_image,
			vulkan_state->surface_format.format,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VK_ASSERT(vkQueueSubmit(vulkan_state->graphics_queue, 1, &submit_info, vulkan_state->in_flight_fences[current_frame]));

	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signal_semaphores,
		.swapchainCount = 1,
		.pSwapchains = &vulkan_state->swapchain,
		.pImageIndices = &image_index
	};

	vkQueuePresentKHR(vulkan_state->present_queue, &present_info);

	global->renderer_state.num_frames++;
}

void load_trace_shader(char* path) {
	VulkanState* vulkan_state = &global->vulkan_state;

	// Create shader module
	uint32 compute_shader_code_size;
	char* compute_shader_code = platform_read_file(path, &compute_shader_code_size);

	VkShaderModuleCreateInfo compute_shader_create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = compute_shader_code_size,
		.pCode = (uint32*)compute_shader_code
	};

	VkShaderModule compute_shader_module;
	vkCreateShaderModule(vulkan_state->device, &compute_shader_create_info, NULL, &compute_shader_module);

	VkPipelineShaderStageCreateInfo compute_shader_stage_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = compute_shader_module,
		.pName = "main"
	};

	// Create pipeline
	VkComputePipelineCreateInfo compute_pipeline_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.layout = vulkan_state->compute_pipeline_layout,
		.stage = compute_shader_stage_create_info
	};

	VK_ASSERT(vkCreateComputePipelines(
			vulkan_state->device,
			VK_NULL_HANDLE,
			1, &compute_pipeline_create_info,
			NULL,
			&vulkan_state->compute_pipeline));

	printf("Rebuilt compute pipeline\n");
}

__declspec(dllexport)
void game_update_and_render(Inputs inputs) {
	// Check for changes to compute shader
	char* shader_path = SHADER_PATHS[global->current_render_mode];
	uint64 compute_shader_modified_time = platform_get_file_modified_time(shader_path);
	if (compute_shader_modified_time > global->prev_compute_shader_modified_time) {
		global->prev_compute_shader_modified_time = compute_shader_modified_time;
		load_trace_shader(shader_path);
	}

	// Input
	if (inputs.f1 && !global->prev_inputs.f1) {
		if (global->current_render_mode != RM_NORMAL) {
			global->current_render_mode = RM_NORMAL;
			load_trace_shader(SHADER_PATHS[global->current_render_mode]);
		}
	}

	if (inputs.f2 && !global->prev_inputs.f2) {
		if (global->current_render_mode != RM_DEBUG) {
			global->current_render_mode = RM_DEBUG;
			load_trace_shader(SHADER_PATHS[global->current_render_mode]);
		}
	}

	FOR(i, 10) {
		if (!inputs.nums[i] || global->prev_inputs.nums[i]) continue;
		global->current_scene_id = i;
	}

	if (inputs.p && !global->prev_inputs.p) {
		global->renderer_state.progressive ^= true;
		global->renderer_state.num_frames = 1;
	}
 
	// Update
	float total_time = global->platform_data->total_time;
	float delta_time = global->platform_data->delta_time;

	Object* suz = get_object("suz");
	suz->position.y = sinf(total_time);
	suz->orientation.y += delta_time;

	// Draw
	global->renderer_state.time = global->platform_data->total_time;
	draw_frame(&global->vulkan_state, global->current_frame, global->renderer_state);
	global->current_frame = (global->current_frame + 1) % 2;

	global->prev_inputs = inputs;

	clear_arena(&global->frame_arena);
}

void game_start() {
	// Cornell box
	global->current_scene_id = 1;

	Object* suz = register_object("suz");
	suz->mesh_id = get_mesh_id("suzanne.obj");
	suz->material.color = vec3(0, 1, 0);

	Object* ground = register_object("ground");
	ground->mesh_id = get_mesh_id("plane.obj");
	ground->position = vec3(0, -5, 0);
	ground->scale = vec3(5, 1, 5);

	Object* ceiling = register_object("ceiling");
	ceiling->mesh_id = get_mesh_id("plane.obj");
	ceiling->position = vec3(0, 5, 0);
	ceiling->orientation = vec3(tau, 0, 0);
	ceiling->scale = vec3(5, 1, 5);

	Object* wall_left = register_object("wall_left");
	wall_left->mesh_id = get_mesh_id("plane.obj");
	wall_left->position = vec3(-5, 0, 0);
	wall_left->orientation = vec3(pi, 0, 0);
	wall_left->material.color = vec3(1, 0, 0);
	wall_left->scale = vec3(5, 1, 5);

	Object* wall_right = register_object("wall_right");
	wall_right->mesh_id = get_mesh_id("plane.obj");
	wall_right->position = vec3(5, 0, 0);
	wall_right->orientation = vec3(pi, 0, 0);
	wall_right->material.color = vec3(0, 1, 0);
	wall_right->scale = vec3(5, 1, 5);

	Object* wall_back = register_object("wall_back");
	wall_back->mesh_id = get_mesh_id("plane.obj");
	wall_back->position = vec3(0, 0, 5);
	wall_back->orientation = vec3(pi, 0, 0);
	wall_back->scale = vec3(5, 1, 5);

	Object* light = register_object("light");
	light->mesh_id = get_mesh_id("plane.obj");
	light->position = vec3(0, 4.9, 0);
	light->orientation = vec3(tau, 0, 0);
	light->material = (Material){
		.color = vec3(2, 2, 2)
	};
	light->scale = vec3(1, 1, 1);

	// Monkey
	global->current_scene_id = 0;

	Object* top_light = register_object("top_light");
	top_light->mesh_id = get_mesh_id("plane.obj");
	top_light->material = (Material){
		.color = vec3(3, 3, 3)
	};
	top_light->orientation = vec3(pi, pi / 4, 0);
	top_light->position = vec3(0, 5, 0);
	top_light->scale = vec3(2, 1, 2);

	Object* bottom_light = register_object("bottom_light");
	bottom_light->mesh_id = get_mesh_id("plane.obj");
	bottom_light->material = (Material){
		.color = vec3(3, 3, 3)
	};
	bottom_light->orientation = vec3(pi, pi / 4, pi);
	bottom_light->position = vec3(0, -2, 0);
	bottom_light->scale = vec3(2, 1, 2);

	Object* lit_monkey = register_object("lit_monkey");
	lit_monkey->mesh_id = get_mesh_id("suzanne.obj");
	lit_monkey->material.color = vec3(1, 1, 0);

	Object* lit_cube = register_object("lit_cube");
	lit_cube->mesh_id = get_mesh_id("cube_cool.obj");
	lit_cube->position = vec3(-3, -2, 0);

	Object* lit_plane = register_object("lit_plane");
	lit_plane->mesh_id = get_mesh_id("plane.obj");
	lit_plane->position = vec3(0, -2 - 1e-5f, 0);
	lit_plane->scale = vec3(10, 1, 10);

	Object* red_cube = register_object("red_cube");
	red_cube->mesh_id = get_mesh_id("cube_cool.obj");
	red_cube->material.color = vec3(1, 0, 0);
	red_cube->position = vec3(3, -2, 0);
	red_cube->scale = vec3(0.5f, 5, 1);
}

__declspec(dllexport)
void game_init(
	PlatformData* platform_data,
	VulkanPlatformData vulkan_platform_data,
	void* memory,
	uint memory_size)
{
	global = memory;
	global->base_arena = (Arena){
		.base = memory,
		.head = (char*)memory + sizeof(GlobalMemory),
		.size = memory_size
	};
	push_arena(&global->base_arena);

	global->platform_data = platform_data;

	load_vulkan(vulkan_platform_data);

	/*==========================*/
	/*       LOAD ASSETS        */
	/*==========================*/

	printf("Loading assets...\n");
	float loading_assets_start_time = platform_get_time_ms();

	global->asset_arena = branch_arena(kb(512));
	push_arena(&global->asset_arena);
	global->bvh_buffer = alloc(kb(256));
	global->triangle_buffer = alloc(kb(256));
	pop_arena();

	Arena asset_temp_arena = spawn_arena(mb(2));
	push_arena(&asset_temp_arena);

	char path_buffer[256] = "assets/";
	char** asset_names = platform_read_dir("assets\\*");
	global->triangle_buffer_size = 0;
	global->bvh_buffer_size = 0;
	for (int i = 0;; i++) {
		char* current_asset_name = asset_names[i];
		if (current_asset_name == NULL) {
			break;
		}

		Mesh mesh = {
			.root_node_offset = global->bvh_buffer_size,
			.triangle_offset = global->triangle_buffer_size
		};

		printf("\"%s\":\n", current_asset_name);

		memcpy(path_buffer + 7, current_asset_name, strlen(current_asset_name) + 1);
		char* file_contents = platform_read_file(path_buffer, NULL);

		// Load triangles
		Triangle* triangles;
		uint32 num_triangles = parse_obj(file_contents, &triangles);
		printf("- Triangles: %i\n", num_triangles);

		// Build BVH and reorder triangles
		BVHNodeFlat* bvh_nodes;
		uint32 num_bvh_nodes = build_bvh(triangles, num_triangles, &bvh_nodes);

		printf("- BVH nodes: %i\n", num_bvh_nodes);

		register_mesh(mesh, current_asset_name);

		// Copy to permanent buffers
		memcpy(global->triangle_buffer + global->triangle_buffer_size, triangles, num_triangles * sizeof(Triangle));
		global->triangle_buffer_size += num_triangles;
		memcpy(global->bvh_buffer + global->bvh_buffer_size, bvh_nodes, num_bvh_nodes * sizeof(BVHNodeFlat));
		global->bvh_buffer_size += num_bvh_nodes;
	}

	pop_arena();
	free_arena(&asset_temp_arena);

	printf("Finished loading assets. Took %.2fms.\n\n", platform_get_time_ms() - loading_assets_start_time);

	/*===============================*/
	/*      SETUP GAME INSTANCE      */
	/*===============================*/

	if (!global->vulkan_state.is_initialized) {
		global->vulkan_state = setup_renderer(vulkan_platform_data, platform_data->window_width, platform_data->window_height);
		global->prev_compute_shader_modified_time = platform_get_file_modified_time("shaders/main.spv");
		global->current_frame = 0;
		global->current_render_mode = RM_NORMAL;

		global->frame_arena = spawn_arena(mb(1));
		push_arena(&global->frame_arena);

		float focal_length = 1;
		float viewport_height = 2;
		float viewport_width = viewport_height * ((float)320 / 180);

		Vec3 viewport_u = vec3(viewport_width, 0, 0);
		Vec3 viewport_v = vec3(0, -viewport_height, 0);
		Vec3 pixel_delta_u = vec3_div(viewport_u, 320);
		Vec3 pixel_delta_v = vec3_div(viewport_v, 180);
		Vec3 viewport_upper_left = vec3_sub(vec3(0, 0, -focal_length), vec3_add(vec3_div(viewport_u, 2), vec3_div(viewport_v, 2)));
		Vec3 first_pixel_location = vec3_add(viewport_upper_left, vec3_scale(vec3_add(pixel_delta_u, pixel_delta_v), 0.5f));

		global->renderer_state = (RendererState){
			.sample_count = 16,
			.pixel_delta_u = pixel_delta_u,
			.pixel_delta_v = pixel_delta_v,
			.first_pixel_location = first_pixel_location
		};

		// Set up camera
		Mat4 camera_transform;

		Mat4 translation = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 5,
			0, 0, 0, 1
		};

		global->renderer_state.camera_transform = translation;

		game_start();
	}

	/*===============================*/
	/*        SEND DATA TO GPU       */
	/*===============================*/

	memcpy(
			(BVHNodeFlat*)global->vulkan_state.bvh_buffer_mapped,
			global->bvh_buffer,
			global->bvh_buffer_size * sizeof(BVHNodeFlat));

	FOR(i, global->triangle_buffer_size) {
		GPUTriangle gpu_triangle;
		FOR(j, 3) {
			memcpy(
					gpu_triangle.vertices[j].position.arr,
					global->triangle_buffer[i].vertices[j].position.arr,
					3 * sizeof(float));
			memcpy(
					gpu_triangle.vertices[j].normal.arr,
					global->triangle_buffer[i].vertices[j].normal.arr,
					3 * sizeof(float));
			memcpy(
					gpu_triangle.vertices[j].tex_coord.arr,
					global->triangle_buffer[i].vertices[j].tex_coord.arr,
					2 * sizeof(float));
		}

		memcpy(
				(GPUTriangle*)global->vulkan_state.triangle_buffer_mapped + i,
				&gpu_triangle,
				sizeof(GPUTriangle));
	}
}
