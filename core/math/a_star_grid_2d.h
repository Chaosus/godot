/*************************************************************************/
/*  a_star_grid_2d.h                                                     */
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

#ifndef A_STAR_GRID_2D_H
#define A_STAR_GRID_2D_H

#include "core/object/gdvirtual.gen.inc"
#include "core/object/ref_counted.h"
#include "core/object/script_language.h"
#include "core/templates/oa_hash_map.h"

class AStarGrid2D : public RefCounted {
	GDCLASS(AStarGrid2D, RefCounted);

	Vector2i size;
	Vector2 offset;
	Vector2 spacing;
	bool use_four_dir = false;

	enum Dir {
		DIR_LEFT,
		DIR_RIGHT,
		DIR_TOP,
		DIR_BOTTOM,
		DIR_TOP_LEFT,
		DIR_TOP_RIGHT,
		DIR_BOTTOM_LEFT,
		DIR_BOTTOM_RIGHT,
	};

	struct Point {
		Point() {}

		int id = 0;
		Vector2 pos;
		real_t weight_scale = 0;
		bool enabled = false;

		OAHashMap<int, Point *> neighbours = 4u;

		// Used for pathfinding.
		Point *prev_point = nullptr;
		real_t g_score = 0;
		real_t f_score = 0;
		uint64_t open_pass = 0;
		uint64_t closed_pass = 0;
	};

	struct SortPoints {
		_FORCE_INLINE_ bool operator()(const Point *A, const Point *B) const { // Returns true when the Point A is worse than Point B.
			if (A->f_score > B->f_score) {
				return true;
			} else if (A->f_score < B->f_score) {
				return false;
			} else {
				return A->g_score < B->g_score; // If the f_costs are the same then prioritize the points that are further away from the start.
			}
		}
	};

	int last_free_id = 0;
	uint64_t pass = 1;

	OAHashMap<int, Point *> points;

	bool _solve(Point *p_begin_point, Point *p_end_point);

protected:
	static void _bind_methods();

	virtual real_t _estimate_cost(int p_from_id, int p_to_id);
	virtual real_t _compute_cost(int p_from_id, int p_to_id);

	GDVIRTUAL2RC(real_t, _estimate_cost, int64_t, int64_t)
	GDVIRTUAL2RC(real_t, _compute_cost, int64_t, int64_t)

public:
	Vector2i get_size() const;
	Vector2 get_offset() const;
	Vector2 get_spacing() const;
	bool is_use_four_dir() const;

	void setup(const Vector2i &p_size, const Vector2 &p_offset = Vector2(), const Vector2 &p_spacing = Vector2(1, 1), bool p_use_four_dir = false);
	void clear();

	int get_point_count() const;
	int get_point_id(const Vector2i &p_position);

	real_t get_point_weight_scale(int p_id) const;
	void set_point_weight_scale(int p_id, real_t p_weight_scale);

	void set_point_disabled(int p_id, bool p_disabled = true);
	bool is_point_disabled(int p_id) const;

	Vector2 get_point_position(int p_id) const;

	Vector<Vector2> get_point_path(int p_from_id, int p_to_id);
	Vector<int> get_id_path(int p_from_id, int p_to_id);

	AStarGrid2D() {}
	~AStarGrid2D();
};

#endif // A_STAR_GRID_2D_H