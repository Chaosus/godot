/*************************************************************************/
/*  compute_algorithm_2d.cpp                                             */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "compute_algorithm_2d.h"

void ComputeAlgorithm2D::set_size(const Vector2i &p_size) {
	if (size == p_size) {
		return;
	}
	ERR_FAIL_COND_MSG(size.x <= 0, "Width must be greater than 0.");
	ERR_FAIL_COND_MSG(size.y <= 0, "Height must be greater than 0.");

	size = p_size;
	emit_changed();
}

Vector2i ComputeAlgorithm2D::get_size() const {
	return size;
}

String ComputeAlgorithm2D::get_code() const {
	String code;
	code += "#[compute]\n";
	code += "\n";
	code += "#version 450\n";
	code += "\n";
	code += "layout(local_size_x = " + itos(size.x) + ", local_size_y = " + itos(size.y) + ", local_size_z = 1) in;\n";
	code += "\n";
	code += _get_global_code();
	code += "\n\n";
	code += "void main() {\n";
	code += _get_main_code();
	code += "\n}\n";
	return code;
}

void ComputeAlgorithm2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_size", "size"), &ComputeAlgorithm2D::set_size);
	ClassDB::bind_method(D_METHOD("get_size"), &ComputeAlgorithm2D::get_size);

	ClassDB::bind_method(D_METHOD("get_code"), &ComputeAlgorithm2D::get_code);

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "size"), "set_size", "get_size");
}

ComputeAlgorithm2D::ComputeAlgorithm2D() {
}
