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

class AStarGrid2D : public RefCounted {
	GDCLASS(AStarGrid2D, RefCounted);
	friend class AStarGrid2D::Graph;

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

	bool hpa_dirty = false;
	bool hpa_enabled = false;
	int hpa_max_level = 1;
	int hpa_cluster_size = 10;

	struct Edge;

	struct Point {
		Vector2i id;

		bool solid = false;
		Vector2 pos;
		real_t weight_scale = 1.0;

		LocalVector<Edge *> edges;
		Point *child = nullptr;

		// Used for pathfinding.
		Point *prev_point = nullptr;
		real_t g_score = 0;
		real_t f_score = 0;
		uint64_t open_pass = 0;
		uint64_t closed_pass = 0;

		Point() {}

		Point(const Vector2i &p_id, const Vector2 &p_pos = Vector2()) :
				id(p_id), pos(p_pos) {}

		Point(const Point &p_other) :
				id(p_other.id), solid(p_other.solid), pos(p_other.pos), weight_scale(p_other.weight_scale) {
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
	Point *end = nullptr;

	uint64_t pass = 1;

	// Hierarchial Pathfinding classes.

	// This implementation of Hierarchial Pathfinding is based
	// on the code from https://github.com/hugoscurti/hierarchical-pathfinding (made by hugoscurti (hugoscurti@gmail.com), MIT licensed),
	// which is based on the paper https://webdocs.cs.ualberta.ca/~mmueller/ps/hpastar.pdf.

	struct Bounds {
		Vector2i min;
		Vector2i max;
	};

	struct Cluster {
		Bounds bounds;
		HashMap<Vector2i, Point *> nodes;
		LocalVector<Cluster *> clusters;
		int width;
		int height;

		_FORCE_INLINE_ bool contains(const Cluster *p_other) {
			return p_other->bounds.min.x >= bounds.min.x && p_other->bounds.min.y >= bounds.min.y &&
					p_other->bounds.max.x <= bounds.max.x && p_other->bounds.max.y <= bounds.max.y;
		}
		_FORCE_INLINE_ bool contains(const Vector2i &p_pos) {
			return p_pos.x >= bounds.min.x && p_pos.x <= bounds.max.x && p_pos.y >= bounds.min.y && p_pos.y <= bounds.max.y;
		}

		Cluster() {}
	};

	struct Edge {
		Point *start;
		Point *end;
		bool is_inter = false;
		real_t weight = 0;
		LocalVector<Edge *> path;

		Edge() {}

		Edge(Point *p_start, Point *p_end, bool p_is_inter, real_t p_weight) :
				start(p_start), end(p_end), is_inter(p_is_inter), weight(p_weight) {
		}
	};

	class Graph {
		int depth;
		Rect2i region;

		HashMap<Vector2i, Point *> nodes;
		LocalVector<LocalVector<Cluster *>> clusters;
		LocalVector<Point *> added_nodes;

		LocalVector<Point *> all_nodes;
		LocalVector<Edge *> all_edges;
		LocalVector<Cluster *> all_clusters;
		LocalVector<Point *> temp_nodes;

	private:
		void _clear(); // Clear internal array memory.
		void _create_map_nodes(AStarGrid2D *p_map, HashMap<Vector2i, Point *> &r_nodes); // Create the node-based representation of the map.
		void _search_map_edge(AStarGrid2D *p_map, HashMap<Vector2i, Point *> &p_nodes, Point *p_n, int p_x, int p_y, bool p_diagonal);
		bool _connect_nodes(AStarGrid2D *p_map, Point *p_n1, Point *p_n2, Cluster *p_cluster);
		void _build_clusters(AStarGrid2D *p_map, int p_level, int p_cluster_size, int p_cluster_width, int p_cluster_height, LocalVector<Cluster *> &r_clusters);
		void _detect_adjacent_clusters(Cluster *p_cluster1, Cluster *p_cluster2, bool p_use_concrete_or_abstract);
		void _create_concrete_border_nodes(Cluster *p_cluster1, Cluster *p_cluster2, bool p_x);
		void _create_concrete_inter_edges(Cluster *p_cluster1, Cluster *p_cluster2, bool p_x, int &r_line_size, int p_i);
		void _create_concrete_inter_edge(Cluster *p_cluster1, Cluster *p_cluster2, bool p_x, int p_i);
		void _create_abstract_border_nodes(Cluster *p_cluster1, Cluster *p_cluster2, bool p_x);
		void _create_abstract_inter_edges(Cluster *p_p1, Cluster *p_p2, Cluster *p_c1, Cluster *p_c2);
		void _generate_intra_edges(AStarGrid2D *p_map, Cluster *p_cluster); // Intra edges are edges that lives inside clusters.
		Point *_connect_to_border(AStarGrid2D *p_map, const Vector2i &p_pos, Cluster *p_c, Point *child);

	public:
		void build(AStarGrid2D *p_map); // Construct a graph from the map.
		void insert_nodes(AStarGrid2D *p_map, const Vector2i &p_start, const Vector2i &p_dest, Point *&r_start, Point *&r_dest);
		void remove_added_nodes(); // Remove added nodes from the graph, including all underlying edges.

		Graph();
		~Graph();
	} *hpa_graph = nullptr;

private: // Internal routines.
	_FORCE_INLINE_ bool _is_walkable(int32_t p_x, int32_t p_y) const {
		if (region.has_point(Vector2i(p_x, p_y))) {
			return !points[p_y - region.position.y][p_x - region.position.x].solid;
		}
		return false;
	}

	_FORCE_INLINE_ bool _is_walkable_unchecked(int32_t p_x, int32_t p_y) const {
		return !points[p_y][p_x].solid;
	}

	_FORCE_INLINE_ Point *_get_point(int32_t p_x, int32_t p_y) {
		if (region.has_point(Vector2i(p_x, p_y))) {
			return &points[p_y - region.position.y][p_x - region.position.x];
		}
		return nullptr;
	}

	_FORCE_INLINE_ Point *_get_point_unchecked(int32_t p_x, int32_t p_y) {
		return &points[p_y - region.position.y][p_x - region.position.x];
	}

	_FORCE_INLINE_ Point *_get_point_unchecked(const Vector2i &p_id) {
		return &points[p_id.y - region.position.y][p_id.x - region.position.x];
	}

	_FORCE_INLINE_ const Point *_get_point_unchecked(const Vector2i &p_id) const {
		return &points[p_id.y - region.position.y][p_id.x - region.position.x];
	}

	void _get_nbors(Point *p_point, LocalVector<Point *> &r_nbors);
	Point *_jump(Point *p_from, Point *p_to);
	bool _solve(Point *p_begin_point, Point *p_end_point);
	void _solve(Point *p_begin_point, Point *p_end_point, List<Edge *> &r_edges);
	bool _hpa_is_valid(const Vector2i &p_grid_size, int p_max_levels, int p_cluster_size) const;

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
	void update_hpa();

	bool is_in_bounds(int32_t p_x, int32_t p_y) const;
	bool is_in_boundsv(const Vector2i &p_id) const;
	bool is_dirty() const;
	bool is_hpa_dirty() const;

	void set_hpa_enabled(bool p_enabled);
	bool is_hpa_enabled() const;

	void set_max_level(int p_max_level);
	int get_max_level() const;

	void set_cluster_size(int p_cluster_size);
	int get_cluster_size() const;

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

	void clear();

	Vector2 get_point_position(const Vector2i &p_id) const;
	Vector<Vector2> get_point_path(const Vector2i &p_from, const Vector2i &p_to);
	TypedArray<Vector2i> get_id_path(const Vector2i &p_from, const Vector2i &p_to);

	AStarGrid2D();
	virtual ~AStarGrid2D();
};

VARIANT_ENUM_CAST(AStarGrid2D::DiagonalMode);
VARIANT_ENUM_CAST(AStarGrid2D::Heuristic);

#endif // A_STAR_GRID_2D_H
