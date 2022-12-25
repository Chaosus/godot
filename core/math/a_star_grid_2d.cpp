/**************************************************************************/
/*  a_star_grid_2d.cpp                                                    */
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

#include "a_star_grid_2d.h"

#include "core/variant/typed_array.h"

static real_t heuristic_euclidean(const Vector2i &p_from, const Vector2i &p_to) {
	real_t dx = (real_t)ABS(p_to.x - p_from.x);
	real_t dy = (real_t)ABS(p_to.y - p_from.y);
	return (real_t)Math::sqrt(dx * dx + dy * dy);
}

static real_t heuristic_manhattan(const Vector2i &p_from, const Vector2i &p_to) {
	real_t dx = (real_t)ABS(p_to.x - p_from.x);
	real_t dy = (real_t)ABS(p_to.y - p_from.y);
	return dx + dy;
}

static real_t heuristic_octile(const Vector2i &p_from, const Vector2i &p_to) {
	real_t dx = (real_t)ABS(p_to.x - p_from.x);
	real_t dy = (real_t)ABS(p_to.y - p_from.y);
	real_t F = Math_SQRT2 - 1;
	return (dx < dy) ? F * dx + dy : F * dy + dx;
}

static real_t heuristic_chebyshev(const Vector2i &p_from, const Vector2i &p_to) {
	real_t dx = (real_t)ABS(p_to.x - p_from.x);
	real_t dy = (real_t)ABS(p_to.y - p_from.y);
	return MAX(dx, dy);
}

static real_t (*heuristics[AStarGrid2D::HEURISTIC_MAX])(const Vector2i &, const Vector2i &) = { heuristic_euclidean, heuristic_manhattan, heuristic_octile, heuristic_chebyshev };

void AStarGrid2D::set_region(const Rect2i &p_region) {
	ERR_FAIL_COND(p_region.size.x < 0 || p_region.size.y < 0);
	if (p_region != region) {
		if (hpa_enabled) {
			if (!_hpa_is_valid(p_region.size, hpa_max_level, hpa_cluster_size)) {
				return;
			}
			hpa_dirty = true;
		}
		region = p_region;
		dirty = true;
	}
}

Rect2i AStarGrid2D::get_region() const {
	return region;
}

void AStarGrid2D::set_size(const Size2i &p_size) {
	WARN_DEPRECATED_MSG(R"(The "size" property is deprecated, use "region" instead.)");
	ERR_FAIL_COND(p_size.x < 0 || p_size.y < 0);
	if (p_size != region.size) {
		if (hpa_enabled) {
			if (!_hpa_is_valid(p_size, hpa_max_level, hpa_cluster_size)) {
				return;
			}
			hpa_dirty = true;
		}
		region.size = p_size;
		dirty = true;
	}
}

Size2i AStarGrid2D::get_size() const {
	return region.size;
}

void AStarGrid2D::set_offset(const Vector2 &p_offset) {
	if (!offset.is_equal_approx(p_offset)) {
		offset = p_offset;
		dirty = true;
	}
}

Vector2 AStarGrid2D::get_offset() const {
	return offset;
}

void AStarGrid2D::set_cell_size(const Size2 &p_cell_size) {
	if (!cell_size.is_equal_approx(p_cell_size)) {
		cell_size = p_cell_size;
		dirty = true;
	}
}

Size2 AStarGrid2D::get_cell_size() const {
	return cell_size;
}

void AStarGrid2D::update() {
	points.clear();

	const int32_t end_x = region.get_end().x;
	const int32_t end_y = region.get_end().y;

	for (int32_t y = region.position.y; y < end_y; y++) {
		LocalVector<Point> line;
		for (int32_t x = region.position.x; x < end_x; x++) {
			line.push_back(Point(Vector2i(x, y), offset + Vector2(x, y) * cell_size));
		}
		points.push_back(line);
	}

	dirty = false;
}

void AStarGrid2D::update_hpa() {
	ERR_FAIL_COND_MSG(!hpa_enabled, "HPA must be enabled before using its methods or properties.");
	hpa_graph->build(this);

	hpa_dirty = false;
}

bool AStarGrid2D::is_in_bounds(int32_t p_x, int32_t p_y) const {
	return region.has_point(Vector2i(p_x, p_y));
}

bool AStarGrid2D::is_in_boundsv(const Vector2i &p_id) const {
	return region.has_point(p_id);
}

bool AStarGrid2D::is_dirty() const {
	return dirty;
}

bool AStarGrid2D::is_hpa_dirty() const {
	return hpa_dirty;
}

void AStarGrid2D::set_hpa_enabled(bool p_enabled) {
	if (hpa_enabled == p_enabled) {
		return;
	}
	if (p_enabled) {
		if (!_hpa_is_valid(region.size, hpa_max_level, hpa_cluster_size)) {
			return;
		}
	}
	hpa_enabled = p_enabled;
	hpa_dirty = p_enabled;
	if (hpa_graph) {
		memdelete(hpa_graph);
	}
	if (hpa_enabled) {
		hpa_graph = memnew(Graph);
	} else {
		hpa_graph = nullptr;
	}
}

bool AStarGrid2D::is_hpa_enabled() const {
	return hpa_enabled;
}

void AStarGrid2D::set_max_level(int p_max_level) {
	if (hpa_max_level == p_max_level) {
		return;
	}
	ERR_FAIL_COND_MSG(!hpa_enabled, "HPA must be enabled before using its methods or properties.");
	if (!_hpa_is_valid(region.size, p_max_level, hpa_cluster_size)) {
		return;
	}
	hpa_max_level = p_max_level;
	hpa_dirty = true;
}

int AStarGrid2D::get_max_level() const {
	return hpa_max_level;
}

void AStarGrid2D::set_cluster_size(int p_cluster_size) {
	if (hpa_cluster_size == p_cluster_size) {
		return;
	}
	ERR_FAIL_COND_MSG(!hpa_enabled, "HPA must be enabled before using its methods or properties.");
	if (!_hpa_is_valid(region.size, hpa_max_level, p_cluster_size)) {
		return;
	}

	hpa_cluster_size = p_cluster_size;
	hpa_dirty = true;
}

int AStarGrid2D::get_cluster_size() const {
	return hpa_cluster_size;
}

void AStarGrid2D::set_jumping_enabled(bool p_enabled) {
	jumping_enabled = p_enabled;
}

bool AStarGrid2D::is_jumping_enabled() const {
	return jumping_enabled;
}

void AStarGrid2D::set_diagonal_mode(DiagonalMode p_diagonal_mode) {
	ERR_FAIL_INDEX((int)p_diagonal_mode, (int)DIAGONAL_MODE_MAX);
	diagonal_mode = p_diagonal_mode;
}

AStarGrid2D::DiagonalMode AStarGrid2D::get_diagonal_mode() const {
	return diagonal_mode;
}

void AStarGrid2D::set_default_compute_heuristic(Heuristic p_heuristic) {
	ERR_FAIL_INDEX((int)p_heuristic, (int)HEURISTIC_MAX);
	default_compute_heuristic = p_heuristic;
}

AStarGrid2D::Heuristic AStarGrid2D::get_default_compute_heuristic() const {
	return default_compute_heuristic;
}

void AStarGrid2D::set_default_estimate_heuristic(Heuristic p_heuristic) {
	ERR_FAIL_INDEX((int)p_heuristic, (int)HEURISTIC_MAX);
	default_estimate_heuristic = p_heuristic;
}

AStarGrid2D::Heuristic AStarGrid2D::get_default_estimate_heuristic() const {
	return default_estimate_heuristic;
}

void AStarGrid2D::set_point_solid(const Vector2i &p_id, bool p_solid) {
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_MSG(!is_in_boundsv(p_id), vformat("Can't set if point is disabled. Point %s out of bounds %s.", p_id, region));
	_get_point_unchecked(p_id)->solid = p_solid;
}

bool AStarGrid2D::is_point_solid(const Vector2i &p_id) const {
	ERR_FAIL_COND_V_MSG(dirty, false, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_id), false, vformat("Can't get if point is disabled. Point %s out of bounds %s.", p_id, region));
	return _get_point_unchecked(p_id)->solid;
}

void AStarGrid2D::set_point_weight_scale(const Vector2i &p_id, real_t p_weight_scale) {
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_MSG(!is_in_boundsv(p_id), vformat("Can't set point's weight scale. Point %s out of bounds %s.", p_id, region));
	ERR_FAIL_COND_MSG(p_weight_scale < 0.0, vformat("Can't set point's weight scale less than 0.0: %f.", p_weight_scale));
	_get_point_unchecked(p_id)->weight_scale = p_weight_scale;
}

real_t AStarGrid2D::get_point_weight_scale(const Vector2i &p_id) const {
	ERR_FAIL_COND_V_MSG(dirty, 0, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_id), 0, vformat("Can't get point's weight scale. Point %s out of bounds %s.", p_id, region));
	return _get_point_unchecked(p_id)->weight_scale;
}

void AStarGrid2D::fill_solid_region(const Rect2i &p_region, bool p_solid) {
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");

	const Rect2i safe_region = p_region.intersection(region);
	const int32_t end_x = safe_region.get_end().x;
	const int32_t end_y = safe_region.get_end().y;

	for (int32_t y = safe_region.position.y; y < end_y; y++) {
		for (int32_t x = safe_region.position.x; x < end_x; x++) {
			_get_point_unchecked(x, y)->solid = p_solid;
		}
	}
}

void AStarGrid2D::fill_weight_scale_region(const Rect2i &p_region, real_t p_weight_scale) {
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_MSG(p_weight_scale < 0.0, vformat("Can't set point's weight scale less than 0.0: %f.", p_weight_scale));

	const Rect2i safe_region = p_region.intersection(region);
	const int32_t end_x = safe_region.get_end().x;
	const int32_t end_y = safe_region.get_end().y;

	for (int32_t y = safe_region.position.y; y < end_y; y++) {
		for (int32_t x = safe_region.position.x; x < end_x; x++) {
			_get_point_unchecked(x, y)->weight_scale = p_weight_scale;
		}
	}
}

AStarGrid2D::Point *AStarGrid2D::_jump(Point *p_from, Point *p_to) {
	if (!p_to || p_to->solid) {
		return nullptr;
	}
	if (p_to == end) {
		return p_to;
	}

	int32_t from_x = p_from->id.x;
	int32_t from_y = p_from->id.y;

	int32_t to_x = p_to->id.x;
	int32_t to_y = p_to->id.y;

	int32_t dx = to_x - from_x;
	int32_t dy = to_y - from_y;

	if (diagonal_mode == DIAGONAL_MODE_ALWAYS || diagonal_mode == DIAGONAL_MODE_AT_LEAST_ONE_WALKABLE) {
		if (dx != 0 && dy != 0) {
			if ((_is_walkable(to_x - dx, to_y + dy) && !_is_walkable(to_x - dx, to_y)) || (_is_walkable(to_x + dx, to_y - dy) && !_is_walkable(to_x, to_y - dy))) {
				return p_to;
			}
			if (_jump(p_to, _get_point(to_x + dx, to_y)) != nullptr) {
				return p_to;
			}
			if (_jump(p_to, _get_point(to_x, to_y + dy)) != nullptr) {
				return p_to;
			}
		} else {
			if (dx != 0) {
				if ((_is_walkable(to_x + dx, to_y + 1) && !_is_walkable(to_x, to_y + 1)) || (_is_walkable(to_x + dx, to_y - 1) && !_is_walkable(to_x, to_y - 1))) {
					return p_to;
				}
			} else {
				if ((_is_walkable(to_x + 1, to_y + dy) && !_is_walkable(to_x + 1, to_y)) || (_is_walkable(to_x - 1, to_y + dy) && !_is_walkable(to_x - 1, to_y))) {
					return p_to;
				}
			}
		}
		if (_is_walkable(to_x + dx, to_y + dy) && (diagonal_mode == DIAGONAL_MODE_ALWAYS || (_is_walkable(to_x + dx, to_y) || _is_walkable(to_x, to_y + dy)))) {
			return _jump(p_to, _get_point(to_x + dx, to_y + dy));
		}
	} else if (diagonal_mode == DIAGONAL_MODE_ONLY_IF_NO_OBSTACLES) {
		if (dx != 0 && dy != 0) {
			if ((_is_walkable(to_x + dx, to_y + dy) && !_is_walkable(to_x, to_y + dy)) || !_is_walkable(to_x + dx, to_y)) {
				return p_to;
			}
			if (_jump(p_to, _get_point(to_x + dx, to_y)) != nullptr) {
				return p_to;
			}
			if (_jump(p_to, _get_point(to_x, to_y + dy)) != nullptr) {
				return p_to;
			}
		} else {
			if (dx != 0) {
				if ((_is_walkable(to_x, to_y + 1) && !_is_walkable(to_x - dx, to_y + 1)) || (_is_walkable(to_x, to_y - 1) && !_is_walkable(to_x - dx, to_y - 1))) {
					return p_to;
				}
			} else {
				if ((_is_walkable(to_x + 1, to_y) && !_is_walkable(to_x + 1, to_y - dy)) || (_is_walkable(to_x - 1, to_y) && !_is_walkable(to_x - 1, to_y - dy))) {
					return p_to;
				}
			}
		}
		if (_is_walkable(to_x + dx, to_y + dy) && _is_walkable(to_x + dx, to_y) && _is_walkable(to_x, to_y + dy)) {
			return _jump(p_to, _get_point(to_x + dx, to_y + dy));
		}
	} else { // DIAGONAL_MODE_NEVER
		if (dx != 0) {
			if ((_is_walkable(to_x, to_y - 1) && !_is_walkable(to_x - dx, to_y - 1)) || (_is_walkable(to_x, to_y + 1) && !_is_walkable(to_x - dx, to_y + 1))) {
				return p_to;
			}
		} else if (dy != 0) {
			if ((_is_walkable(to_x - 1, to_y) && !_is_walkable(to_x - 1, to_y - dy)) || (_is_walkable(to_x + 1, to_y) && !_is_walkable(to_x + 1, to_y - dy))) {
				return p_to;
			}
			if (_jump(p_to, _get_point(to_x + 1, to_y)) != nullptr) {
				return p_to;
			}
			if (_jump(p_to, _get_point(to_x - 1, to_y)) != nullptr) {
				return p_to;
			}
		}
		return _jump(p_to, _get_point(to_x + dx, to_y + dy));
	}
	return nullptr;
}

void AStarGrid2D::_get_nbors(Point *p_point, LocalVector<Point *> &r_nbors) {
	bool ts0 = false, td0 = false,
		 ts1 = false, td1 = false,
		 ts2 = false, td2 = false,
		 ts3 = false, td3 = false;

	Point *left = nullptr;
	Point *right = nullptr;
	Point *top = nullptr;
	Point *bottom = nullptr;

	Point *top_left = nullptr;
	Point *top_right = nullptr;
	Point *bottom_left = nullptr;
	Point *bottom_right = nullptr;

	{
		bool has_left = false;
		bool has_right = false;

		if (p_point->id.x - 1 >= region.position.x) {
			left = _get_point_unchecked(p_point->id.x - 1, p_point->id.y);
			has_left = true;
		}
		if (p_point->id.x + 1 < region.position.x + region.size.width) {
			right = _get_point_unchecked(p_point->id.x + 1, p_point->id.y);
			has_right = true;
		}
		if (p_point->id.y - 1 >= region.position.y) {
			top = _get_point_unchecked(p_point->id.x, p_point->id.y - 1);
			if (has_left) {
				top_left = _get_point_unchecked(p_point->id.x - 1, p_point->id.y - 1);
			}
			if (has_right) {
				top_right = _get_point_unchecked(p_point->id.x + 1, p_point->id.y - 1);
			}
		}
		if (p_point->id.y + 1 < region.position.y + region.size.height) {
			bottom = _get_point_unchecked(p_point->id.x, p_point->id.y + 1);
			if (has_left) {
				bottom_left = _get_point_unchecked(p_point->id.x - 1, p_point->id.y + 1);
			}
			if (has_right) {
				bottom_right = _get_point_unchecked(p_point->id.x + 1, p_point->id.y + 1);
			}
		}
	}

	if (top && !top->solid) {
		r_nbors.push_back(top);
		ts0 = true;
	}
	if (right && !right->solid) {
		r_nbors.push_back(right);
		ts1 = true;
	}
	if (bottom && !bottom->solid) {
		r_nbors.push_back(bottom);
		ts2 = true;
	}
	if (left && !left->solid) {
		r_nbors.push_back(left);
		ts3 = true;
	}

	switch (diagonal_mode) {
		case DIAGONAL_MODE_ALWAYS: {
			td0 = true;
			td1 = true;
			td2 = true;
			td3 = true;
		} break;
		case DIAGONAL_MODE_NEVER: {
		} break;
		case DIAGONAL_MODE_AT_LEAST_ONE_WALKABLE: {
			td0 = ts3 || ts0;
			td1 = ts0 || ts1;
			td2 = ts1 || ts2;
			td3 = ts2 || ts3;
		} break;
		case DIAGONAL_MODE_ONLY_IF_NO_OBSTACLES: {
			td0 = ts3 && ts0;
			td1 = ts0 && ts1;
			td2 = ts1 && ts2;
			td3 = ts2 && ts3;
		} break;
		default:
			break;
	}

	if (td0 && (top_left && !top_left->solid)) {
		r_nbors.push_back(top_left);
	}
	if (td1 && (top_right && !top_right->solid)) {
		r_nbors.push_back(top_right);
	}
	if (td2 && (bottom_right && !bottom_right->solid)) {
		r_nbors.push_back(bottom_right);
	}
	if (td3 && (bottom_left && !bottom_left->solid)) {
		r_nbors.push_back(bottom_left);
	}
}

bool AStarGrid2D::_solve(Point *p_begin_point, Point *p_end_point) {
	pass++;

	if (p_end_point->solid) {
		return false;
	}

	bool found_route = false;

	LocalVector<Point *> open_list;
	SortArray<Point *, SortPoints> sorter;

	p_begin_point->g_score = 0;
	p_begin_point->f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	open_list.push_back(p_begin_point);
	end = p_end_point;

	while (!open_list.is_empty()) {
		Point *p = open_list[0]; // The currently processed point.

		if (p == p_end_point) {
			found_route = true;
			break;
		}

		sorter.pop_heap(0, open_list.size(), open_list.ptr()); // Remove the current point from the open list.
		open_list.remove_at(open_list.size() - 1);
		p->closed_pass = pass; // Mark the point as closed.

		LocalVector<Point *> nbors;
		_get_nbors(p, nbors);

		for (Point *e : nbors) {
			real_t weight_scale = 1.0;

			if (jumping_enabled) {
				// TODO: Make it works with weight_scale.
				e = _jump(p, e);
				if (!e || e->closed_pass == pass) {
					continue;
				}
			} else {
				if (e->solid || e->closed_pass == pass) {
					continue;
				}
				weight_scale = e->weight_scale;
			}

			real_t tentative_g_score = p->g_score + _compute_cost(p->id, e->id) * weight_scale;
			bool new_point = false;

			if (e->open_pass != pass) { // The point wasn't inside the open list.
				e->open_pass = pass;
				open_list.push_back(e);
				new_point = true;
			} else if (tentative_g_score >= e->g_score) { // The new path is worse than the previous.
				continue;
			}

			e->prev_point = p;
			e->g_score = tentative_g_score;
			e->f_score = e->g_score + _estimate_cost(e->id, p_end_point->id);

			if (new_point) { // The position of the new points is already known.
				sorter.push_heap(0, open_list.size() - 1, 0, e, open_list.ptr());
			} else {
				sorter.push_heap(0, open_list.find(e), 0, e, open_list.ptr());
			}
		}
	}

	return found_route;
}

void AStarGrid2D::_solve(Point *p_begin_point, Point *p_end_point, List<Edge *> &r_edges) {
	pass++;

	if (p_end_point->solid) {
		return;
	}

	HashMap<Vector2i, Edge *> parent;

	Vector<Point *> open_list;
	SortArray<Point *, SortPoints> sorter;

	p_begin_point->g_score = 0;
	p_begin_point->f_score = _estimate_cost(p_begin_point->id, p_end_point->id);

	open_list.push_back(p_begin_point);

	while (!open_list.is_empty()) {
		Point *p = open_list[0]; // The currently processed point.

		if (p == p_end_point) {
			// Create a path and return.
			while (p != p_begin_point) {
				Edge *e = parent[p->id];
				r_edges.push_front(e);
				p = e->start;
			}
			break;
		}

		sorter.pop_heap(0, open_list.size(), open_list.ptrw()); // Remove the current point from the open list.
		open_list.remove_at(open_list.size() - 1);
		p->closed_pass = pass; // Mark the point as closed.

		// Visit all neighbours through edges going out of node.
		for (Edge *edge : p->edges) {
			if (!is_in_boundsv(edge->end->id)) {
				continue;
			}

			Point *e = edge->end;
			real_t weight_scale = 1.0;

			if (jumping_enabled) {
				// TODO: Make it works with weight_scale.
				e = _jump(p, e);
				if (!e || e->closed_pass == pass) {
					continue;
				}
			} else {
				if (e->solid || e->closed_pass == pass) {
					continue;
				}
				weight_scale = e->weight_scale;
			}

			real_t tentative_g_score = p->g_score + _compute_cost(p->id, e->id) * weight_scale;
			bool new_point = false;

			if (e->open_pass != pass) { // The point wasn't inside the open list.
				e->open_pass = pass;
				open_list.push_back(e);
				new_point = true;
			} else if (tentative_g_score >= e->g_score) { // The new path is worse than the previous.
				continue;
			}

			parent[e->id] = edge;

			e->prev_point = p;
			e->g_score = tentative_g_score;
			e->f_score = e->g_score + _estimate_cost(e->id, p_end_point->id);

			if (new_point) { // The position of the new points is already known.
				sorter.push_heap(0, open_list.size() - 1, 0, e, open_list.ptrw());
			} else {
				sorter.push_heap(0, open_list.find(e), 0, e, open_list.ptrw());
			}
		}
	}
}

real_t AStarGrid2D::_estimate_cost(const Vector2i &p_from_id, const Vector2i &p_to_id) {
	real_t scost;
	if (GDVIRTUAL_CALL(_estimate_cost, p_from_id, p_to_id, scost)) {
		return scost;
	}
	return heuristics[default_estimate_heuristic](p_from_id, p_to_id);
}

real_t AStarGrid2D::_compute_cost(const Vector2i &p_from_id, const Vector2i &p_to_id) {
	real_t scost;
	if (GDVIRTUAL_CALL(_compute_cost, p_from_id, p_to_id, scost)) {
		return scost;
	}
	return heuristics[default_compute_heuristic](p_from_id, p_to_id);
}

bool AStarGrid2D::_hpa_is_valid(const Vector2i &p_grid_size, int p_max_levels, int p_cluster_size) const {
	ERR_FAIL_COND_V_MSG(p_grid_size.width % 2 != 0 || p_grid_size.height % 2 != 0, false, "The grid size must be even to use HPA.");
	ERR_FAIL_COND_V_MSG(p_max_levels < 1 || p_max_levels > 5, false, vformat("The level size (%i) must be within 1 to 5 range.", p_cluster_size));
	ERR_FAIL_COND_V_MSG(p_cluster_size < 10 || p_cluster_size > 50, false, vformat("The cluster size (%i) must be within 10 to 50 range.", p_cluster_size));
	ERR_FAIL_COND_V_MSG(p_cluster_size > p_grid_size.width || p_cluster_size > p_grid_size.height, false, vformat("The cluster size (%i) must not exceeds a grid size (%i, %i).", p_cluster_size, p_grid_size.x, p_grid_size.y));

	float x = p_grid_size.width / (float)p_cluster_size;
	float y = p_grid_size.height / (float)p_cluster_size;

	ERR_FAIL_COND_V_MSG(Math::floor(x) != x || Math::floor(y) != y, false, "The overall amount of clusters must lay within a grid perfectly.");
	return true;
}

void AStarGrid2D::clear() {
	points.clear();
	region = Rect2i();
}

Vector2 AStarGrid2D::get_point_position(const Vector2i &p_id) const {
	ERR_FAIL_COND_V_MSG(dirty, Vector2(), "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_id), Vector2(), vformat("Can't get point's position. Point %s out of bounds %s.", p_id, region));
	return _get_point_unchecked(p_id)->pos;
}

Vector<Vector2> AStarGrid2D::get_point_path(const Vector2i &p_from_id, const Vector2i &p_to_id) {
	ERR_FAIL_COND_V_MSG(dirty, Vector<Vector2>(), "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_from_id), Vector<Vector2>(), vformat("Can't get id path. Point %s out of bounds %s.", p_from_id, region));
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_to_id), Vector<Vector2>(), vformat("Can't get id path. Point %s out of bounds %s.", p_to_id, region));
	if (hpa_enabled) {
		ERR_FAIL_COND_V_MSG(hpa_dirty, Vector<Vector2>(), "HPA is not initialized. Call the update_hpa method.");
	}

	Point *a = _get_point(p_from_id.x, p_from_id.y);
	Point *b = _get_point(p_to_id.x, p_to_id.y);

	if (a == b) {
		Vector<Vector2> ret;
		ret.push_back(a->pos);
		return ret;
	}

	Point *begin_point = a;
	Point *end_point = b;

	if (hpa_enabled) {
		hpa_graph->insert_nodes(this, p_from_id, p_to_id, begin_point, end_point);

		List<Edge *> edges;
		_solve(begin_point, end_point, edges);
		if (edges.size() > 0) {
			return Vector<Vector2>();
		}
		Vector<Vector2> path;
		for (Edge *e : edges) {
			path.push_back(e->end->pos);
		}

		hpa_graph->remove_added_nodes();
		return path;
	}

	bool found_route = _solve(begin_point, end_point);
	if (!found_route) {
		return Vector<Vector2>();
	}

	Point *p = end_point;
	int32_t pc = 1;
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<Vector2> path;
	path.resize(pc);

	{
		Vector2 *w = path.ptrw();

		p = end_point;
		int32_t idx = pc - 1;
		while (p != begin_point) {
			w[idx--] = p->pos;
			p = p->prev_point;
		}

		w[0] = p->pos;
	}

	return path;
}

TypedArray<Vector2i> AStarGrid2D::get_id_path(const Vector2i &p_from_id, const Vector2i &p_to_id) {
	ERR_FAIL_COND_V_MSG(dirty, TypedArray<Vector2i>(), "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_from_id), TypedArray<Vector2i>(), vformat("Can't get id path. Point %s out of bounds %s.", p_from_id, region));
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_to_id), TypedArray<Vector2i>(), vformat("Can't get id path. Point %s out of bounds %s.", p_to_id, region));
	if (hpa_enabled) {
		ERR_FAIL_COND_V_MSG(hpa_dirty, TypedArray<Vector2>(), "HPA is not initialized. Call the update_hpa method.");
	}

	Point *a = _get_point(p_from_id.x, p_from_id.y);
	Point *b = _get_point(p_to_id.x, p_to_id.y);

	if (a == b) {
		TypedArray<Vector2i> ret;
		ret.push_back(a->id);
		return ret;
	}

	Point *begin_point = a;
	Point *end_point = b;

	if (hpa_enabled) {
		hpa_graph->insert_nodes(this, p_from_id, p_to_id, begin_point, end_point);

		List<Edge *> edges;
		_solve(begin_point, end_point, edges);
		if (edges.size() > 0) {
			return Vector<Vector2>();
		}
		TypedArray<Vector2i> path;
		for (Edge *e : edges) {
			path.push_back(e->end->id);
		}

		hpa_graph->remove_added_nodes();
		return path;
	}

	bool found_route = _solve(begin_point, end_point);
	if (!found_route) {
		return TypedArray<Vector2i>();
	}

	Point *p = end_point;
	int32_t pc = 1;
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	TypedArray<Vector2i> path;
	path.resize(pc);

	{
		p = end_point;
		int32_t idx = pc - 1;
		while (p != begin_point) {
			path[idx--] = p->id;
			p = p->prev_point;
		}

		path[0] = p->id;
	}

	return path;
}

void AStarGrid2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_region", "region"), &AStarGrid2D::set_region);
	ClassDB::bind_method(D_METHOD("get_region"), &AStarGrid2D::get_region);
	ClassDB::bind_method(D_METHOD("set_size", "size"), &AStarGrid2D::set_size);
	ClassDB::bind_method(D_METHOD("get_size"), &AStarGrid2D::get_size);
	ClassDB::bind_method(D_METHOD("set_offset", "offset"), &AStarGrid2D::set_offset);
	ClassDB::bind_method(D_METHOD("get_offset"), &AStarGrid2D::get_offset);
	ClassDB::bind_method(D_METHOD("set_cell_size", "cell_size"), &AStarGrid2D::set_cell_size);
	ClassDB::bind_method(D_METHOD("get_cell_size"), &AStarGrid2D::get_cell_size);
	ClassDB::bind_method(D_METHOD("is_in_bounds", "x", "y"), &AStarGrid2D::is_in_bounds);
	ClassDB::bind_method(D_METHOD("is_in_boundsv", "id"), &AStarGrid2D::is_in_boundsv);
	ClassDB::bind_method(D_METHOD("is_dirty"), &AStarGrid2D::is_dirty);
	ClassDB::bind_method(D_METHOD("is_hpa_dirty"), &AStarGrid2D::is_hpa_dirty);
	ClassDB::bind_method(D_METHOD("update"), &AStarGrid2D::update);
	ClassDB::bind_method(D_METHOD("update_hpa"), &AStarGrid2D::update_hpa);
	ClassDB::bind_method(D_METHOD("set_hpa_enabled", "enabled"), &AStarGrid2D::set_hpa_enabled);
	ClassDB::bind_method(D_METHOD("is_hpa_enabled"), &AStarGrid2D::is_hpa_enabled);
	ClassDB::bind_method(D_METHOD("set_max_level", "cluster_size"), &AStarGrid2D::set_max_level);
	ClassDB::bind_method(D_METHOD("get_max_level"), &AStarGrid2D::get_max_level);
	ClassDB::bind_method(D_METHOD("set_cluster_size", "cluster_size"), &AStarGrid2D::set_cluster_size);
	ClassDB::bind_method(D_METHOD("get_cluster_size"), &AStarGrid2D::get_cluster_size);
	ClassDB::bind_method(D_METHOD("set_jumping_enabled", "enabled"), &AStarGrid2D::set_jumping_enabled);
	ClassDB::bind_method(D_METHOD("is_jumping_enabled"), &AStarGrid2D::is_jumping_enabled);
	ClassDB::bind_method(D_METHOD("set_diagonal_mode", "mode"), &AStarGrid2D::set_diagonal_mode);
	ClassDB::bind_method(D_METHOD("get_diagonal_mode"), &AStarGrid2D::get_diagonal_mode);
	ClassDB::bind_method(D_METHOD("set_default_compute_heuristic", "heuristic"), &AStarGrid2D::set_default_compute_heuristic);
	ClassDB::bind_method(D_METHOD("get_default_compute_heuristic"), &AStarGrid2D::get_default_compute_heuristic);
	ClassDB::bind_method(D_METHOD("set_default_estimate_heuristic", "heuristic"), &AStarGrid2D::set_default_estimate_heuristic);
	ClassDB::bind_method(D_METHOD("get_default_estimate_heuristic"), &AStarGrid2D::get_default_estimate_heuristic);
	ClassDB::bind_method(D_METHOD("set_point_solid", "id", "solid"), &AStarGrid2D::set_point_solid, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("is_point_solid", "id"), &AStarGrid2D::is_point_solid);
	ClassDB::bind_method(D_METHOD("set_point_weight_scale", "id", "weight_scale"), &AStarGrid2D::set_point_weight_scale);
	ClassDB::bind_method(D_METHOD("get_point_weight_scale", "id"), &AStarGrid2D::get_point_weight_scale);
	ClassDB::bind_method(D_METHOD("fill_solid_region", "region", "solid"), &AStarGrid2D::fill_solid_region, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("fill_weight_scale_region", "region", "weight_scale"), &AStarGrid2D::fill_weight_scale_region);
	ClassDB::bind_method(D_METHOD("clear"), &AStarGrid2D::clear);

	ClassDB::bind_method(D_METHOD("get_point_position", "id"), &AStarGrid2D::get_point_position);
	ClassDB::bind_method(D_METHOD("get_point_path", "from_id", "to_id"), &AStarGrid2D::get_point_path);
	ClassDB::bind_method(D_METHOD("get_id_path", "from_id", "to_id"), &AStarGrid2D::get_id_path);

	GDVIRTUAL_BIND(_estimate_cost, "from_id", "to_id")
	GDVIRTUAL_BIND(_compute_cost, "from_id", "to_id")

	ADD_PROPERTY(PropertyInfo(Variant::RECT2I, "region"), "set_region", "get_region");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "size"), "set_size", "get_size");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "offset"), "set_offset", "get_offset");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "cell_size"), "set_cell_size", "get_cell_size");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "jumping_enabled"), "set_jumping_enabled", "is_jumping_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "default_compute_heuristic", PROPERTY_HINT_ENUM, "Euclidean,Manhattan,Octile,Chebyshev"), "set_default_compute_heuristic", "get_default_compute_heuristic");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "default_estimate_heuristic", PROPERTY_HINT_ENUM, "Euclidean,Manhattan,Octile,Chebyshev"), "set_default_estimate_heuristic", "get_default_estimate_heuristic");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "diagonal_mode", PROPERTY_HINT_ENUM, "Never,Always,At Least One Walkable,Only If No Obstacles"), "set_diagonal_mode", "get_diagonal_mode");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hpa_enabled"), "set_hpa_enabled", "is_hpa_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "hpa_max_level", PROPERTY_HINT_RANGE, "1,5,1"), "set_max_level", "get_max_level");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "hpa_cluster_size", PROPERTY_HINT_RANGE, "10,50,1"), "set_cluster_size", "get_cluster_size");

	BIND_ENUM_CONSTANT(HEURISTIC_EUCLIDEAN);
	BIND_ENUM_CONSTANT(HEURISTIC_MANHATTAN);
	BIND_ENUM_CONSTANT(HEURISTIC_OCTILE);
	BIND_ENUM_CONSTANT(HEURISTIC_CHEBYSHEV);
	BIND_ENUM_CONSTANT(HEURISTIC_MAX);

	BIND_ENUM_CONSTANT(DIAGONAL_MODE_ALWAYS);
	BIND_ENUM_CONSTANT(DIAGONAL_MODE_NEVER);
	BIND_ENUM_CONSTANT(DIAGONAL_MODE_AT_LEAST_ONE_WALKABLE);
	BIND_ENUM_CONSTANT(DIAGONAL_MODE_ONLY_IF_NO_OBSTACLES);
	BIND_ENUM_CONSTANT(DIAGONAL_MODE_MAX);
}

AStarGrid2D::AStarGrid2D() {
}

AStarGrid2D::~AStarGrid2D() {
	if (hpa_graph) {
		memdelete(hpa_graph);
	}
}

// HPA-Graph implementation.

AStarGrid2D::Graph::Graph() {
}

AStarGrid2D::Graph::~Graph() {
	_clear();
}

void AStarGrid2D::Graph::_clear() { // Clear internal array memory.
	for (AStarGrid2D::Point *p : all_nodes) {
		memdelete(p);
	}
	all_nodes.clear();

	for (Edge *e : all_edges) {
		memdelete(e);
	}
	all_edges.clear();

	for (Cluster *c : all_clusters) {
		memdelete(c);
	}
	all_clusters.clear();
}

void AStarGrid2D::Graph::build(AStarGrid2D *p_map) { // Construct a graph from the map.
	_clear();

	region = p_map->region;
	depth = p_map->hpa_max_level;

	nodes.clear();
	_create_map_nodes(p_map, nodes);

	int cluster_size = p_map->hpa_cluster_size;
	int cluster_width = 0;
	int cluster_height = 0;

	clusters.clear();
	clusters.resize(p_map->hpa_max_level);
	for (int i = 0; i < p_map->hpa_max_level; i++) {
		if (i != 0) {
			//Increment cluster size for higher levels.
			cluster_size *= 3; // Scaling factor 3 is arbitrary.
		}
		cluster_height = (int)Math::ceil((float)region.size.height / cluster_size);
		cluster_width = (int)Math::ceil((float)region.size.width / cluster_size);

		if (cluster_width <= 1 && cluster_height <= 1) {
			// A cluster_width or cluster_height of 1 means there is only going to be
			// one cluster in this direction. Therefore if both are 1 then this level is useless.
			depth = i;
			break;
		}

		_build_clusters(p_map, i, cluster_size, cluster_width, cluster_height, clusters[i]);
	}
}

void AStarGrid2D::Graph::_create_map_nodes(AStarGrid2D *p_map, HashMap<Vector2i, AStarGrid2D::Point *> &r_nodes) { // Create the node-based representation of the map.
	//1. Create all necessary nodes.
	{
		int begin_x = region.position.x;
		int begin_y = region.position.y;
		int end_x = region.position.x + region.size.x;
		int end_y = region.position.y + region.size.y;

		for (int i = begin_x; i < end_x; i++) {
			for (int j = begin_y; j < end_y; j++) {
				Vector2i id = Vector2i(i, j);

				if (p_map->_is_walkable_unchecked(id.x, id.y)) {
					Point *p = memnew(AStarGrid2D::Point(*p_map->_get_point_unchecked(id.x, id.y)));
					r_nodes[id] = p;
					all_nodes.push_back(p);
				}
			}
		}
	}

	//2. Create all possible edges.
	HashMap<Vector2i, AStarGrid2D::Point *>::Iterator E = r_nodes.begin();
	while (E) {
		AStarGrid2D::Point *n = E->value;

		// Look for straight edges.
		for (int i = -1; i < 2; i += 2) {
			_search_map_edge(p_map, r_nodes, n, n->id.x + i, n->id.y, false);
			_search_map_edge(p_map, r_nodes, n, n->id.x, n->id.y + i, false);
		}
		// Look for diagonal edges.
		for (int i = -1; i < 2; i += 2) {
			for (int j = -1; j < 2; j += 2) {
				_search_map_edge(p_map, r_nodes, n, n->id.x + i, n->id.y + j, true);
			}
		}

		++E;
	}
}

void AStarGrid2D::Graph::_search_map_edge(AStarGrid2D *p_map, HashMap<Vector2i, AStarGrid2D::Point *> &p_nodes, AStarGrid2D::Point *p_n, int p_x, int p_y, bool p_diagonal) {
	Vector2i grid_tile;

	// Don't let diagonal movement occur when an obstacle is crossing the edge.
	if (p_diagonal) {
		grid_tile.x = p_n->id.x;
		grid_tile.y = p_y;
		if (!p_map->_is_walkable(grid_tile.x, grid_tile.y)) {
			return;
		}

		grid_tile.x = p_x;
		grid_tile.y = p_n->id.y;
		if (!p_map->_is_walkable(grid_tile.x, grid_tile.y)) {
			return;
		}
	}

	grid_tile.x = p_x;
	grid_tile.y = p_y;
	if (!p_map->_is_walkable(grid_tile.x, grid_tile.y)) {
		return;
	}

	// Edge is valid, add it to the node.
	Edge *edge = memnew(Edge(p_n, p_nodes[grid_tile], true, p_diagonal ? Math_SQRT2 : 1));
	p_n->edges.push_back(edge);
	all_edges.push_back(edge);
}

bool AStarGrid2D::Graph::_connect_nodes(AStarGrid2D *p_map, Point *p_n1, Point *p_n2, Cluster *p_cluster) { // Connect two nodes by pathfinding between them.
	// We assume they are different nodes. If the path returned is 0, then there is no path that connects them.
	LocalVector<Edge *> path;

	List<Edge *> path2;
	p_map->_solve(p_n1->child, p_n2->child, path2);

	for (Edge *e : path2) {
		path.push_back(e);
	}

	Edge *e1;
	Edge *e2;

	float weight = 0.0f;

	if (path.size() > 0) {
		e1 = memnew(Edge(p_n1, p_n2, false, 0));
		e1->path = path;
		all_edges.push_back(e1);

		e2 = memnew(Edge(p_n2, p_n1, false, 0));
		all_edges.push_back(e2);

		// Store inverse path in node n2.
		// Sum weights of underlying edges at the same time.
		for (uint32_t i = e1->path.size() - 1; i > 0U; i--) {
			Edge *val = nullptr;

			// Find twin edge.

			for (uint32_t j = 0U; j < e1->path[i]->end->edges.size(); j++) {
				Edge *e = e1->path[i]->end->edges[j];

				if (e->start == e1->path[i]->end && e->end == e1->path[i]->start) {
					val = e;
					break;
				}
			}

			if (val != nullptr) {
				weight += val->weight;
				e2->path.push_back(val);
			}
		}

		// Update weights.
		e1->weight = weight;
		e2->weight = weight;

		p_n1->edges.push_back(e1);
		p_n2->edges.push_back(e2);

		return true;
	} else {
		// No path, return false.
		return false;
	}
}

void AStarGrid2D::Graph::_build_clusters(AStarGrid2D *p_map, int p_level, int p_cluster_size, int p_cluster_width, int p_cluster_height, LocalVector<Cluster *> &r_clusters) { // Build Clusters of a certain level, given the size of a cluster.
	Cluster *cluster;

	// Create clusters of this level.
	for (int i = 0; i < p_cluster_height; i++) {
		for (int j = 0; j < p_cluster_width; j++) {
			cluster = memnew(Cluster);
			cluster->bounds.min = Vector2i(j * p_cluster_size, i * p_cluster_size);
			cluster->bounds.max = Vector2i(MIN(cluster->bounds.min.x + p_cluster_size - 1, region.size.width - 1), MIN(cluster->bounds.min.y + p_cluster_size - 1, region.size.height - 1));

			// Adjust size of cluster based on boundaries.
			cluster->width = cluster->bounds.max.x - cluster->bounds.min.x + 1;
			cluster->height = cluster->bounds.max.y - cluster->bounds.min.y + 1;

			if (p_level > 0) {
				// Since we're abstract, we will have lower level clusters.
				// Add lower level clusters in newly created clusters.
				for (uint32_t k = 0; k < clusters[p_level - 1].size(); k++) {
					Cluster *&c = clusters[p_level - 1][k];
					if (cluster->contains(c)) {
						cluster->clusters.push_back(c);
					}
				}
			}

			r_clusters.push_back(cluster);
		}
	}

	bool use_concrete_or_abstract = p_level == 0;

	//Add border nodes for every adjacent pair of clusters.
	for (Cluster *c1 : r_clusters) {
		for (Cluster *c2 : r_clusters) {
			_detect_adjacent_clusters(c1, c2, use_concrete_or_abstract);
		}
	}

	// Add intra edges for every border nodes and pathfind between them.
	for (Cluster *c : r_clusters) {
		_generate_intra_edges(p_map, c);
	}
}

void AStarGrid2D::Graph::_detect_adjacent_clusters(Cluster *p_c1, Cluster *p_c2, bool p_use_concrete_or_abstract) {
	if (p_c1 == p_c2) {
		return;
	}

	// Check if both clusters are adjacent.
	if (p_c1->bounds.min.x == p_c2->bounds.min.x) {
		if (p_c1->bounds.max.y + 1 == p_c2->bounds.min.y) {
			if (p_use_concrete_or_abstract) {
				_create_concrete_border_nodes(p_c1, p_c2, false);
			} else {
				_create_abstract_border_nodes(p_c1, p_c2, false);
			}
		} else if (p_c2->bounds.max.y + 1 == p_c1->bounds.min.y) {
			if (p_use_concrete_or_abstract) {
				_create_concrete_border_nodes(p_c2, p_c1, false);
			} else {
				_create_abstract_border_nodes(p_c2, p_c1, false);
			}
		}
	} else if (p_c1->bounds.min.y == p_c2->bounds.min.y) {
		if (p_c1->bounds.max.x + 1 == p_c2->bounds.min.x) {
			if (p_use_concrete_or_abstract) {
				_create_concrete_border_nodes(p_c1, p_c2, true);
			} else {
				_create_abstract_border_nodes(p_c1, p_c2, true);
			}
		} else if (p_c2->bounds.max.x + 1 == p_c1->bounds.min.x) {
			if (p_use_concrete_or_abstract) {
				_create_concrete_border_nodes(p_c2, p_c1, true);
			} else {
				_create_abstract_border_nodes(p_c2, p_c1, true);
			}
		}
	}
}

void AStarGrid2D::Graph::_create_concrete_border_nodes(Cluster *p_c1, Cluster *p_c2, bool p_x) {
	/// Create border nodes and attach them together.
	/// We always pass the lower cluster first (in p_c1).
	/// Adjacent index : if p_x == true, then p_c1.BottomRight.x else p_c1.BottomRight.y.

	int i = 0;
	int i_min = 0;
	int i_max = 0;
	if (p_x) {
		i_min = p_c1->bounds.min.y;
		i_max = i_min + p_c1->height;
	} else {
		i_min = p_c1->bounds.min.x;
		i_max = i_min + p_c1->width;
	}
	int line_size = 0;
	for (i = i_min; i < i_max; i++) {
		bool b1 = p_x && (nodes.has(Vector2i(p_c1->bounds.max.x, i)) && nodes.has(Vector2i(p_c2->bounds.min.x, i)));
		bool b2 = !p_x && (nodes.has(Vector2i(i, p_c1->bounds.max.y)) && nodes.has(Vector2i(i, p_c2->bounds.min.y)));
		if (b1 || b2) {
			line_size++;
		} else {
			_create_concrete_inter_edges(p_c1, p_c2, p_x, line_size, i);
		}
	}
	// If line size > 0 after looping, then we have another line to fill in.
	_create_concrete_inter_edges(p_c1, p_c2, p_x, line_size, i);
}

void AStarGrid2D::Graph::_create_concrete_inter_edges(Cluster *p_c1, Cluster *p_c2, bool p_x, int &r_line_size, int p_i) { // i is the index at which we stopped (either its an obstacle or the end of the cluster.
	if (r_line_size > 0) {
		if (r_line_size <= 5) {
			// Line is too small, create 1 inter edges.
			_create_concrete_inter_edge(p_c1, p_c2, p_x, p_i - (r_line_size / 2 + 1));
		} else {
			// Create 2 inter edges.
			_create_concrete_inter_edge(p_c1, p_c2, p_x, p_i - r_line_size);
			_create_concrete_inter_edge(p_c1, p_c2, p_x, p_i - 1);
		}
		r_line_size = 0;
	}
}

void AStarGrid2D::Graph::_create_concrete_inter_edge(Cluster *p_c1, Cluster *p_c2, bool p_x, int p_i) { // Inter edges are edges that crosses clusters.
	Vector2i g1;
	Vector2i g2;
	Point *n1;
	Point *n2;

	if (p_x) {
		g1 = Vector2i(p_c1->bounds.max.x, p_i);
		g2 = Vector2i(p_c2->bounds.min.x, p_i);
	} else {
		g1 = Vector2i(p_i, p_c1->bounds.max.y);
		g2 = Vector2i(p_i, p_c2->bounds.min.y);
	}

	if (!p_c1->nodes.has(g1)) {
		n1 = memnew(Point(g1));
		n1->child = nodes[g1];

		p_c1->nodes.insert(g1, n1);
	} else {
		n1 = p_c1->nodes[g1];
	}

	if (!p_c2->nodes.has(g2)) {
		n2 = memnew(Point(g2));
		n2->child = nodes[g2];

		p_c2->nodes.insert(g2, n2);
	} else {
		n2 = p_c2->nodes[g2];
	}

	n1->edges.push_back(memnew(Edge(n1, n2, true, 1.0)));
	n2->edges.push_back(memnew(Edge(n2, n1, true, 1.0)));
}

void AStarGrid2D::Graph::_create_abstract_border_nodes(Cluster *p_p1, Cluster *p_p2, bool p_x) {
	for (Cluster *c1 : p_p1->clusters) {
		for (Cluster *c2 : p_p2->clusters) {
			if ((p_x && c1->bounds.min.y == c2->bounds.min.y && c1->bounds.max.x + 1 == c2->bounds.min.x) ||
					(!p_x && c1->bounds.min.x == c2->bounds.min.x && c1->bounds.max.y + 1 == c2->bounds.min.y)) {
				_create_abstract_inter_edges(p_p1, p_p2, c1, c2);
			}
		}
	}
}

void AStarGrid2D::Graph::_create_abstract_inter_edges(Cluster *p_p1, Cluster *p_p2, Cluster *p_c1, Cluster *p_c2) {
	LocalVector<Edge *> edges1;
	LocalVector<Edge *> edges2;
	Point *n1;
	Point *n2;

	// Add edges that connects them from c1.
	for (KeyValue<Vector2i, Point *> &n : p_c1->nodes) {
		for (Edge *e : n.value->edges) {
			if (e->is_inter && p_c2->contains(e->end->id)) {
				edges1.push_back(e);
			}
		}
	}

	for (KeyValue<Vector2i, Point *> &n : p_c2->nodes) {
		for (Edge *e : n.value->edges) {
			if (e->is_inter && p_c1->contains(e->end->id)) {
				edges2.push_back(e);
			}
		}
	}

	// Find every pair of twin edges and insert them in their respective parents.
	for (Edge *e : edges1) {
		for (Edge *e2 : edges2) {
			if (e->end == e2->start) {
				if (!p_p1->nodes.has(e->start->id)) {
					n1 = memnew(Point(e->start->id, e->start->pos));
					n1->child = e->start;
					p_p1->nodes.insert(n1->id, n1);

					all_nodes.push_back(n1);
				} else {
					n1 = p_p1->nodes[e2->start->id];
				}

				if (!p_p2->nodes.has(e2->start->id)) {
					n2 = memnew(Point(e2->start->id, e2->start->pos));
					n2->child = e2->start;
					p_p2->nodes.insert(n2->id, n2);

					all_nodes.push_back(n2);
				} else {
					n2 = p_p2->nodes[e2->start->id];
				}

				e = memnew(Edge(n1, n2, true, 1));
				e2 = memnew(Edge(n2, n1, true, 1));

				n1->edges.push_back(e);
				n2->edges.push_back(e2);

				all_edges.push_back(e);
				all_edges.push_back(e2);

				break; // Break the second loop since we've found a pair.
			}
		}
	}
}

void AStarGrid2D::Graph::_generate_intra_edges(AStarGrid2D *p_map, Cluster *p_cluster) { // Intra edges are edges that lives inside clusters.
	/* We do this so that we can iterate through pairs once,
	 * by keeping the second index always higher than the first */
	LocalVector<Point *> local_nodes;

	HashMap<Vector2i, AStarGrid2D::Point *>::Iterator E = p_cluster->nodes.begin();
	while (E) {
		local_nodes.push_back(E->value);
		++E;
	}

	for (Point *n : local_nodes) {
		for (Point *n2 : local_nodes) {
			if (n != n2) {
				_connect_nodes(p_map, n, n2, p_cluster);
			}
		}
	}
}

AStarGrid2D::Point *AStarGrid2D::Graph::_connect_to_border(AStarGrid2D *p_map, const Vector2i &p_pos, Cluster *p_c, Point *p_child) {
	if (p_c->nodes.has(p_pos)) {
		return p_c->nodes[p_pos];
	}

	Point *new_node = memnew(Point(p_pos));
	new_node->child = p_child;

	HashMap<Vector2i, AStarGrid2D::Point *>::Iterator E = p_c->nodes.begin();
	while (E) {
		_connect_nodes(p_map, new_node, E->value, p_c);
		++E;
	}

	added_nodes.push_back(new_node);
	return new_node;
}

void AStarGrid2D::Graph::insert_nodes(AStarGrid2D *p_map, const Vector2i &p_start, const Vector2i &p_dest, Point *&r_start, Point *&r_dest) {
	for (Point *n : temp_nodes) {
		memdelete(n);
	}
	temp_nodes.clear();
	for (Point *n : added_nodes) {
		memdelete(n);
	}
	added_nodes.clear();

	Point *new_start;
	Point *new_dest;
	r_start = nodes[p_start];
	r_dest = nodes[p_dest];

	for (int i = 0; i < depth; i++) {
		Cluster *c_start = nullptr;
		Cluster *c_dest = nullptr;
		bool is_connected = false;

		for (uint32_t j = 0U; j < clusters[i].size(); j++) {
			Cluster *c = clusters[i][j];

			if (c->contains(p_start)) {
				c_start = c;
			}
			if (c->contains(p_dest)) {
				c_dest = c;
			}
			if (c_start != nullptr && c_dest != nullptr) {
				break;
			}
		}

		if (c_start == c_dest) {
			new_start = memnew(Point(p_start));
			new_start->child = r_start;
			temp_nodes.push_back(new_start);

			new_dest = memnew(Point(p_dest));
			new_dest->child = r_dest;
			temp_nodes.push_back(new_dest);

			is_connected = _connect_nodes(p_map, r_start, r_dest, c_start);

			if (is_connected) {
				r_start = new_start;
				r_dest = new_dest;
			}
		}

		if (!is_connected) {
			r_start = _connect_to_border(p_map, p_start, c_start, r_start);
			r_dest = _connect_to_border(p_map, p_dest, c_dest, r_dest);
		}
	}
}

void AStarGrid2D::Graph::remove_added_nodes() { // Remove added nodes from the graph, including all underlying edges.
	for (Point *n : added_nodes) {
		List<Edge *> to_clean;

		// Find an edge in current.end that points to this node.
		for (uint32_t j = 0U; j < n->edges.size(); j++) {
			Edge *e = n->edges[j];
			if (e->end == n) {
				to_clean.push_back(e);
			}
		}
		for (Edge *e : to_clean) {
			memdelete(e);
			n->edges.erase(e);
		}
	}
	added_nodes.clear();
}
