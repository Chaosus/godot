/*************************************************************************/
/*  fluid_2d.cpp                                                         */
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

#include "fluid_2d.h"

#define DENSITY_LIMIT Vector2(0.0, 100.0)
#define TEMPERATURE_LIMIT Vector2(0, 1000.0)

String Fluid2D::_get_global_code() const {
	return "";
}

String Fluid2D::_get_main_code() const {
	return "";
}

void Fluid2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PROCESS: {
			_fluid_process(1.0f);
		} break;
		case NOTIFICATION_DRAW: {
			Ref<Texture2D> texture;

			switch (draw_mode) {
				case DRAW_MODE_DENSITY:
					texture = density_texture;
					break;
				case DRAW_MODE_TEMPERATURE:
					texture = temperature_texture;
					break;
				case DRAW_MODE_VELOCITY:
					texture = velocity_texture;
					break;
			}

			if (texture.is_null()) {
				return;
			}

			RID ci = get_canvas_item();
			Rect2 rect = Rect2(Point2(0, 0), Point2(size.x, size.y));
			texture->draw_rect_region(ci, rect, rect, Color(1, 1, 1), false, false);
		} break;
	}
}

void Fluid2D::_fluid_process(double p_delta) {
	density_texture->update(density_image);
}

void Fluid2D::_reset() {
	_reset_density();
	_reset_temperature();
}

void Fluid2D::_reset_density() {
	density_image = Image::create_empty(size.x, size.y, false, Image::FORMAT_RF);
	density_texture = ImageTexture::create_from_image(density_image);
}

void Fluid2D::_reset_temperature() {
	temperature_image = Image::create_empty(size.x, size.y, false, Image::FORMAT_RF);
	temperature_texture = ImageTexture::create_from_image(temperature_image);
}

void Fluid2D::clear() {
	_reset();
}

void Fluid2D::set_draw_mode(DrawMode p_draw_mode) {
	if (draw_mode == p_draw_mode) {
		return;
	}
	ERR_FAIL_INDEX(p_draw_mode, DRAW_MODE_MAX);

	draw_mode = p_draw_mode;
	queue_redraw();
}

Fluid2D::DrawMode Fluid2D::get_draw_mode() const {
	return draw_mode;
}

Ref<ImageTexture> Fluid2D::get_density_texture() const {
	return density_texture;
}

Ref<ImageTexture> Fluid2D::get_temperature_texture() const {
	return temperature_texture;
}

Ref<ImageTexture> Fluid2D::get_velocity_texture() const {
	return velocity_texture;
}

void Fluid2D::_pixel_set(Ref<Image> &p_image, const Vector2i &p_coord, float p_value, const Vector2 &p_min_max) {
	p_image->set_pixelv(p_coord, Color(CLAMP(p_value, p_min_max.x, p_min_max.y), 0, 0));
}

void Fluid2D::_circle_set(Ref<Image> &p_image, const Vector2i &p_coord, float p_radius, float p_value, const Vector2 &p_min_max) {
}

void Fluid2D::_rect_set(Ref<Image> &p_image, const Vector2i &p_coord, const Vector2i &p_size, float p_value, const Vector2 &p_min_max) {
	if (p_size.x == 1 && p_size.y == 1) {
		_pixel_set(p_image, p_coord, p_value, p_min_max);
		return;
	}
	ERR_FAIL_COND_MSG(p_size.x <= 0, "Width must be greater than 0.");
	ERR_FAIL_COND_MSG(p_size.y <= 0, "Height must be greater than 0.");

	int left = CLAMP(p_coord.x, 0, p_size.x - 1);
	int right = CLAMP(p_coord.x + p_size.x, 0, p_size.x - 1);
	int top = CLAMP(p_coord.y, 0, p_size.y - 1);
	int bottom = CLAMP(p_coord.y + p_size.y, 0, p_size.y - 1);
	Color value = Color(CLAMP(p_value, p_min_max.x, p_min_max.y), 0, 0);

	for (int y = top; y < bottom; y++) {
		for (int x = left; x < right; x++) {
			p_image->set_pixel(x, y, value);
		}
	}
}

void Fluid2D::_pixel_add(Ref<Image> &p_image, const Vector2i &p_coord, float p_value, const Vector2 &p_min_max) {
	p_image->set_pixelv(p_coord, Color(CLAMP(p_image->get_pixelv(p_coord).r + p_value, p_min_max.x, p_min_max.y), 0, 0));
}

void Fluid2D::_circle_add(Ref<Image> &p_image, const Vector2i &p_coord, float p_radius, float p_value, const Vector2 &p_min_max) {
}

void Fluid2D::_rect_add(Ref<Image> &p_image, const Vector2i &p_coord, const Vector2i &p_size, float p_value, const Vector2 &p_min_max) {
	if (p_size.x == 1 && p_size.y == 1) {
		_pixel_add(p_image, p_coord, p_value, p_min_max);
		return;
	}
	ERR_FAIL_COND_MSG(p_size.x <= 0, "Width must be greater than 0.");
	ERR_FAIL_COND_MSG(p_size.y <= 0, "Height must be greater than 0.");

	int left = CLAMP(p_coord.x, 0, p_size.x - 1);
	int right = CLAMP(p_coord.x + p_size.x, 0, p_size.x - 1);
	int top = CLAMP(p_coord.y, 0, p_size.y - 1);
	int bottom = CLAMP(p_coord.y + p_size.y, 0, p_size.y - 1);

	for (int y = top; y < bottom; y++) {
		for (int x = left; x < right; x++) {
			p_image->set_pixel(x, y, Color(CLAMP(p_image->get_pixel(x, y).r + p_value, p_min_max.x, p_min_max.y), 0, 0));
		}
	}
}

void Fluid2D::_pixel_sub(Ref<Image> &p_image, const Vector2i &p_coord, float p_value, const Vector2 &p_min_max) {
	p_image->set_pixelv(p_coord, Color(CLAMP(p_image->get_pixelv(p_coord).r - p_value, p_min_max.x, p_min_max.y), 0, 0));
}

void Fluid2D::_circle_sub(Ref<Image> &p_image, const Vector2i &p_coord, float p_radius, float p_value, const Vector2 &p_min_max) {
}

void Fluid2D::_rect_sub(Ref<Image> &p_image, const Vector2i &p_coord, const Vector2i &p_size, float p_value, const Vector2 &p_min_max) {
	if (p_size.x == 1 && p_size.y == 1) {
		_pixel_sub(p_image, p_coord, p_value, p_min_max);
		return;
	}
	ERR_FAIL_COND_MSG(p_size.x <= 0, "Width must be greater than 0.");
	ERR_FAIL_COND_MSG(p_size.y <= 0, "Height must be greater than 0.");

	int left = CLAMP(p_coord.x, 0, p_size.x - 1);
	int right = CLAMP(p_coord.x + p_size.x, 0, p_size.x - 1);
	int top = CLAMP(p_coord.y, 0, p_size.y - 1);
	int bottom = CLAMP(p_coord.y + p_size.y, 0, p_size.y - 1);

	for (int y = top; y < bottom; y++) {
		for (int x = left; x < right; x++) {
			p_image->set_pixel(x, y, Color(CLAMP(p_image->get_pixel(x, y).r - p_value, p_min_max.x, p_min_max.y), 0, 0));
		}
	}
}

void Fluid2D::set_density_to_pixel(const Vector2i &p_coord, float p_value) {
	_pixel_set(density_image, p_coord, p_value, DENSITY_LIMIT);
}

void Fluid2D::set_density_to_circle(const Vector2i &p_coord, float p_radius, float p_value) {
	_circle_set(density_image, p_coord, p_radius, p_value, DENSITY_LIMIT);
}

void Fluid2D::set_density_to_rect(const Vector2i &p_coord, const Vector2i &p_size, float p_value) {
	_rect_set(density_image, p_coord, p_size, p_value, DENSITY_LIMIT);
}

void Fluid2D::add_density_to_pixel(const Vector2i &p_coord, float p_amount) {
	_pixel_add(density_image, p_coord, p_amount, DENSITY_LIMIT);
}

void Fluid2D::add_density_to_circle(const Vector2i &p_coord, float p_radius, float p_amount) {
	_circle_add(density_image, p_coord, p_radius, p_amount, DENSITY_LIMIT);
}

void Fluid2D::add_density_to_rect(const Vector2i &p_coord, const Vector2i &p_size, float p_amount) {
	_rect_add(density_image, p_coord, p_size, p_amount, DENSITY_LIMIT);
}

void Fluid2D::sub_density_from_pixel(const Vector2i &p_coord, float p_amount) {
	_pixel_sub(density_image, p_coord, p_amount, DENSITY_LIMIT);
}

void Fluid2D::sub_density_from_circle(const Vector2i &p_coord, float p_radius, float p_amount) {
	_circle_sub(density_image, p_coord, p_radius, p_amount, DENSITY_LIMIT);
}

void Fluid2D::sub_density_from_rect(const Vector2i &p_coord, const Vector2i &p_size, float p_amount) {
	_rect_sub(density_image, p_coord, p_size, p_amount, DENSITY_LIMIT);
}

void Fluid2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("clear"), &Fluid2D::clear);

	ClassDB::bind_method(D_METHOD("set_draw_mode", "draw_mode"), &Fluid2D::set_draw_mode);
	ClassDB::bind_method(D_METHOD("get_draw_mode"), &Fluid2D::get_draw_mode);

	ClassDB::bind_method(D_METHOD("get_density_texture"), &Fluid2D::get_density_texture);
	ClassDB::bind_method(D_METHOD("get_temperature_texture"), &Fluid2D::get_temperature_texture);
	ClassDB::bind_method(D_METHOD("get_velocity_texture"), &Fluid2D::get_velocity_texture);

	ClassDB::bind_method(D_METHOD("set_density_to_pixel", "coord", "value"), &Fluid2D::set_density_to_pixel);
	ClassDB::bind_method(D_METHOD("set_density_to_circle", "coord", "radius", "value"), &Fluid2D::set_density_to_circle);
	ClassDB::bind_method(D_METHOD("set_density_to_rect", "coord", "size", "value"), &Fluid2D::set_density_to_rect);

	ClassDB::bind_method(D_METHOD("add_density_to_pixel", "coord", "amount"), &Fluid2D::add_density_to_pixel);
	ClassDB::bind_method(D_METHOD("add_density_to_circle", "coord", "radius", "amount"), &Fluid2D::add_density_to_circle);
	ClassDB::bind_method(D_METHOD("add_density_to_rect", "coord", "size", "amount"), &Fluid2D::add_density_to_rect);

	ClassDB::bind_method(D_METHOD("sub_density_from_pixel", "coord", "amount"), &Fluid2D::sub_density_from_pixel);
	ClassDB::bind_method(D_METHOD("sub_density_from_circle", "coord", "radius", "amount"), &Fluid2D::sub_density_from_circle);
	ClassDB::bind_method(D_METHOD("sub_density_from_rect", "coord", "size", "amount"), &Fluid2D::sub_density_from_rect);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "draw_mode", PROPERTY_HINT_ENUM, "Density,Temperature,Velocity"), "set_draw_mode", "get_draw_mode");

	BIND_ENUM_CONSTANT(DRAW_MODE_DENSITY);
	BIND_ENUM_CONSTANT(DRAW_MODE_TEMPERATURE);
	BIND_ENUM_CONSTANT(DRAW_MODE_VELOCITY);
	BIND_ENUM_CONSTANT(DRAW_MODE_MAX);
}

Fluid2D::Fluid2D() {
}
