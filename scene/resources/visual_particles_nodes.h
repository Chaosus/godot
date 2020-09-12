/*************************************************************************/
/*  visual_particles_nodes.h                                             */
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

#ifndef VISUAL_PARTICLES_NODES_H
#define VISUAL_PARTICLES_NODES_H

#include "scene/resources/visual_shader.h"

// VisualShaderNodeEmission

class VisualShaderNodeEmission : public VisualShaderNode {
	GDCLASS(VisualShaderNodeEmission, VisualShaderNode);

public:
	enum ShapeType {
		SHAPE_TYPE_RING,
		SHAPE_TYPE_MAX,
	};

	enum VelocityType {
		VELOCITY_TYPE_CONE,
		VELOCITY_TYPE_RADIAL,
		VELOCITY_TYPE_MAX,
	};

	ShapeType shape_type = SHAPE_TYPE_RING;
	VelocityType initial_velocity_type = VELOCITY_TYPE_CONE;

protected:
	static void _bind_methods();

public:
	virtual int get_input_port_count() const override;
	virtual PortType get_input_port_type(int p_port) const override;
	virtual String get_input_port_name(int p_port) const override;

	virtual int get_output_port_count() const override;
	virtual PortType get_output_port_type(int p_port) const override;
	virtual String get_output_port_name(int p_port) const override;

	virtual String get_caption() const override;

	virtual bool is_generate_input_var(int p_port) const override;
	virtual String generate_global_per_node(Shader::Mode p_mode, VisualShader::Type p_type, int p_id) const override;
	virtual String generate_global_compute(VisualShader::Type p_type) const override;
	virtual String generate_code(Shader::Mode p_mode, VisualShader::Type p_type, int p_id, const String *p_input_vars, const String *p_output_vars, bool p_for_preview = false) const override;

	virtual Vector<StringName> get_editable_properties() const override;

	void set_shape_type(ShapeType p_shape_type);
	ShapeType get_shape_type() const;

	void set_initial_velocity_type(VelocityType p_velocity_type);
	VelocityType get_initial_velocity_type() const;

	VisualShaderNodeEmission();
};

VARIANT_ENUM_CAST(VisualShaderNodeEmission::ShapeType)
VARIANT_ENUM_CAST(VisualShaderNodeEmission::VelocityType)

#endif
