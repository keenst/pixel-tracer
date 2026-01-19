#include "parse_obj.h"

float parse_float(char* string, char** end) {
	float parsed_value = 0;

	bool passed_decimal_point = false;
	uint32 decimals = 0;

	char* c = string;

	bool is_negative = false;
	if (*c == '-') {
		is_negative = true;
		c++;
	}

	uint32 index = 0;
	while ((*c >= '0' && *c <= '9') || *c == '.') {
		if (*c == '.') {
			c++;
			passed_decimal_point = true;
			continue;
		}

		if (passed_decimal_point) {
			decimals++;
		}

		uint32 digit = *c - '0';
		parsed_value *= 10;
		parsed_value += (float)digit;
		c++;
	}

	*end = c;

	if (is_negative) {
		parsed_value *= -1;
	}

	return parsed_value / powf(10, decimals);
}

uint32 parse_obj(char* file_contents, Triangle** out_triangles) {
	// Count vertices
	uint32 face_count = 0;
	uint32 indexed_vertex_count = 0;
	for (char* c = file_contents; *c != EOF && *c != '\0'; c++) {
		switch (*c) {
			case 'f': face_count++; break;
			case 'v': indexed_vertex_count++; break;
		}
	}

	// Parse vertices
	Float3* indexed_vertices = malloc(indexed_vertex_count * sizeof(Float3));
	UInt3* faces = malloc(face_count * sizeof(UInt3));

	char* c = file_contents;
	uint32 indexed_vertex_index = 0;
	uint32 face_index = 0;
	while (true) {
		switch (*c) {
			case 'v': { // Vertex
				Float3 vertex;
				char* next_char;

				c += 2; // Skip v and first space
				FOR(i, 3) {
					vertex.array[i] = parse_float(c, &next_char);
					c = next_char + 1; // Skip space / newline
				}

				indexed_vertices[indexed_vertex_index++] = vertex;
			} break;
			case 'f': {
				UInt3 face;
				char* next_char;

				c += 2; // Skip f and first space
				FOR(i, 3) {
					face.array[i] = (uint32)parse_float(c, &next_char) - 1;
					c = next_char + 1; // Skip space / newline
				}

				faces[face_index++] = face;
			} break;
			case '\r': case '\n': c++; continue;
			default: { // Skip to next line
				while (*c != '\n' && *c != '\r') {
					if (*c == EOF) {
						goto done_parsing;
					}

					c++;
				}
				c++;
			} break;
		}
	}

	done_parsing:;

	Triangle* triangles = malloc(face_count * sizeof(Triangle));
	FOR(tri_index, face_count) {
		FOR(vert_index, 3) {
			triangles[tri_index].vertices[vert_index] = indexed_vertices[faces[tri_index].array[vert_index]];
		}
	}

	*out_triangles = triangles;
	return face_count;
}
