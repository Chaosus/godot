/*************************************************************************/
/*  visual_particles_nodes.cpp                                           */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "visual_particles_nodes.h"

// VisualShaderNodeEmission

void VisualShaderNodeEmission::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_shape_type", "type"), &VisualShaderNodeEmission::set_shape_type);
	ClassDB::bind_method(D_METHOD("get_shape_type"), &VisualShaderNodeEmission::get_shape_type);
	ClassDB::bind_method(D_METHOD("set_initial_velocity_type", "type"), &VisualShaderNodeEmission::set_initial_velocity_type);
	ClassDB::bind_method(D_METHOD("get_initial_velocity_type"), &VisualShaderNodeEmission::get_initial_velocity_type);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "shape_type", PROPERTY_HINT_ENUM, "Ring"), "set_shape_type", "get_shape_type");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "initial_velocity_type", PROPERTY_HINT_ENUM, "Cone,Radial"), "set_initial_velocity_type", "get_initial_velocity_type");

	BIND_ENUM_CONSTANT(SHAPE_TYPE_RING);
	BIND_ENUM_CONSTANT(SHAPE_TYPE_MAX);

	BIND_ENUM_CONSTANT(VELOCITY_TYPE_CONE);
	BIND_ENUM_CONSTANT(VELOCITY_TYPE_RADIAL);
	BIND_ENUM_CONSTANT(VELOCITY_TYPE_MAX);
}

String VisualShaderNodeEmission::get_caption() const {
	return "Emission";
}

int VisualShaderNodeEmission::get_input_port_count() const {
	switch (shape_type) {
		case SHAPE_TYPE_RING:
			return 5;
	}
	return 4;
}

bool VisualShaderNodeEmission::is_generate_input_var(int p_port) const {
	return false;
}

Vector<StringName> VisualShaderNodeEmission::get_editable_properties() const {
	Vector<StringName> props;
	props.push_back("shape_type");
	if (shape_type == SHAPE_TYPE_RING) {
		props.push_back("initial_velocity_type");
	}
	return props;
}

VisualShaderNodeEmission::PortType VisualShaderNodeEmission::get_input_port_type(int p_port) const {
	switch (p_port) {
		case 0:
			return PORT_TYPE_BOOLEAN; // condition
		case 1:
			return PORT_TYPE_VECTOR; // emitter offset
		case 2:
			return PORT_TYPE_VECTOR; // particle location
	}
	switch (shape_type) {
		case SHAPE_TYPE_RING:
			switch (p_port) {
				case 3:
					return PORT_TYPE_SCALAR; // radius
			}
			break;
	}
	if (p_port == get_input_port_count() - 1) {
		return PORT_TYPE_STAGE;
	}
	return PORT_TYPE_SCALAR;
}

String VisualShaderNodeEmission::get_input_port_name(int p_port) const {
	switch (p_port) {
		case 0:
			return "condition";
		case 1:
			return "emitter_offset";
		case 2:
			return "location";
	}
	switch (shape_type) {
		case SHAPE_TYPE_RING:
			switch (p_port) {
				case 3:
					return "radius";
			}
			break;
	}
	if (p_port == get_input_port_count() - 1) {
		return "prev_stage";
	}
	return String();
}

int VisualShaderNodeEmission::get_output_port_count() const {
	return 1;
}

VisualShaderNodeOutput::PortType VisualShaderNodeEmission::get_output_port_type(int p_port) const {
	return PORT_TYPE_STAGE;
}

String VisualShaderNodeEmission::get_output_port_name(int p_port) const {
	return "next_stage";
}

String VisualShaderNodeEmission::generate_global_per_node(Shader::Mode p_mode, VisualShader::Type p_type, int p_id) const {
	String code;

	code += "float rand_from_seed(inout uint seed) {\n";
	code += "\tint k;\n";
	code += "\tint s = int(seed);\n";
	code += "\tif (s == 0)\n";
	code += "\ts = 305420679;\n";
	code += "\tk = s / 127773;\n";
	code += "\ts = 16807 * (s - k * 127773) - 2836 * k;\n";
	code += "\tif (s < 0)\n";
	code += "\t\ts += 2147483647;\n";
	code += "\tseed = uint(s);\n";
	code += "\treturn float(seed % uint(65536)) / 65535.0;\n";
	code += "}\n\n";

	code += "float rand_from_seed_m1_p1(inout uint seed) {\n";
	code += "\treturn rand_from_seed(seed) * 2.0 - 1.0;\n";
	code += "}\n\n";

	code += "uint hash(uint x) {\n";
	code += "\tx = ((x >> uint(16)) ^ x) * uint(73244475);\n";
	code += "\tx = ((x >> uint(16)) ^ x) * uint(73244475);\n";
	code += "\tx = (x >> uint(16)) ^ x;\n";
	code += "\treturn x;\n";
	code += "}\n\n";

	code += "vec2 get_random_point_on_unit_circle(vec2 position) {\n";
	code += "\treturn vec2(sin(position.x), cos(position.y));\n"; // TODO: needs to be implemented properly
	code += "}\n\n";

	code += "vec3 get_random_point_on_circle(uint seed, vec3 position, float radius) {\n";
	code += "\treturn position + vec3(0.0, get_random_point_on_unit_circle(vec2(rand_from_seed_m1_p1(seed), rand_from_seed_m1_p1(seed))) * radius);\n"; // TODO: needs to be implemented properly
	code += "}\n\n";

	return code;
}

String VisualShaderNodeEmission::generate_global_compute(VisualShader::Type p_type) const {
	String code;
	code += "\tuint base_number = NUMBER;\n";
	code += "\tuint alt_seed = hash(base_number + uint(1) + RANDOM_SEED);\n";
	return code;
}

String VisualShaderNodeEmission::generate_code(Shader::Mode p_mode, VisualShader::Type p_type, int p_id, const String *p_input_vars, const String *p_output_vars, bool p_for_preview) const {
	int idx = 0;
	String tab = "\t\t";
	String code = "\t";

	// apply condition

	if (is_input_port_connected(0)) {
		code += "if (";
		code += p_input_vars[0];
		code += ") ";
	} else {
		if (!get_input_port_default_value(0)) {
			return code;
		}
	}
	code += "{\n";

	// apply emitter_offset

	String offset = "";
	if (is_input_port_connected(1)) {
		offset += vformat("%s + ", p_input_vars[1]);
	}

	static const char *func_arr[SHAPE_TYPE_MAX] = { "get_random_point_on_circle" };
	String current_func = func_arr[shape_type];

	// apply location

	String func_params = "alt_seed, ";
	if (is_input_port_connected(2)) {
		func_params += p_input_vars[2];
	} else {
		func_params += vformat("vec3%s", get_input_port_default_value(2));
	}

	// apply shape

	switch (shape_type) {
		case ShapeType::SHAPE_TYPE_RING:
			func_params += ", ";
			if (p_input_vars[3] == String()) {
				func_params += "0.0";
			} else {
				func_params += p_input_vars[3];
			}
			break;
		default:
			break;
	}

	// apply particle position

	code += tab + vformat("TRANSFORM[3].xyz = %s%s(%s);\n", offset, current_func, func_params);

	code += "\t}\n";
	return code;
}

void VisualShaderNodeEmission::set_shape_type(ShapeType p_shape_type) {
	shape_type = p_shape_type;
	emit_changed();
}

VisualShaderNodeEmission::ShapeType VisualShaderNodeEmission::get_shape_type() const {
	return shape_type;
}

void VisualShaderNodeEmission::set_initial_velocity_type(VelocityType p_velocity_type) {
	initial_velocity_type = p_velocity_type;
	emit_changed();
}

VisualShaderNodeEmission::VelocityType VisualShaderNodeEmission::get_initial_velocity_type() const {
	return initial_velocity_type;
}

VisualShaderNodeEmission::VisualShaderNodeEmission() {
	set_input_port_default_value(0, true);
	set_input_port_default_value(1, Vector3(0.0, 0.0, 0.0));
	set_input_port_default_value(2, Vector3(0.0, 0.0, 0.0));
}
