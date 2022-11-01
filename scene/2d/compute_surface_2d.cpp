/*************************************************************************/
/*  compute_surface_2d.cpp                                               */
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

#include "compute_surface_2d.h"
#include "core/core_string_names.h"
#include "servers/rendering_server.h"

void ComputeSurface2D::set_algorithm(Ref<ComputeAlgorithm2D> p_algorithm) {
	if (algorithm != p_algorithm) {
		if (algorithm.is_valid()) {
			algorithm->disconnect(CoreStringNames::get_singleton()->changed, callable_mp(this, &ComputeSurface2D::_update));
		}
		algorithm = p_algorithm;
		if (algorithm.is_valid()) {
			algorithm->connect(CoreStringNames::get_singleton()->changed, callable_mp(this, &ComputeSurface2D::_update));
		}
		_update();
	}
}

Ref<ComputeAlgorithm2D> ComputeSurface2D::get_algorithm() const {
	return algorithm;
}

void ComputeSurface2D::_update() {
	if (algorithm.is_valid()) {
		if (rd.is_null()) {
			rd = Ref<RenderingDevice>(RenderingServer::get_singleton()->create_local_rendering_device());
		}
	}
}

void ComputeSurface2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_algorithm", "algorithm"), &ComputeSurface2D::set_algorithm);
	ClassDB::bind_method(D_METHOD("get_algorithm"), &ComputeSurface2D::get_algorithm);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "algorithm", PROPERTY_HINT_RESOURCE_TYPE, "ComputeAlgorithm2D"), "set_algorithm", "get_algorithm");
}

ComputeSurface2D::ComputeSurface2D() {
}
