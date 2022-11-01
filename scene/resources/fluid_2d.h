/*************************************************************************/
/*  fluid_2d.h                                                           */
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

#ifndef FLUID_2D_H
#define FLUID_2D_H

#include "scene/resources/compute_algorithm_2d.h"

class Fluid2D : public ComputeAlgorithm2D {
private:
	GDCLASS(Fluid2D, ComputeAlgorithm2D);

public:
	enum DrawMode {
		DRAW_MODE_DENSITY,
		DRAW_MODE_TEMPERATURE,
		DRAW_MODE_VELOCITY,
		DRAW_MODE_MAX,
	};

private:
	DrawMode draw_mode = DRAW_MODE_DENSITY;

	Ref<Image> density_image;
	Ref<Image> temperature_image;
	Ref<Image> velocity_image;

	Ref<ImageTexture> density_texture;
	Ref<ImageTexture> temperature_texture;
	Ref<ImageTexture> velocity_texture;

protected:
	static void _bind_methods();
	void _notification(int p_what);

	virtual String _get_global_code() const override;
	virtual String _get_main_code() const override;

private:
	void _reset();
	void _reset_density();
	void _reset_temperature();
	void _reset_velocity();

	void _fluid_process(double p_delta);

	void _pixel_set(Ref<Image> &p_image, const Vector2i &p_coord, float p_value, const Vector2 &p_limit);
	void _circle_set(Ref<Image> &p_image, const Vector2i &p_coord, float p_radius, float p_value, const Vector2 &p_limit);
	void _rect_set(Ref<Image> &p_image, const Vector2i &p_coord, const Vector2i &p_size, float p_value, const Vector2 &p_limit);

	void _pixel_add(Ref<Image> &p_image, const Vector2i &p_coord, float p_value, const Vector2 &p_limit);
	void _circle_add(Ref<Image> &p_image, const Vector2i &p_coord, float p_radius, float p_value, const Vector2 &p_limit);
	void _rect_add(Ref<Image> &p_image, const Vector2i &p_coord, const Vector2i &p_size, float p_value, const Vector2 &p_limit);

	void _pixel_sub(Ref<Image> &p_image, const Vector2i &p_coord, float p_value, const Vector2 &p_limit);
	void _circle_sub(Ref<Image> &p_image, const Vector2i &p_coord, float p_radius, float p_value, const Vector2 &p_limit);
	void _rect_sub(Ref<Image> &p_image, const Vector2i &p_coord, const Vector2i &p_size, float p_value, const Vector2 &p_limit);

public:
	void clear();

	void set_draw_mode(DrawMode p_mode);
	DrawMode get_draw_mode() const;

	Ref<ImageTexture> get_density_texture() const;
	Ref<ImageTexture> get_temperature_texture() const;
	Ref<ImageTexture> get_velocity_texture() const;

	void set_density_to_pixel(const Vector2i &p_coord, float p_value);
	void set_density_to_circle(const Vector2i &p_coord, float p_radius, float p_value);
	void set_density_to_rect(const Vector2i &p_coord, const Vector2i &p_size, float p_value);

	void add_density_to_pixel(const Vector2i &p_coord, float p_value);
	void add_density_to_circle(const Vector2i &p_coord, float p_radius, float p_value);
	void add_density_to_rect(const Vector2i &p_coord, const Vector2i &p_size, float p_value);

	void sub_density_from_pixel(const Vector2i &p_coord, float p_value);
	void sub_density_from_circle(const Vector2i &p_coord, float p_radius, float p_value);
	void sub_density_from_rect(const Vector2i &p_coord, const Vector2i &p_size, float p_value);

	Fluid2D();
};

VARIANT_ENUM_CAST(Fluid2D::DrawMode);

#endif // FLUID_2D_H
