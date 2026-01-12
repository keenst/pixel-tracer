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

uint32 parse_obj(char* file_contents, float** out_vertices) {
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
	Uint3* faces = malloc(face_count * sizeof(Uint3));

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
				Uint3 face;
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

	uint32 vertex_count = face_count * 3;
	float* vertices = malloc(vertex_count * 4 * sizeof(float));

	uint32 vertex_index = 0;
	FOR(face_index, face_count) {
		FOR(i, 3) {
			memcpy(&vertices[vertex_index++ * 4], &indexed_vertices[faces[face_index].array[i]], 3 * sizeof(float));
			//vertices[vertex_index++] = indexed_vertices[faces[face_index].array[i]];
		}
	}

	*out_vertices = vertices;
	return vertex_count;
}
