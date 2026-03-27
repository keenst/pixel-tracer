void swap_triangles(Triangle* triangles, uint32 a, uint32 b) {
	Triangle a_temp = triangles[a];
	triangles[a] = triangles[b];
	triangles[b] = a_temp;
}

void update_mesh_bvh_bounds(Triangle* triangles, BVHNode* node) {
	float min_x = FLT_MAX;
	float min_y = FLT_MAX;
	float min_z = FLT_MAX;
	float max_x = -FLT_MAX;
	float max_y = -FLT_MAX;
	float max_z = -FLT_MAX;

	FOR_RANGE(tri_index, node->first_primitive, node->first_primitive + node->num_primitives - 1) {
		FOR(vertex_index, 3) {
			Vertex vertex = triangles[tri_index].vertices[vertex_index];
			if (vertex.position.x < min_x) min_x = vertex.position.x;
			if (vertex.position.x > max_x) max_x = vertex.position.x;
			if (vertex.position.y < min_y) min_y = vertex.position.y;
			if (vertex.position.y > max_y) max_y = vertex.position.y;
			if (vertex.position.z < min_z) min_z = vertex.position.z;
			if (vertex.position.z > max_z) max_z = vertex.position.z;
		}
	}

	node->min_bounds = vec3(min_x, min_y, min_z);
	node->max_bounds = vec3(max_x, max_y, max_z);
}

void update_scene_bvh_bounds(SceneBVHObject* objects, uint* object_indices, BVHNode* node) {
	float min_x = FLT_MAX;
	float min_y = FLT_MAX;
	float min_z = FLT_MAX;
	float max_x = -FLT_MAX;
	float max_y = -FLT_MAX;
	float max_z = -FLT_MAX;

	FOR_RANGE(object_index, node->first_primitive, node->first_primitive + node->num_primitives - 1) {
		SceneBVHObject object = objects[object_indices[object_index]];

		Vec3 corners[8];
		Vec3 bounds[2] = { object.mesh->min_bounds, object.mesh->max_bounds };
		int index = 0;
		FOR(i, 2) {
			FOR(j, 2) {
				FOR(k, 2) {
					corners[index++] = vec3(bounds[i].x, bounds[j].y, bounds[k].z);
				}
			}
		}
	
		FOR(corner_index, 8) {
			Vec4 min_bounds = vec4_mul_mat4(vec3_xyz1(corners[corner_index]), object.transform);
			Vec4 max_bounds = vec4_mul_mat4(vec3_xyz1(corners[corner_index]), object.transform);

			if (min_bounds.x < min_x) min_x = min_bounds.x;
			if (min_bounds.x > max_x) max_x = min_bounds.x;
			if (min_bounds.y < min_y) min_y = min_bounds.y;
			if (min_bounds.y > max_y) max_y = min_bounds.y;
			if (min_bounds.z < min_z) min_z = min_bounds.z;
			if (min_bounds.z > max_z) max_z = min_bounds.z;

			if (max_bounds.x < min_x) min_x = max_bounds.x;
			if (max_bounds.x > max_x) max_x = max_bounds.x;
			if (max_bounds.y < min_y) min_y = max_bounds.y;
			if (max_bounds.y > max_y) max_y = max_bounds.y;
			if (max_bounds.z < min_z) min_z = max_bounds.z;
			if (max_bounds.z > max_z) max_z = max_bounds.z;
		}
	}

	node->min_bounds = vec3(min_x, min_y, min_z);
	node->max_bounds = vec3(max_x, max_y, max_z);
}

void subdivide_mesh_bvh(Triangle* triangles, BVHNode* node, uint32* num_nodes) {
	// Find longest axis
	Vec3 extent = vec3_sub(node->max_bounds, node->min_bounds);

	uint axis = 0;
	if (extent.y > extent.x) {
		axis = 1;
	} else if (extent.z > extent.arr[axis]) {
		axis = 2;
	}

	// Split by midpoint of centroids
	float split_pos = 0;
	FOR(i, node->num_primitives) {
		split_pos += triangles[node->first_primitive + i].centroid.arr[axis];
	}
	split_pos /= node->num_primitives;

	// Swap tris that are on wrong side of split
	int i = node->first_primitive;
	int j = i + node->num_primitives - 1;
	while (i <= j) {
		if (triangles[i].centroid.arr[axis] < split_pos) {
			i++;
		} else {
			swap_triangles(triangles, i, j--);
		}
	}

	// Create child nodes for each half
	uint num_left = i - node->first_primitive;
	if (num_left == 0 || num_left == node->num_primitives) {
		node->is_leaf = true;
		return;
	}

	node->left = alloc(sizeof(BVHNode));
	*node->left = (BVHNode){
		.is_leaf = num_left <= 2,
		.first_primitive = node->first_primitive,
		.num_primitives = num_left
	};
	update_mesh_bvh_bounds(triangles, node->left);
	(*num_nodes)++;

	if (!node->left->is_leaf) {
		subdivide_mesh_bvh(triangles, node->left, num_nodes);
	}

	node->right = alloc(sizeof(BVHNode));
	*node->right = (BVHNode){
		.is_leaf = node->num_primitives - num_left <= 2,
		.first_primitive = i,
		.num_primitives = node->num_primitives - num_left
	};
	update_mesh_bvh_bounds(triangles, node->right);
	(*num_nodes)++;

	if (!node->right->is_leaf) {
		subdivide_mesh_bvh(triangles, node->right, num_nodes);
	}

	node->num_primitives = 0;
}

void subdivide_scene_bvh(SceneBVHObject* objects, uint* object_indices, BVHNode* node, uint32* num_nodes) {
	// Find longest axis
	Vec3 extent = vec3_sub(node->max_bounds, node->min_bounds);

	uint axis = 0;
	if (extent.y > extent.x) {
		axis = 1;
	} else if (extent.z > extent.arr[axis]) {
		axis = 2;
	}

	// Split by midpoint of centroids
	float split_pos = 0;
	FOR(i, node->num_primitives) {
		SceneBVHObject object = objects[object_indices[node->first_primitive + i]];
		Vec4 world_centroid = vec4_mul_mat4(vec3_xyz1(object.mesh->centroid), object.transform);
		split_pos += world_centroid.arr[axis];
	}
	split_pos /= node->num_primitives;

	// Swap tris that are on wrong side of split
	int i = node->first_primitive;
	int j = i + node->num_primitives - 1;
	while (i <= j) {
		SceneBVHObject object = objects[object_indices[i]];
		Vec4 world_centroid = vec4_mul_mat4(vec3_xyz1(object.mesh->centroid), object.transform);

		if (world_centroid.arr[axis] < split_pos) {
			i++;
		} else {
			uint temp = object_indices[i];
			object_indices[i] = object_indices[j];
			object_indices[j] = temp;
			j--;
		}
	}

	// Create child nodes for each half
	uint num_left = i - node->first_primitive;
	if (num_left == 0 || num_left == node->num_primitives) {
		node->is_leaf = true;
		return;
	}

	node->left = alloc(sizeof(BVHNode));
	*node->left = (BVHNode){
		.is_leaf = num_left <= 1,
		.first_primitive = node->first_primitive,
		.num_primitives = num_left
	};
	update_scene_bvh_bounds(objects, object_indices, node->left);
	(*num_nodes)++;

	if (!node->left->is_leaf) {
		subdivide_scene_bvh(objects, object_indices, node->left, num_nodes);
	}

	node->right = alloc(sizeof(BVHNode));
	*node->right = (BVHNode){
		.is_leaf = node->num_primitives - num_left <= 1,
		.first_primitive = i,
		.num_primitives = node->num_primitives - num_left
	};
	update_scene_bvh_bounds(objects, object_indices, node->right);
	(*num_nodes)++;

	if (!node->right->is_leaf) {
		subdivide_scene_bvh(objects, object_indices, node->right, num_nodes);
	}

	node->num_primitives = 0;
}

uint32 flatten_bvh(BVHNode* node, BVHNodeFlat* flat_nodes, uint32* offset) {
	BVHNodeFlat flat_node = {
		.min_bounds = node->min_bounds,
		.max_bounds = node->max_bounds
	};

	uint32 my_offset = (*offset)++;

	if (node->is_leaf) {
		flat_node.num_primitives = node->num_primitives;
		flat_node.primitives_offset = node->first_primitive;
	} else {
		flat_node.num_primitives = 0;
		flatten_bvh(node->left, flat_nodes, offset);
		flat_node.right_child_offset = flatten_bvh(node->right, flat_nodes, offset);
	}

	flat_nodes[my_offset] = flat_node;
	return my_offset;
}

// Reorders the provided triangles
uint32 build_mesh_bvh(Triangle* triangles, uint num_triangles, BVHNodeFlat** out_bvh_nodes) {
	// Compute centroids
	FOR(i, num_triangles) {
		Vec3 centroid = {};
		centroid = vec3_add(
				triangles[i].vertices[0].position,
				vec3_add(triangles[i].vertices[1].position, triangles[i].vertices[2].position));

		centroid = vec3_scale(centroid, 1.0f / 3);
		triangles[i].centroid = centroid;
	}

	// Create root node
	BVHNode* root = alloc(sizeof(BVHNode));
	root->first_primitive = 0;
	root->num_primitives = num_triangles;
	root->is_leaf = false;
	update_mesh_bvh_bounds(triangles, root);

	// Subdivide recursively
	uint32 num_nodes = 1;
	subdivide_mesh_bvh(triangles, root, &num_nodes);

	// Flatten recursively
	BVHNodeFlat* flat_nodes = alloc(num_nodes * sizeof(BVHNodeFlat));
	uint32 offset = 0;
	flatten_bvh(root, flat_nodes, &offset);
	*out_bvh_nodes = flat_nodes;

	return num_nodes;
}

// Doesn't reorder the provided objects, instead creates an array of ordered indices.
uint32 build_scene_bvh(
		SceneBVHObject* objects,
		uint num_objects,
		uint** out_object_indices,
		BVHNodeFlat** out_bvh_nodes) {
	// Generate indices
	*out_object_indices = alloc(num_objects * sizeof(uint));
	FOR(i, num_objects) {
		(*out_object_indices)[i] = i;
	}

	// Create root node
	BVHNode* root = alloc(sizeof(BVHNode));
	root->first_primitive = 0;
	root->num_primitives = num_objects;
	root->is_leaf = false;
	update_scene_bvh_bounds(objects, *out_object_indices, root);

	// Subdivide recursively
	uint32 num_nodes = 1;
	subdivide_scene_bvh(objects, *out_object_indices, root, &num_nodes);

	// Flatten recursively
	BVHNodeFlat* flat_nodes = alloc(num_nodes * sizeof(BVHNodeFlat));
	uint32 offset = 0;
	flatten_bvh(root, flat_nodes, &offset);
	*out_bvh_nodes = flat_nodes;

	return num_nodes;
}
