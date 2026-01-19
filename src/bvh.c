#include "bvh.h"

void swap_triangles(BVHTriangle* triangles, uint32 a, uint32 b) {
	BVHTriangle a_temp = triangles[a];
	triangles[a] = triangles[b];
	triangles[b] = a_temp;
}

void update_bvh_bounds(BVHTriangle* triangles, BVHNode* node) {
	float min_x = FLT_MAX;
	float min_y = FLT_MAX;
	float min_z = FLT_MAX;
	float max_x = -FLT_MAX;
	float max_y = -FLT_MAX;
	float max_z = -FLT_MAX;

	FOR_RANGE(tri_index, node->first_tri, node->first_tri + node->num_triangles - 1) {
		FOR(vertex_index, 3) {
			Float3 vertex = triangles[tri_index].vertices[vertex_index];
			if (vertex.x < min_x) min_x = vertex.x;
			if (vertex.x > max_x) max_x = vertex.x;
			if (vertex.y < min_y) min_y = vertex.y;
			if (vertex.y > max_y) max_y = vertex.y;
			if (vertex.z < min_z) min_z = vertex.z;
			if (vertex.z > max_z) max_z = vertex.z;
		}
	}

	node->min_bounds = float3(min_x, min_y, min_z);
	node->max_bounds = float3(max_x, max_y, max_z);
}

void subdivide_bvh(BVHTriangle* triangles, BVHNode* node, uint32* num_nodes) {
	// Find longest axis
	Float3 extent = float3_sub(node->max_bounds, node->min_bounds);

	uint32 axis = 0;
	if (extent.y > extent.x) {
		axis = 1;
	} else if (extent.z > extent.array[axis]) {
		axis = 2;
	}

	// Split by midpoint of centroids
	float split_pos = 0;
	FOR(i, node->num_triangles) {
		split_pos += triangles[node->first_tri + i].centroid.array[axis];
	}
	split_pos /= node->num_triangles;
	//float split_pos = node->min_bounds.array[axis] + extent.array[axis] * 0.5f;

	// Swap tris that are on wrong side of split
	int32 i = node->first_tri;
	int32 j = i + node->num_triangles - 1;
	while (i <= j) {
		if (triangles[i].centroid.array[axis] < split_pos) {
			i++;
		} else {
			swap_triangles(triangles, i, j--);
		}
	}

	// Create child nodes for each half
	uint32 num_left = i - node->first_tri;
	if (num_left == 0 || num_left == node->num_triangles) {
		return;
	}

	node->left = malloc(sizeof(BVHNode));
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

	node->right = malloc(sizeof(BVHNode));
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

uint32 build_bvh(Triangle* mesh_triangles, uint32 num_triangles, BVHNodeFlat** out_bvh_nodes) {
	// Convert mesh triangles to bvh triangles
	BVHTriangle* triangles = malloc(num_triangles * sizeof(BVHTriangle));
	FOR(i, num_triangles) {
		memcpy(triangles[i].vertices, mesh_triangles[i].vertices, 3 * sizeof(Float3));
		Float3 centroid = {};
		centroid = float3_add(triangles[i].vertices[0], float3_add(triangles[i].vertices[1], triangles[i].vertices[2]));
		centroid = float3_scale(centroid, 1.0f / 3);
		triangles[i].centroid = centroid;
	}

	// Create root node
	BVHNode* root = malloc(sizeof(BVHNode));
	root->first_tri = 0;
	root->num_triangles = num_triangles;
	root->is_leaf = false;
	update_bvh_bounds(triangles, root);

	// Subdivide recursively
	uint32 num_nodes = 1;
	subdivide_bvh(triangles, root, &num_nodes);

	// Flatten recursively
	BVHNodeFlat* flat_nodes = malloc(num_nodes * sizeof(BVHNodeFlat));
	uint32 offset = 0;
	flatten_bvh(root, flat_nodes, &offset);
	*out_bvh_nodes = flat_nodes;

	return num_nodes;
}
