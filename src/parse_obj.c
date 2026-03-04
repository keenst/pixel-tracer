// Sets `end` to last char in float
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

Face parse_f(char* line_start) {
	// Skip 'f '
	char* c = line_start + 2;

	Face face;
	FOR(i, 3) {
		face.v.arr[i] = (int)parse_float(c, &c) - 1;
		c++;
		face.vt.arr[i] = (int)parse_float(c, &c) - 1;
		c++;
		face.vn.arr[i] = (int)parse_float(c, &c) - 1;
		c++;
	}

	return face;
}

Vec3 parse_v(char* line_start) {
	// Skip 'v '
	char* c = line_start + 2;

	Vec3 pos;
	FOR(i, 3) {
		pos.arr[i] = parse_float(c, &c);
		c++;
	}

	return pos;
}

Vec2 parse_vt(char* line_start) {
	// Skip 'vt '
	char* c = line_start + 3;

	Vec2 tex_coord;
	FOR(i, 2) {
		tex_coord.arr[i] = parse_float(c, &c);
		c++;
	}

	return tex_coord;
}

Vec3 parse_vn(char* line_start) {
	// Skip 'vn '
	char* c = line_start + 3;

	Vec3 normal;
	FOR(i, 3) {
		normal.arr[i] = parse_float(c, &c);
		c++;
	}

	return normal;
}

uint32 parse_obj(char* file_contents, Triangle** out_triangles) {
	// Count
	uint num_f = 0;
	uint num_v = 0;
	uint num_vn = 0;
	uint num_vt = 0;

	for (char* c = file_contents; *c != EOF && *c != '\0'; c++) {
		switch (*c) {
			case 'f': num_f++; break;
			case 'v': {
				switch (*(c + 1)) {
					case 'n': num_vn++; break;
					case 't': num_vt++; break;
					case ' ': num_v++; break;
				}
			} break;
		}
	}

	Face* f = alloc(num_f * sizeof(Face));
	Vec3* v = alloc(num_v * sizeof(Vec3));
	Vec3* vn = alloc(num_vn * sizeof(Vec3));
	Vec2* vt = alloc(num_vt * sizeof(Vec2));

	// Parse
	uint f_index;
	uint v_index;
	uint vn_index;
	uint vt_index;

	char* line_start = file_contents;
	while (true) {
		if (*line_start == '\0') break;

		// Parse line
		switch (*line_start) {
			case 'f': {
				f[f_index++] = parse_f(line_start);
			} break;
			case 'v': {
				switch (*(line_start + 1)) {
					case ' ': {
						v[v_index++] = parse_v(line_start);
					} break;
					case 't': {
						vt[vt_index++] = parse_vt(line_start);
					} break;
					case 'n': {
						vn[vn_index++] = parse_vn(line_start);
					} break;
				}
			} break;
		}

		// Go to next line
		while (*line_start != '\n') {
			line_start++;
		}
		line_start++;
	}

	Triangle* triangles = alloc(num_f * sizeof(Triangle));
	FOR(face_index, num_f) {
		Triangle* triangle = &triangles[face_index];
		Face face = f[face_index];

		FOR(vert_index, 3) {
			triangle->vertices[vert_index].position = v[face.v.arr[vert_index]];
			triangle->vertices[vert_index].normal = vn[face.vn.arr[vert_index]];
			triangle->vertices[vert_index].tex_coord = vt[face.vt.arr[vert_index]];
		}
	}

	*out_triangles = triangles;
	return num_f;
}
