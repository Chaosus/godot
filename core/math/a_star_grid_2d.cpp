/*************************************************************************/
/*  a_star_grid_2d.cpp                                                   */
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

#include "a_star_grid_2d.h"

Vector2i AStarGrid2D::get_size() const {
	return size;
}

Vector2 AStarGrid2D::get_offset() const {
	return offset;
}

Vector2 AStarGrid2D::get_spacing() const {
	return spacing;
}

bool AStarGrid2D::is_use_four_dir() const {
	return use_four_dir;
}

void AStarGrid2D::setup(const Vector2i &p_size, const Vector2 &p_offset, const Vector2 &p_spacing, bool p_use_four_dir) {
	clear();

	size = p_size;
	offset = p_offset;
	spacing = p_spacing;
	use_four_dir = p_use_four_dir;

	for (int y = 0, id = 0; y < p_size.y; y++) {
		for (int x = 0; x < p_size.x; x++) {
			Point *pt = memnew(Point);
			pt->id = id++;
			pt->pos = offset + Vector2(x, y) * spacing;
			pt->weight_scale = 1;
			pt->prev_point = nullptr;
			pt->open_pass = 0;
			pt->closed_pass = 0;
			pt->enabled = true;
			points.set(pt->id, pt);
		}
	}

	for (int y = 0, id = 0; y < p_size.y; y++) {
		for (int x = 0; x < p_size.x; x++) {
			Point *pt;
			points.lookup(id, pt);

			Point *pt2;
			int pid = 0;

			bool has_left = x - 1 >= 0;
			bool has_right = x + 1 < size.x;
			bool has_top = y - 1 >= 0;
			bool has_bottom = y + 1 < size.y;

			if (has_left) {
				pid = get_point_id(Vector2i(x - 1, y));
				points.lookup(pid, pt2);
				pt->neighbours.insert(pid, pt2);
			}
			if (has_right) {
				pid = get_point_id(Vector2i(x + 1, y));
				points.lookup(pid, pt2);
				pt->neighbours.insert(pid, pt2);
			}
			if (has_top) {
				pid = get_point_id(Vector2i(x, y - 1));
				points.lookup(pid, pt2);
				pt->neighbours.insert(pid, pt2);
			}
			if (has_bottom) {
				pid = get_point_id(Vector2i(x, y + 1));
				points.lookup(pid, pt2);
				pt->neighbours.insert(pid, pt2);
			}
			if (!p_use_four_dir) {
				if (has_left && has_top) {
					pid = get_point_id(Vector2i(x - 1, y - 1));
					points.lookup(pid, pt2);
					pt->neighbours.insert(pid, pt2);
				}
				if (has_right && has_top) {
					pid = get_point_id(Vector2i(x + 1, y - 1));
					points.lookup(pid, pt2);
					pt->neighbours.insert(pid, pt2);
				}
				if (has_left && has_bottom) {
					pid = get_point_id(Vector2i(x - 1, y + 1));
					points.lookup(pid, pt2);
					pt->neighbours.insert(pid, pt2);
				}
				if (has_right && has_bottom) {
					pid = get_point_id(Vector2i(x + 1, y + 1));
					points.lookup(pid, pt2);
					pt->neighbours.insert(pid, pt2);
				}
			}
			id++;
		}
	}
}

void AStarGrid2D::clear() {
	last_free_id = 0;
	for (OAHashMap<int, Point *>::Iterator it = points.iter(); it.valid; it = points.next_iter(it)) {
		memdelete(*(it.value));
	}
	points.clear();
}

int AStarGrid2D::get_point_count() const {
	return points.get_num_elements();
}

int AStarGrid2D::get_point_id(const Vector2i &p_position) {
	ERR_FAIL_COND_V_MSG(p_position.x < 0 || p_position.x > size.x || p_position.y < 0 || p_position.y > size.y, -1, "Can't get position index. Position out of range.");
	return p_position.y * size.x + p_position.x;
}

real_t AStarGrid2D::get_point_weight_scale(int p_id) const {
	Point *p;
	bool p_exists = points.lookup(p_id, p);
	ERR_FAIL_COND_V_MSG(!p_exists, 0, vformat("Can't get point's weight scale. Point with id: %d doesn't exist.", p_id));

	return p->weight_scale;
}

void AStarGrid2D::set_point_weight_scale(int p_id, real_t p_weight_scale) {
	Point *p;
	bool p_exists = points.lookup(p_id, p);
	ERR_FAIL_COND_MSG(!p_exists, vformat("Can't set point's weight scale. Point with id: %d doesn't exist.", p_id));
	ERR_FAIL_COND_MSG(p_weight_scale < 0.0, vformat("Can't set point's weight scale less than 0.0: %f.", p_weight_scale));

	p->weight_scale = p_weight_scale;
}

void AStarGrid2D::set_point_disabled(int p_id, bool p_disabled) {
	Point *p;
	bool p_exists = points.lookup(p_id, p);
	ERR_FAIL_COND_MSG(!p_exists, vformat("Can't set if point is disabled. Point with id: %d doesn't exist.", p_id));

	p->enabled = !p_disabled;
}

bool AStarGrid2D::is_point_disabled(int p_id) const {
	Point *p;
	bool p_exists = points.lookup(p_id, p);
	ERR_FAIL_COND_V_MSG(!p_exists, false, vformat("Can't get if point is disabled. Point with id: %d doesn't exist.", p_id));

	return !p->enabled;
}

Vector2 AStarGrid2D::get_point_position(int p_id) const {
	Point *p;
	bool p_exists = points.lookup(p_id, p);
	ERR_FAIL_COND_V_MSG(!p_exists, Vector2(), vformat("Can't get point's position. Point with id: %d doesn't exist.", p_id));

	return p->pos;
}

bool AStarGrid2D::_solve(Point *p_begin_point, Point *p_end_point) {
	pass++;

	if (!p_end_point->enabled) {
		return false;
	}

	bool found_route = false;

	Vector<Point *> open_list;
	SortArray<Point *, SortPoints> sorter;

	p_begin_point->g_score = 0;
	p_begin_point->f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	open_list.push_back(p_begin_point);

	while (!open_list.is_empty()) {
		Point *p = open_list[0]; // The currently processed point

		if (p == p_end_point) {
			found_route = true;
			break;
		}

		sorter.pop_heap(0, open_list.size(), open_list.ptrw()); // Remove the current point from the open list
		open_list.remove_at(open_list.size() - 1);
		p->closed_pass = pass; // Mark the point as closed

		for (OAHashMap<int, Point *>::Iterator it = p->neighbours.iter(); it.valid; it = p->neighbours.next_iter(it)) {
			Point *e = *(it.value); // The neighbour point

			if (!e->enabled || e->closed_pass == pass) {
				continue;
			}

			real_t tentative_g_score = p->g_score + _compute_cost(p->id, e->id) * e->weight_scale;

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
				sorter.push_heap(0, open_list.size() - 1, 0, e, open_list.ptrw());
			} else {
				sorter.push_heap(0, open_list.find(e), 0, e, open_list.ptrw());
			}
		}
	}

	return found_route;
}

real_t AStarGrid2D::_estimate_cost(int p_from_id, int p_to_id) {
	real_t scost;
	if (GDVIRTUAL_CALL(_estimate_cost, p_from_id, p_to_id, scost)) {
		return scost;
	}

	Point *from_point;
	bool from_exists = points.lookup(p_from_id, from_point);
	ERR_FAIL_COND_V_MSG(!from_exists, 0, vformat("Can't estimate cost. Point with id: %d doesn't exist.", p_from_id));

	Point *to_point;
	bool to_exists = points.lookup(p_to_id, to_point);
	ERR_FAIL_COND_V_MSG(!to_exists, 0, vformat("Can't estimate cost. Point with id: %d doesn't exist.", p_to_id));

	return from_point->pos.distance_to(to_point->pos);
}

real_t AStarGrid2D::_compute_cost(int p_from_id, int p_to_id) {
	real_t scost;
	if (GDVIRTUAL_CALL(_compute_cost, p_from_id, p_to_id, scost)) {
		return scost;
	}

	Point *from_point;
	bool from_exists = points.lookup(p_from_id, from_point);
	ERR_FAIL_COND_V_MSG(!from_exists, 0, vformat("Can't compute cost. Point with id: %d doesn't exist.", p_from_id));

	Point *to_point;
	bool to_exists = points.lookup(p_to_id, to_point);
	ERR_FAIL_COND_V_MSG(!to_exists, 0, vformat("Can't compute cost. Point with id: %d doesn't exist.", p_to_id));

	return from_point->pos.distance_to(to_point->pos);
}

Vector<Vector2> AStarGrid2D::get_point_path(int p_from_id, int p_to_id) {
	Point *a;
	bool from_exists = points.lookup(p_from_id, a);
	ERR_FAIL_COND_V_MSG(!from_exists, Vector<Vector2>(), vformat("Can't get point path. Point with id: %d doesn't exist.", p_from_id));

	Point *b;
	bool to_exists = points.lookup(p_to_id, b);
	ERR_FAIL_COND_V_MSG(!to_exists, Vector<Vector2>(), vformat("Can't get point path. Point with id: %d doesn't exist.", p_to_id));

	if (a == b) {
		Vector<Vector2> ret;
		ret.push_back(a->pos);
		return ret;
	}

	Point *begin_point = a;
	Point *end_point = b;

	bool found_route = _solve(begin_point, end_point);
	if (!found_route) {
		return Vector<Vector2>();
	}

	Point *p = end_point;
	int pc = 1; // Begin point
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<Vector2> path;
	path.resize(pc);

	{
		Vector2 *w = path.ptrw();

		Point *p2 = end_point;
		int idx = pc - 1;
		while (p2 != begin_point) {
			w[idx--] = p2->pos;
			p2 = p2->prev_point;
		}

		w[0] = p2->pos; // Assign first
	}

	return path;
}

Vector<int> AStarGrid2D::get_id_path(int p_from_id, int p_to_id) {
	Point *a;
	bool from_exists = points.lookup(p_from_id, a);
	ERR_FAIL_COND_V_MSG(!from_exists, Vector<int>(), vformat("Can't get id path. Point with id: %d doesn't exist.", p_from_id));

	Point *b;
	bool to_exists = points.lookup(p_to_id, b);
	ERR_FAIL_COND_V_MSG(!to_exists, Vector<int>(), vformat("Can't get id path. Point with id: %d doesn't exist.", p_to_id));

	if (a == b) {
		Vector<int> ret;
		ret.push_back(a->id);
		return ret;
	}

	Point *begin_point = a;
	Point *end_point = b;

	bool found_route = _solve(begin_point, end_point);
	if (!found_route) {
		return Vector<int>();
	}

	Point *p = end_point;
	int pc = 1; // Begin point
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<int> path;
	path.resize(pc);

	{
		int *w = path.ptrw();

		p = end_point;
		int idx = pc - 1;
		while (p != begin_point) {
			w[idx--] = p->id;
			p = p->prev_point;
		}

		w[0] = p->id; // Assign first
	}

	return path;
}

void AStarGrid2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_size"), &AStarGrid2D::get_size);
	ClassDB::bind_method(D_METHOD("get_offset"), &AStarGrid2D::get_offset);
	ClassDB::bind_method(D_METHOD("get_spacing"), &AStarGrid2D::get_spacing);
	ClassDB::bind_method(D_METHOD("is_use_four_dir"), &AStarGrid2D::is_use_four_dir);

	ClassDB::bind_method(D_METHOD("get_point_weight_scale", "id"), &AStarGrid2D::get_point_weight_scale);
	ClassDB::bind_method(D_METHOD("set_point_weight_scale", "id", "weight_scale"), &AStarGrid2D::set_point_weight_scale);

	ClassDB::bind_method(D_METHOD("set_point_disabled", "id", "disabled"), &AStarGrid2D::set_point_disabled, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("is_point_disabled", "id"), &AStarGrid2D::is_point_disabled);

	ClassDB::bind_method(D_METHOD("get_point_position", "id"), &AStarGrid2D::get_point_position);

	ClassDB::bind_method(D_METHOD("get_point_count"), &AStarGrid2D::get_point_count);
	ClassDB::bind_method(D_METHOD("get_point_id", "position"), &AStarGrid2D::get_point_id);

	ClassDB::bind_method(D_METHOD("setup", "size", "offset", "spacing", "weight_scale", "use_four_dir"), &AStarGrid2D::setup, DEFVAL(Vector2()), DEFVAL(1.0f), DEFVAL(1.0f), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("clear"), &AStarGrid2D::clear);

	GDVIRTUAL_BIND(_estimate_cost, "from_id", "to_id")
	GDVIRTUAL_BIND(_compute_cost, "from_id", "to_id")
}

AStarGrid2D::~AStarGrid2D() {
	clear();
}
