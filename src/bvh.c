void swap_triangles(Triangle* triangles, uint32 a, uint32 b) {
	Triangle a_temp = triangles[a];
	triangles[a] = triangles[b];
	triangles[b] = a_temp;
}

void update_bvh_bounds(Triangle* triangles, BVHNode* node) {
	float min_x = FLT_MAX;
	float min_y = FLT_MAX;
	float min_z = FLT_MAX;
	float max_x = -FLT_MAX;
	float max_y = -FLT_MAX;
	float max_z = -FLT_MAX;

	FOR_RANGE(tri_index, node->first_tri, node->first_tri + node->num_triangles - 1) {
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

void subdivide_bvh(Triangle* triangles, BVHNode* node, uint32* num_nodes) {
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
	FOR(i, node->num_triangles) {
		split_pos += triangles[node->first_tri + i].centroid.arr[axis];
	}
	split_pos /= node->num_triangles;
	//float split_pos = node->min_bounds.arr[axis] + extent.arr[axis] * 0.5f;

	// Swap tris that are on wrong side of split
	int i = node->first_tri;
	int j = i + node->num_triangles - 1;
	while (i <= j) {
		if (triangles[i].centroid.arr[axis] < split_pos) {
			i++;
		} else {
			swap_triangles(triangles, i, j--);
		}
	}

	// Create child nodes for each half
	uint num_left = i - node->first_tri;
	if (num_left == 0 || num_left == node->num_triangles) {
		node->is_leaf = true;
		return;
	}

	node->left = alloc(sizeof(BVHNode));
	*node->left = (BVHNode){
		.is_leaf = num_left <= 2,
		.first_tri = node->first_tri,
		.num_triangles = num_left
	};
	update_bvh_bounds(triangles, node->left);
	(*num_nodes)++;

	if (!node->left->is_leaf) {
		subdivide_bvh(triangles, node->left, num_nodes);
	}

	node->right = alloc(sizeof(BVHNode));
	*node->right = (BVHNode) {
		.is_leaf = node->num_triangles - num_left <= 2,
		.first_tri = i,
		.num_triangles = node->num_triangles - num_left
	};
	update_bvh_bounds(triangles, node->right);
	(*num_nodes)++;

	if (!node->right->is_leaf) {
		subdivide_bvh(triangles, node->right, num_nodes);
	}

	node->num_triangles = 0;
}

uint32 flatten_bvh(BVHNode* node, BVHNodeFlat* flat_nodes, uint32* offset) {
	BVHNodeFlat flat_node = {
		.min_bounds = node->min_bounds,
		.max_bounds = node->max_bounds
	};

	uint32 my_offset = (*offset)++;

	if (node->is_leaf) {
		flat_node.num_triangles = node->num_triangles;
		flat_node.triangles_offset = node->first_tri;
	} else {
		flat_node.num_triangles = 0;
		flatten_bvh(node->left, flat_nodes, offset);
		flat_node.right_child_offset = flatten_bvh(node->right, flat_nodes, offset);
	}

	flat_nodes[my_offset] = flat_node;
	return my_offset;
}

// Reorders the provided triangles
uint32 build_bvh(Triangle* triangles, uint32 num_triangles, BVHNodeFlat** out_bvh_nodes) {
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
	root->first_tri = 0;
	root->num_triangles = num_triangles;
	root->is_leaf = false;
	update_bvh_bounds(triangles, root);

	// Subdivide recursively
	uint32 num_nodes = 1;
	subdivide_bvh(triangles, root, &num_nodes);

	// Flatten recursively
	BVHNodeFlat* flat_nodes = alloc(num_nodes * sizeof(BVHNodeFlat));
	uint32 offset = 0;
	flatten_bvh(root, flat_nodes, &offset);
	*out_bvh_nodes = flat_nodes;

	return num_nodes;
}
