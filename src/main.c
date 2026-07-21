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

Texture load_texture_from_file(VulkanState* vulkan_state, char* path) {
	int width, height, component_count;
	uint8* data = stbi_load(path, &width, &height, &component_count, 0);

	VkImageCreateInfo image_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent.width = width,
		.extent.height = height,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = 1,
		.format = VK_FORMAT_R8G8B8A8_UNORM,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.samples = VK_SAMPLE_COUNT_1_BIT
	};

	VkImage image;
	VK_ASSERT(vkCreateImage(
			vulkan_state->device,
			&image_create_info,
			NULL,
			&image));

	VkMemoryRequirements texture_memory_requirements;
	vkGetImageMemoryRequirements(
			vulkan_state->device,
			image,
			&texture_memory_requirements);

	VkMemoryAllocateInfo texture_memory_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = texture_memory_requirements.size,
		.memoryTypeIndex = find_vulkan_memory_type(vulkan_state, texture_memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};

	VkDeviceMemory image_memory;
	VK_ASSERT(vkAllocateMemory(
			vulkan_state->device,
			&texture_memory_allocate_info,
			NULL,
			&image_memory));

	vkBindImageMemory(
			vulkan_state->device, 
			image,
			image_memory,
			0);

	VkImageViewCreateInfo image_view_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_UNORM,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1
	};

	VkImageView image_view;
	VK_ASSERT(vkCreateImageView(
			vulkan_state->device,
			&image_view_create_info,
			NULL,
			&image_view));

	// Copy the contents
	transition_image_layout(
			vulkan_state,
			image,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkBuffer buffer;
	VkDeviceMemory buffer_memory;
	create_buffer(
			vulkan_state,
			(VkDeviceSize)(width * height * component_count),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&buffer,
			&buffer_memory);

	void* buffer_mapped;
	vkMapMemory(
			vulkan_state->device,
			buffer_memory,
			0, sizeof(RendererState),
			0, &buffer_mapped);

	// TODO(leo): Unmap and destroy buffers created here.

	memcpy(buffer_mapped, data, width * height * component_count);

	VkCommandBuffer temp_command_buffer = begin_temp_command_buffer(vulkan_state);

	VkBufferImageCopy region = {
		.bufferImageHeight = height,
		.bufferRowLength = width,
		.imageExtent.width = width,
		.imageExtent.height = height,
		.imageExtent.depth = 1,
		.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.imageSubresource.layerCount = 1
	};

	vkCmdCopyBufferToImage(
			temp_command_buffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region);

	end_temp_command_buffer(vulkan_state, temp_command_buffer);

	transition_image_layout(
			vulkan_state,
			image,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Finalize
	stbi_image_free(data);

	Texture texture = {
		.image = image,
		.image_view = image_view
	};

	return texture;
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

TextureID get_texture_id(char* name) {
	FOR(texture_index, global->num_textures) {
		if (strcmp(global->texture_names[texture_index], name) != 0) {
			continue;
		}

		return texture_index;
	}

	TRAP("Texture \"%s\" not found", name);
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

	SceneBVHObject* scene_bvh_objects = alloc(global->num_objects * sizeof(SceneBVHObject));

	// Create `RenderObject`s
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
			render_object->is_light = true;

			scene_bvh_objects[light_index] = (SceneBVHObject){
				.mesh = &global->meshes[object->mesh_id],
				.transform = transform
			};

			light_index++;
		}
		else
		{
			render_object = &render_objects[global->num_objects - non_light_index - 1];

			scene_bvh_objects[global->num_objects - non_light_index - 1] = (SceneBVHObject){
				.mesh = &global->meshes[object->mesh_id],
				.transform = transform
			};

			non_light_index++;
		}

		render_object->transform = transform;
		render_object->inv_transform = inv_transform;

		render_object->bvh_root_offset = global->meshes[object->mesh_id].root_node_offset;
		render_object->triangle_offset = global->meshes[object->mesh_id].triangle_offset;
		render_object->num_triangles = global->meshes[object->mesh_id].num_triangles;

		render_object->material = object->material;
	}

	global->renderer_state.num_lights = light_index;

	memcpy(
			global->vulkan_state.object_buffers[global->current_frame].mapped,
			render_objects,
			global->num_objects * sizeof(RenderObject));

	// Build scene BVH tree
	uint* object_indices;
	BVHNodeFlat* scene_bvh_nodes;
	uint32 num_nodes = build_scene_bvh(scene_bvh_objects, global->num_objects, &object_indices, &scene_bvh_nodes);

	memcpy(
			global->vulkan_state.object_index_buffers[global->current_frame].mapped,
			object_indices,
			global->num_objects * sizeof(uint));

	memcpy(
			global->vulkan_state.scene_bvh_buffers[global->current_frame].mapped,
			scene_bvh_nodes,
			num_nodes * sizeof(BVHNodeFlat));

	/*======================================*/
	/*               COMPUTE 				*/
	/*======================================*/

	memcpy(vulkan_state->renderer_state_buffers[current_frame].mapped, &global->renderer_state, sizeof(RendererState));

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

		if (global->renderer_state.progressive) {
			global->renderer_state.num_frames = 1;
			global->renderer_state.sample_count = 128;
		} else {
			global->renderer_state.sample_count = 4;
		}
	}

	// Update
	float delta_time = global->platform_data->delta_time;
	global->total_time += delta_time;

	if (!global->renderer_state.progressive)
	{
		global->scaled_time += delta_time;

		// Movement
		static float yaw = 0;
		static float pitch = 0;

		if (inputs.right_mouse) {
			const float mouse_sensitivity = 4.0f;
			yaw += inputs.mouse_delta_x * mouse_sensitivity;
			pitch += inputs.mouse_delta_y * mouse_sensitivity;
			global->platform_data->mouse_locked = true;
		} else {
			global->platform_data->mouse_locked = false;
		}

		static Vec3 position = { 0, 0, 5 };

		Vec3 move_direction = {};

		if (inputs.w) {
			move_direction.z += 1;
		}

		if (inputs.s) {
			move_direction.z -= 1;
		}

		if (inputs.a) {
			move_direction.x -= 1;
		}

		if (inputs.d) {
			move_direction.x += 1;
		}

		if (inputs.space) {
			move_direction.y += 1;
		}

		if (inputs.ctrl) {
			move_direction.y -= 1;
		}

		move_direction = vec3_normalized(move_direction);

		float camera_speed = 6.0f;
		if (inputs.shift) {
			camera_speed = 12.0f;
		}

		Mat4 y_rot = mat4_rot_y(yaw);
		Mat4 x_rot = mat4_rot_x(pitch);
		Mat4 rotation = mat4_mul(y_rot, x_rot);

		global->renderer_state.camera_transform = rotation;

		Vec3 camera_forward = vec3(
				global->renderer_state.camera_transform.arr[0][2],
				global->renderer_state.camera_transform.arr[1][2],
				global->renderer_state.camera_transform.arr[2][2]);

		Vec3 up = vec3(0, 1, 0);
		Vec3 right = vec3_normalized(vec3_cross(up, camera_forward));
		Vec3 forward = vec3_normalized(vec3_cross(up, right));

		move_direction = vec3_add(
				vec3(0, move_direction.y, 0),
				vec3_add(
					vec3_scale(right, move_direction.x),
					vec3_scale(forward, move_direction.z)));

		position = vec3_add(position, vec3_scale(move_direction, camera_speed * delta_time));
		global->renderer_state.camera_transform.arr[0][3] = position.x;
		global->renderer_state.camera_transform.arr[1][3] = position.y;
		global->renderer_state.camera_transform.arr[2][3] = position.z;

		Object* lit_monkey = get_object("lit_monkey");
		lit_monkey->position = vec3(-5, 0, 0);
		lit_monkey->position.y = sinf(global->scaled_time);
		lit_monkey->orientation.y += delta_time;

		Object* green_light = get_object("green_light");
		green_light->position = vec3(cosf(global->scaled_time) * 2, 0.5f, -5 + sinf(global->scaled_time) * 2);

		Object* magenta_monkey = get_object("magenta_monkey");
		magenta_monkey->position = vec3(sinf(global->scaled_time * 1.5f), 0.5f, 6);
		magenta_monkey->scale = vec3_scale(vec3(1, 1, 1), cosf(global->scaled_time * 3) / 4 + 0.5f);
		magenta_monkey->orientation.z += delta_time;

		/*
		Object* merry_go_round = get_object("merry_go_round");
		merry_go_round->position = vec3(cosf(global->scaled_time) * 20, 2, sinf(global->scaled_time) * 20);
		merry_go_round->orientation.x = pi / 2;
		merry_go_round->orientation.y = (sinf(global->scaled_time) / 2 + 0.5f) * pi;
		*/
	}

	// Draw
	global->renderer_state.time = global->total_time;
	while (global->renderer_state.time > 1)
	{
		global->renderer_state.time -= 1;
	}

	draw_frame(&global->vulkan_state, global->current_frame, global->renderer_state);
	global->current_frame = (global->current_frame + 1) % 2;

	global->prev_inputs = inputs;

	clear_arena(&global->frame_arena);
}

void game_start() {
	// Cornell box
	global->current_scene_id = 1;

	/*
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
	*/

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
	lit_plane->material.roughness = 1;
	lit_plane->position = vec3(0, -2 - 1e-5f, 0);
	lit_plane->scale = vec3(10, 1, 10);

	Object* red_cube = register_object("red_cube");
	red_cube->mesh_id = get_mesh_id("cube_cool.obj");
	red_cube->material.color = vec3(1, 0, 0);
	red_cube->position = vec3(3, -2, 0);
	red_cube->scale = vec3(0.5f, 5, 1);

	Object* green_sphere = register_object("green_sphere");
	green_sphere->mesh_id = get_mesh_id("sphere.obj");
	green_sphere->material.color = vec3(0.4, 1, 0.2);
	green_sphere->position = vec3(3, 0, 2);

	Object* blue_sphere = register_object("blue_sphere");
	blue_sphere->mesh_id = get_mesh_id("sphere.obj");
	blue_sphere->material.color = vec3(1, 1, 1);
	blue_sphere->position = vec3(-5, 2, -5);
	blue_sphere->scale = vec3(2, 2, 2);
	blue_sphere->material.texture_id = get_texture_id("test.png");

	Object* green_light = register_object("green_light");
	green_light->mesh_id = get_mesh_id("sphere.obj");
	green_light->material.color = vec3(0, 6, 0);

	Object* magenta_monkey = register_object("magenta_monkey");
	magenta_monkey->mesh_id = get_mesh_id("suzanne.obj");
	magenta_monkey->material.roughness = 0.9f;
	magenta_monkey->material.color = vec3(0, 1, 1);

	/*
	Object* merry_go_round = register_object("merry_go_round");
	merry_go_round->mesh_id = get_mesh_id("plane.obj");
	merry_go_round->material.color = vec3(1.5, 1.5, 1.5);
	merry_go_round->scale = vec3(5, 1, 5);
	*/
}

__declspec(dllexport)
void game_init(
		PlatformData* platform_data,
		VulkanPlatformData vulkan_platform_data,
		void* memory,
		uint memory_size) {
	global = memory;
	global->base_arena = (Arena){
		.base = memory,
		.head = (char*)memory + sizeof(GlobalMemory),
		.size = memory_size
	};
	push_arena(&global->base_arena);

	global->platform_data = platform_data;

	load_vulkan(vulkan_platform_data);

	/*================================*/
	/*      SET UP GAME INSTANCE      */
	/*================================*/

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
			.sample_count = 4,
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
	}

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
	char** asset_names = platform_read_dir("assets\\*.obj");
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

		mesh.num_triangles = num_triangles;

		// Generate centroid and bounds
		float min_x = FLT_MAX;
		float min_y = FLT_MAX;
		float min_z = FLT_MAX;
		float max_x = -FLT_MAX;
		float max_y = -FLT_MAX;
		float max_z = -FLT_MAX;

		Vec3 centroid = {};

		FOR(tri_index, num_triangles) {
			Triangle triangle = triangles[tri_index];
			FOR(vert_index, 3) {
				Vertex vertex = triangle.vertices[vert_index];

				if (vertex.position.x < min_x) min_x = vertex.position.x;
				if (vertex.position.x > max_x) max_x = vertex.position.x;
				if (vertex.position.y < min_y) min_y = vertex.position.y;
				if (vertex.position.y > max_y) max_y = vertex.position.y;
				if (vertex.position.z < min_z) min_z = vertex.position.z;
				if (vertex.position.z > max_z) max_z = vertex.position.z;
			}
		}

		mesh.min_bounds = vec3(min_x, min_y, min_z);
		mesh.max_bounds = vec3(max_x, max_y, max_z);
		mesh.centroid = vec3_scale(vec3_add(mesh.min_bounds, mesh.max_bounds), 0.5f);

		// Build BVH and reorder triangles
		BVHNodeFlat* bvh_nodes;
		uint32 num_bvh_nodes = build_mesh_bvh(triangles, num_triangles, &bvh_nodes);

		printf("- BVH nodes: %i\n", num_bvh_nodes);

		register_mesh(mesh, current_asset_name);

		// Copy to permanent buffers
		memcpy(global->triangle_buffer + global->triangle_buffer_size, triangles, num_triangles * sizeof(Triangle));
		global->triangle_buffer_size += num_triangles;
		memcpy(global->bvh_buffer + global->bvh_buffer_size, bvh_nodes, num_bvh_nodes * sizeof(BVHNodeFlat));
		global->bvh_buffer_size += num_bvh_nodes;
	}

	// Load textures
	global->num_textures = 1;
	char** texture_names = platform_read_dir("assets\\*.png");
	for (int i = 0;; i++) {
		char* current_texture_name = texture_names[i];
		if (current_texture_name == NULL) {
			break;
		}

		printf("\"%s\":\n", current_texture_name);

		memcpy(path_buffer + 7, current_texture_name, strlen(current_texture_name) + 1);
	
		Texture texture = load_texture_from_file(&global->vulkan_state, path_buffer);

		global->textures[global->num_textures] = texture;
		ASSERT(strlen(current_texture_name) <= NAME_LEN);
		snprintf(global->texture_names[global->num_textures], NAME_LEN, current_texture_name);
		global->num_textures++;
	}

	pop_arena();
	free_arena(&asset_temp_arena);

	printf("Finished loading assets. Took %.2fms.\n\n", platform_get_time_ms() - loading_assets_start_time);





	game_start();

	/*===============================*/
	/*        SEND DATA TO GPU       */
	/*===============================*/

	// Buffers

	memcpy(
		(BVHNodeFlat*)global->vulkan_state.mesh_bvh_buffer.mapped,
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
			(GPUTriangle*)global->vulkan_state.triangle_buffer.mapped + i,
			&gpu_triangle,
			sizeof(GPUTriangle));
	}

	// Textures

	VkDescriptorImageInfo image_infos[MAX_TEXTURE_COUNT];
	for (int texture_index = 1; texture_index < global->num_textures; texture_index++) {
		image_infos[texture_index - 1] = (VkDescriptorImageInfo){
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.imageView = global->textures[texture_index].image_view,
		};
	}

	FOR(frame_in_flight, MAX_FRAMES_IN_FLIGHT) {
		VkWriteDescriptorSet texture_write_descriptor = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = global->vulkan_state.compute_descriptor_sets[frame_in_flight],
			.dstBinding = 7,
			.dstArrayElement = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.descriptorCount = global->num_textures - 1,
			.pImageInfo = image_infos
		};

		vkUpdateDescriptorSets(global->vulkan_state.device, 1, &texture_write_descriptor, 0, NULL);
	}
}
