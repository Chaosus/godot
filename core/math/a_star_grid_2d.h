/**************************************************************************/
/*  a_star_grid_2d.h                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef A_STAR_GRID_2D_H
#define A_STAR_GRID_2D_H

#include "core/object/gdvirtual.gen.inc"
#include "core/object/ref_counted.h"
#include "core/object/script_language.h"
#include "core/templates/list.h"
#include "core/templates/local_vector.h"
#include "core/templates/rb_set.h"
#include "core/variant/typed_array.h"

class AStarGrid2D : public RefCounted {
	GDCLASS(AStarGrid2D, RefCounted);

public:
	enum DiagonalMode {
		DIAGONAL_MODE_ALWAYS,
		DIAGONAL_MODE_NEVER,
		DIAGONAL_MODE_AT_LEAST_ONE_WALKABLE,
		DIAGONAL_MODE_ONLY_IF_NO_OBSTACLES,
		DIAGONAL_MODE_MAX,
	};

	enum Heuristic {
		HEURISTIC_EUCLIDEAN,
		HEURISTIC_MANHATTAN,
		HEURISTIC_OCTILE,
		HEURISTIC_CHEBYSHEV,
		HEURISTIC_MAX,
	};

private:
	Rect2i region;
	Vector2 offset;
	Size2 cell_size = Size2(1, 1);
	bool dirty = false;

	bool jumping_enabled = false;
	DiagonalMode diagonal_mode = DIAGONAL_MODE_ALWAYS;
	Heuristic default_compute_heuristic = HEURISTIC_EUCLIDEAN;
	Heuristic default_estimate_heuristic = HEURISTIC_EUCLIDEAN;

	struct Portal {
		RBSet<Vector2i> links;
		bool enabled = true;
		real_t travel_weight_scale = 1.0;

		Portal() {}
		Portal(bool p_enabled, real_t p_travel_weight_scale) :
				enabled(p_enabled), travel_weight_scale(p_travel_weight_scale) {
		}
	};

	struct Point {
		Vector2i id;

		bool solid = false;
		Vector2 pos;
		real_t weight_scale = 1.0;

		// Used for pathfinding.
		Point *prev_point = nullptr;
		real_t g_score = 0;
		real_t f_score = 0;
		uint64_t open_pass = 0;
		uint64_t closed_pass = 0;

		Portal *portal = nullptr;

		_FORCE_INLINE_ void add_portal(real_t p_travel_weight_scale) {
			portal = memnew(Portal(true, p_travel_weight_scale));
		}
		_FORCE_INLINE_ void delete_portal() {
			memdelete(portal);
			portal = nullptr;
		}

		Point() {}

		Point(const Vector2i &p_id, const Vector2 &p_pos) :
				id(p_id), pos(p_pos) {}

		~Point() {
			if (portal != nullptr) {
				delete_portal();
			}
		}
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

	LocalVector<LocalVector<Point>> points;
	TypedArray<Vector2i> portal_ids;
	Point *end = nullptr;

	uint64_t pass = 1;

private: // Internal routines.
	_FORCE_INLINE_ bool _is_walkable(int64_t p_x, int64_t p_y) const {
		if (region.has_point(Vector2i(p_x, p_y))) {
			return !points[p_y - region.position.y][p_x - region.position.x].solid;
		}
		return false;
	}

	_FORCE_INLINE_ Point *_get_point(int64_t p_x, int64_t p_y) {
		if (region.has_point(Vector2i(p_x, p_y))) {
			return &points[p_y - region.position.y][p_x - region.position.x];
		}
		return nullptr;
	}

	_FORCE_INLINE_ Point *_get_point_unchecked(int64_t p_x, int64_t p_y) {
		return &points[p_y - region.position.y][p_x - region.position.x];
	}

	void _get_nbors(Point *p_point, LocalVector<Point *> &r_nbors);
	_FORCE_INLINE_ void _make_portal_links(Point *p_point, LocalVector<Point *> &r_list) {
		if (!jumping_enabled && p_point->portal != nullptr && p_point->portal->enabled) {
			for (const Vector2i &link : p_point->portal->links) {
				Point *p = _get_point_unchecked(link.x, link.y);
				if (!p->solid) {
					if (p->portal != nullptr && !p->portal->enabled) {
						continue;
					}
					r_list.push_back(p);
				}
			}
		}
	}
	Point *_jump(Point *p_from, Point *p_to);
	bool _solve(Point *p_begin_point, Point *p_end_point);

protected:
	static void _bind_methods();

	virtual real_t _estimate_cost(const Vector2i &p_from_id, const Vector2i &p_to_id);
	virtual real_t _compute_cost(const Vector2i &p_from_id, const Vector2i &p_to_id);

	GDVIRTUAL2RC(real_t, _estimate_cost, Vector2i, Vector2i)
	GDVIRTUAL2RC(real_t, _compute_cost, Vector2i, Vector2i)

public:
	void set_region(const Rect2i &p_region);
	Rect2i get_region() const;

	void set_size(const Size2i &p_size);
	Size2i get_size() const;

	void set_offset(const Vector2 &p_offset);
	Vector2 get_offset() const;

	void set_cell_size(const Size2 &p_cell_size);
	Size2 get_cell_size() const;

	void update();

	int get_width() const;
	int get_height() const;

	bool is_in_bounds(int p_x, int p_y) const;
	bool is_in_boundsv(const Vector2i &p_id) const;
	bool is_dirty() const;

	void set_jumping_enabled(bool p_enabled);
	bool is_jumping_enabled() const;

	void set_diagonal_mode(DiagonalMode p_diagonal_mode);
	DiagonalMode get_diagonal_mode() const;

	void set_default_compute_heuristic(Heuristic p_heuristic);
	Heuristic get_default_compute_heuristic() const;

	void set_default_estimate_heuristic(Heuristic p_heuristic);
	Heuristic get_default_estimate_heuristic() const;

	void set_point_solid(const Vector2i &p_id, bool p_solid = true);
	bool is_point_solid(const Vector2i &p_id) const;

	void set_point_weight_scale(const Vector2i &p_id, real_t p_weight_scale);
	real_t get_point_weight_scale(const Vector2i &p_id) const;

	void fill_solid_region(const Rect2i &p_region, bool p_solid = true);
	void fill_weight_scale_region(const Rect2i &p_region, real_t p_weight_scale);

	void add_portal(const Vector2i &p_id, real_t p_travel_weight_scale = 0.0);
	void remove_portal(const Vector2i &p_id);
	void connect_portals(const Vector2i &p_from_id, const Vector2i &p_to_id, bool p_bidirectional = true);
	void connect_portal_to_point(const Vector2i &p_from_id, const Vector2i &p_to_id);
	void disconnect_portals(const Vector2i &p_from_id, const Vector2i &p_to_id, bool p_bidirectional = true);
	void disconnect_portal_from_point(const Vector2i &p_from_id, const Vector2i &p_to_id);
	void clear_portal_connections(const Vector2i &p_id, bool p_bidirectional = true);
	bool is_portal_existed_at(const Vector2i &p_id) const;
	bool is_portal_connected_to(const Vector2i &p_portal_id, const Vector2i &p_target_id);
	void set_portal_disabled(const Vector2i &p_id, bool p_disabled = true);
	bool is_portal_disabled(const Vector2i &p_id) const;
	void set_portal_travel_weight_scale(const Vector2i &p_id, real_t p_travel_weight_scale = 0.0);
	real_t get_portal_travel_weight_scale(const Vector2i &p_id) const;
	int get_portal_connections_amount(const Vector2i &p_id) const;
	TypedArray<Vector2i> get_portals_location() const;

	void clear();

	Vector2 get_point_position(const Vector2i &p_id) const;
	Vector<Vector2> get_point_path(const Vector2i &p_from, const Vector2i &p_to);
	TypedArray<Vector2i> get_id_path(const Vector2i &p_from, const Vector2i &p_to);
};

VARIANT_ENUM_CAST(AStarGrid2D::DiagonalMode);
VARIANT_ENUM_CAST(AStarGrid2D::Heuristic);

#endif // A_STAR_GRID_2D_H
