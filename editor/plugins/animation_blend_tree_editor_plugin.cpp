/**************************************************************************/
/*  animation_blend_tree_editor_plugin.cpp                                */
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

#include "animation_blend_tree_editor_plugin.h"

#include "core/config/project_settings.h"
#include "core/doc_data.h"
#include "core/input/input.h"
#include "core/io/resource_loader.h"
#include "core/os/keyboard.h"
#include "editor/editor_help.h"
#include "editor/editor_inspector.h"
#include "editor/editor_node.h"
#include "editor/editor_scale.h"
#include "editor/editor_settings.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_file_dialog.h"
#include "scene/animation/animation_player.h"
#include "scene/gui/check_box.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/panel.h"
#include "scene/gui/progress_bar.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/separator.h"
#include "scene/gui/view_panner.h"
#include "scene/main/window.h"
#include "scene/resources/style_box_flat.h"

void AnimationNodeBlendTreeEditor::add_custom_type(const String &p_name, const Ref<Script> &p_script) {
	for (int i = 0; i < add_options.size(); i++) {
		ERR_FAIL_COND(add_options[i].script == p_script);
	}

	AddOption ao;
	ao.name = p_name;
	ao.script = p_script;
	ao.is_custom = true;
	add_options.push_back(ao);

	_update_options_menu();
}

void AnimationNodeBlendTreeEditor::remove_custom_type(const Ref<Script> &p_script) {
	for (int i = 0; i < add_options.size(); i++) {
		if (add_options[i].script == p_script) {
			add_options.remove_at(i);
			return;
		}
	}

	_update_options_menu();
}

void AnimationNodeBlendTreeEditor::_update_options_menu(bool p_has_input_ports) {
	node_desc->set_text("");
	members_dialog->get_ok_button()->set_disabled(true);

	members->clear();
	TreeItem *root = members->create_item();

	String filter = node_filter->get_text().strip_edges();
	bool use_filter = !filter.is_empty();

	bool is_first_item = true;

	Color unsupported_color = get_theme_color(SNAME("error_color"), EditorStringName(Editor));
	Color supported_color = get_theme_color(SNAME("warning_color"), EditorStringName(Editor));

	HashMap<String, TreeItem *> folders;

	Vector<AddOption> custom_options;
	Vector<AddOption> embedded_options;

	for (int i = 0; i < add_options.size(); i++) {
		if (!use_filter || add_options[i].name.findn(filter) != -1) {
			const_cast<AddOption &>(add_options[i]).temp_idx = i;
			embedded_options.push_back(add_options[i]);
		}
	}

	Vector<AddOption> options;
	SortArray<AddOption, _OptionComparator> sorter;
	sorter.sort(custom_options.ptrw(), custom_options.size());

	options.append_array(custom_options);
	options.append_array(embedded_options);

	for (int i = 0; i < options.size(); i++) {
		String path = options[i].is_custom ? "Addons" : "";
		TreeItem *category = nullptr;

		if (!path.is_empty() && !folders.has(path)) {
			Vector<String> subfolders = path.split("/");
			category = root;
			String path_temp = "";
			for (int j = 0; j < subfolders.size(); j++) {
				path_temp += subfolders[j];
				if (!folders.has(path_temp)) {
					category = members->create_item(category);
					category->set_selectable(0, false);
					category->set_collapsed(!use_filter);
					category->set_text(0, subfolders[j]);
					folders.insert(path_temp, category);
				} else {
					category = folders[path_temp];
				}
			}
		} else {
			category = folders[path];
		}

		TreeItem *item = members->create_item(category);
		item->set_text(0, options[i].name);

		if (is_first_item && use_filter) {
			item->select(0);
			node_desc->set_text(options[i].description);
			is_first_item = false;
		}
		item->set_meta("id", options[i].temp_idx);
	}
}

Size2 AnimationNodeBlendTreeEditor::get_minimum_size() const {
	return Size2(10, 200);
}

void AnimationNodeBlendTreeEditor::_property_changed(const StringName &p_property, const Variant &p_value, const String &p_field, bool p_changing) {
	AnimationTree *tree = AnimationTreeEditor::get_singleton()->get_animation_tree();
	if (!tree) {
		return;
	}
	updating = true;
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(vformat(TTR("Parameter Changed: %s"), p_property), UndoRedo::MERGE_ENDS);
	undo_redo->add_do_property(tree, p_property, p_value);
	undo_redo->add_undo_property(tree, p_property, tree->get(p_property));
	undo_redo->add_do_method(this, "update_graph");
	undo_redo->add_undo_method(this, "update_graph");
	undo_redo->commit_action();
	updating = false;
}

void AnimationNodeBlendTreeEditor::update_graph() {
	if (updating || blend_tree.is_null()) {
		return;
	}

	AnimationTree *tree = AnimationTreeEditor::get_singleton()->get_animation_tree();
	if (!tree) {
		return;
	}

	visible_properties.clear();

	graph->set_scroll_offset(blend_tree->get_graph_offset() * EDSCALE);

	graph->clear_connections();
	//erase all nodes
	for (int i = 0; i < graph->get_child_count(); i++) {
		if (Object::cast_to<GraphNode>(graph->get_child(i))) {
			memdelete(graph->get_child(i));
			i--;
		}
	}

	animations.clear();

	List<StringName> nodes;
	blend_tree->get_node_list(&nodes);

	for (const StringName &E : nodes) {
		GraphNode *node = memnew(GraphNode);
		graph->add_child(node);

		node->set_draggable(!read_only);

		Ref<AnimationNode> agnode = blend_tree->get_node(E);
		ERR_CONTINUE(!agnode.is_valid());

		node->set_position_offset(blend_tree->get_node_position(E) * EDSCALE);

		node->set_title(agnode->get_caption());
		node->set_name(E);

		int base = 0;
		if (String(E) != "output") {
			LineEdit *name = memnew(LineEdit);
			name->set_text(E);
			name->set_editable(!read_only);
			name->set_expand_to_text_length_enabled(true);
			node->add_child(name);
			node->set_slot(0, false, 0, Color(), true, read_only ? -1 : 0, get_theme_color(SNAME("font_color"), SNAME("Label")));
			name->connect("text_submitted", callable_mp(this, &AnimationNodeBlendTreeEditor::_node_renamed).bind(agnode), CONNECT_DEFERRED);
			name->connect("focus_exited", callable_mp(this, &AnimationNodeBlendTreeEditor::_node_renamed_focus_out).bind(agnode), CONNECT_DEFERRED);
			name->connect("text_changed", callable_mp(this, &AnimationNodeBlendTreeEditor::_node_rename_lineedit_changed), CONNECT_DEFERRED);
			base = 1;
			agnode->set_closable(true);

			if (!read_only) {
				Button *delete_button = memnew(Button);
				delete_button->set_flat(true);
				delete_button->set_focus_mode(FOCUS_NONE);
				delete_button->set_icon(get_editor_theme_icon(SNAME("Close")));
				delete_button->connect("pressed", callable_mp(this, &AnimationNodeBlendTreeEditor::_delete_node_request).bind(E), CONNECT_DEFERRED);
				node->get_titlebar_hbox()->add_child(delete_button);
			}
		}

		for (int i = 0; i < agnode->get_input_count(); i++) {
			Label *in_name = memnew(Label);
			node->add_child(in_name);
			in_name->set_text(agnode->get_input_name(i));
			node->set_slot(base + i, true, read_only ? -1 : 0, get_theme_color(SNAME("font_color"), SNAME("Label")), false, 0, Color());
		}

		List<PropertyInfo> pinfo;
		agnode->get_parameter_list(&pinfo);
		for (const PropertyInfo &F : pinfo) {
			if (!(F.usage & PROPERTY_USAGE_EDITOR)) {
				continue;
			}
			String base_path = AnimationTreeEditor::get_singleton()->get_base_path() + String(E) + "/" + F.name;
			EditorProperty *prop = EditorInspector::instantiate_property_editor(tree, F.type, base_path, F.hint, F.hint_string, F.usage);
			if (prop) {
				prop->set_read_only(read_only || (F.usage & PROPERTY_USAGE_READ_ONLY));
				prop->set_object_and_property(tree, base_path);
				prop->update_property();
				prop->set_name_split_ratio(0);
				prop->connect("property_changed", callable_mp(this, &AnimationNodeBlendTreeEditor::_property_changed));
				node->add_child(prop);
				visible_properties.push_back(prop);
			}
		}

		node->connect("dragged", callable_mp(this, &AnimationNodeBlendTreeEditor::_node_dragged).bind(E));

		if (AnimationTreeEditor::get_singleton()->can_edit(agnode)) {
			node->add_child(memnew(HSeparator));
			Button *open_in_editor = memnew(Button);
			open_in_editor->set_text(TTR("Open Editor"));
			open_in_editor->set_icon(get_editor_theme_icon(SNAME("Edit")));
			node->add_child(open_in_editor);
			open_in_editor->connect("pressed", callable_mp(this, &AnimationNodeBlendTreeEditor::_open_in_editor).bind(E), CONNECT_DEFERRED);
			open_in_editor->set_h_size_flags(SIZE_SHRINK_CENTER);
		}

		if (agnode->has_filter()) {
			node->add_child(memnew(HSeparator));
			Button *inspect_filters = memnew(Button);
			if (read_only) {
				inspect_filters->set_text(TTR("Inspect Filters"));
			} else {
				inspect_filters->set_text(TTR("Edit Filters"));
			}
			inspect_filters->set_icon(get_editor_theme_icon(SNAME("AnimationFilter")));
			node->add_child(inspect_filters);
			inspect_filters->connect("pressed", callable_mp(this, &AnimationNodeBlendTreeEditor::_inspect_filters).bind(E), CONNECT_DEFERRED);
			inspect_filters->set_h_size_flags(SIZE_SHRINK_CENTER);
		}

		Ref<AnimationNodeAnimation> anim = agnode;
		if (anim.is_valid()) {
			MenuButton *mb = memnew(MenuButton);
			mb->set_text(anim->get_animation());
			mb->set_icon(get_editor_theme_icon(SNAME("Animation")));
			mb->set_disabled(read_only);
			Array options;

			node->add_child(memnew(HSeparator));
			node->add_child(mb);

			ProgressBar *pb = memnew(ProgressBar);

			List<StringName> anims;
			tree->get_animation_list(&anims);

			for (const StringName &F : anims) {
				mb->get_popup()->add_item(F);
				options.push_back(F);
			}

			if (tree->has_animation(anim->get_animation())) {
				pb->set_max(tree->get_animation(anim->get_animation())->get_length());
			}

			pb->set_show_percentage(false);
			pb->set_custom_minimum_size(Vector2(0, 14) * EDSCALE);
			animations[E] = pb;
			node->add_child(pb);

			mb->get_popup()->connect("index_pressed", callable_mp(this, &AnimationNodeBlendTreeEditor::_anim_selected).bind(options, E), CONNECT_DEFERRED);
		}

		// TODO: Avoid using strings, expose a method on GraphNode instead.
		Ref<StyleBoxFlat> sb = node->get_theme_stylebox(SNAME("panel"));
		Color c = sb->get_border_color();
		Color mono_color = ((c.r + c.g + c.b) / 3) < 0.7 ? Color(1.0, 1.0, 1.0) : Color(0.0, 0.0, 0.0);
		mono_color.a = 0.85;
		c = mono_color;

		node->add_theme_color_override("title_color", c);
		c.a = 0.7;
		node->add_theme_color_override("close_color", c);
		node->add_theme_color_override("resizer_color", c);
	}

	List<AnimationNodeBlendTree::NodeConnection> node_connections;
	blend_tree->get_node_connections(&node_connections);

	for (const AnimationNodeBlendTree::NodeConnection &E : node_connections) {
		StringName from = E.output_node;
		StringName to = E.input_node;
		int to_idx = E.input_index;

		graph->connect_node(from, 0, to, to_idx);
	}

	float graph_minimap_opacity = EDITOR_GET("editors/visual_editors/minimap_opacity");
	graph->set_minimap_opacity(graph_minimap_opacity);
	float graph_lines_curvature = EDITOR_GET("editors/visual_editors/lines_curvature");
	graph->set_connection_lines_curvature(graph_lines_curvature);
}

void AnimationNodeBlendTreeEditor::_file_opened(const String &p_file) {
	file_loaded = ResourceLoader::load(p_file);
	if (file_loaded.is_valid()) {
		_add_node(MENU_LOAD_FILE_CONFIRM);
	} else {
		EditorNode::get_singleton()->show_warning(TTR("This type of node can't be used. Only animation nodes are allowed."));
	}
}

void AnimationNodeBlendTreeEditor::_add_node(int p_idx) {
	Ref<AnimationNode> anode;

	String base_name;

	if (p_idx == MENU_LOAD_FILE) {
		open_file->clear_filters();
		List<String> ext_filters;
		ResourceLoader::get_recognized_extensions_for_type("AnimationNode", &ext_filters);
		for (const String &E : ext_filters) {
			open_file->add_filter("*." + E);
		}
		open_file->popup_file_dialog();
		return;
	} else if (p_idx == MENU_LOAD_FILE_CONFIRM) {
		anode = file_loaded;
		file_loaded.unref();
		base_name = anode->get_class();
	} else if (p_idx == MENU_PASTE) {
		anode = EditorSettings::get_singleton()->get_resource_clipboard();
		ERR_FAIL_COND(!anode.is_valid());
		base_name = anode->get_class();
	} else if (!add_options[p_idx].type.is_empty()) {
		AnimationNode *an = Object::cast_to<AnimationNode>(ClassDB::instantiate(add_options[p_idx].type));
		ERR_FAIL_NULL(an);
		anode = Ref<AnimationNode>(an);
		base_name = add_options[p_idx].name;
	} else {
		ERR_FAIL_COND(add_options[p_idx].script.is_null());
		StringName base_type = add_options[p_idx].script->get_instance_base_type();
		AnimationNode *an = Object::cast_to<AnimationNode>(ClassDB::instantiate(base_type));
		ERR_FAIL_NULL(an);
		anode = Ref<AnimationNode>(an);
		anode->set_script(add_options[p_idx].script);
		base_name = add_options[p_idx].name;
	}

	Ref<AnimationNodeOutput> out = anode;
	if (out.is_valid()) {
		EditorNode::get_singleton()->show_warning(TTR("Output node can't be added to the blend tree."));
		return;
	}

	if (!from_node.is_empty() && anode->get_input_count() == 0) {
		from_node = "";
		return;
	}

	Point2 instance_pos = graph->get_scroll_offset();
	if (use_position_from_popup_menu) {
		instance_pos += position_from_popup_menu;
	} else {
		instance_pos += graph->get_size() * 0.5;
	}

	instance_pos /= graph->get_zoom();

	int base = 1;
	String name = base_name;
	while (blend_tree->has_node(name)) {
		base++;
		name = base_name + " " + itos(base);
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Add Node to BlendTree"));
	undo_redo->add_do_method(blend_tree.ptr(), "add_node", name, anode, instance_pos / EDSCALE);
	undo_redo->add_undo_method(blend_tree.ptr(), "remove_node", name);

	if (!from_node.is_empty()) {
		undo_redo->add_do_method(blend_tree.ptr(), "connect_node", name, 0, from_node);
		from_node = "";
	}
	if (!to_node.is_empty() && to_slot != -1) {
		undo_redo->add_do_method(blend_tree.ptr(), "connect_node", to_node, to_slot, name);
		to_node = "";
		to_slot = -1;
	}

	undo_redo->add_do_method(this, "update_graph");
	undo_redo->add_undo_method(this, "update_graph");
	undo_redo->commit_action();
}

void AnimationNodeBlendTreeEditor::_sbox_input(const Ref<InputEvent> &p_ie) {
	Ref<InputEventKey> ie = p_ie;
	if (ie.is_valid() && (ie->get_keycode() == Key::UP || ie->get_keycode() == Key::DOWN || ie->get_keycode() == Key::ENTER || ie->get_keycode() == Key::KP_ENTER)) {
		members->gui_input(ie);
		node_filter->accept_event();
	}
}

void AnimationNodeBlendTreeEditor::_member_create() {
	TreeItem *item = members->get_selected();
	if (item != nullptr && item->has_meta("id")) {
		int idx = members->get_selected()->get_meta("id");
		_add_node(idx);
		members_dialog->hide();
	}
}

void AnimationNodeBlendTreeEditor::_member_selected() {
	TreeItem *item = members->get_selected();

	if (item != nullptr && item->has_meta("id")) {
		members_dialog->get_ok_button()->set_disabled(false);
		node_desc->set_text(add_options[item->get_meta("id")].description);
	} else {
		members_dialog->get_ok_button()->set_disabled(true);
		node_desc->set_text("");
	}
}

void AnimationNodeBlendTreeEditor::_member_unselected() {
}

void AnimationNodeBlendTreeEditor::_member_cancel() {}

void AnimationNodeBlendTreeEditor::_member_filter_changed(const String &p_text) {
	_update_options_menu();
}

void AnimationNodeBlendTreeEditor::_show_members_dialog(bool at_mouse_pos, bool p_has_input_ports) {
	_update_options_menu(p_has_input_ports);

	if (at_mouse_pos) {
		saved_node_pos_dirty = true;
		saved_node_pos = graph->get_local_mouse_position();

		Point2 gpos = get_screen_position() + get_local_mouse_position();
		members_dialog->set_position(gpos);
	} else {
		saved_node_pos_dirty = false;
		members_dialog->set_position(graph->get_screen_position() + Point2(5 * EDSCALE, 65 * EDSCALE));
	}

	if (members_dialog->is_visible()) {
		members_dialog->grab_focus();
		return;
	}

	members_dialog->popup();

	// Keep dialog within window bounds.
	Rect2 window_rect = Rect2(get_window()->get_position(), get_window()->get_size());
	Rect2 dialog_rect = Rect2(members_dialog->get_position(), members_dialog->get_size());
	Vector2 difference = (dialog_rect.get_end() - window_rect.get_end()).max(Vector2());
	members_dialog->set_position(members_dialog->get_position() - difference);

	node_filter->call_deferred(SNAME("grab_focus")); // Still not visible.
	node_filter->select_all();
}

String AnimationNodeBlendTreeEditor::_get_brief_class_description(const String &p_class_name) const {
	HashMap<String, DocData::ClassDoc>::ConstIterator E = EditorHelp::get_doc_data()->class_list.find(p_class_name);
	if (E) {
		return E->value.brief_description.strip_edges();
	}
	return "";
}

void AnimationNodeBlendTreeEditor::_popup(bool p_has_input_ports, const Vector2 &p_node_position) {
	_update_options_menu(p_has_input_ports);
	members_dialog->set_position(graph->get_screen_position() + p_node_position);

	if (members_dialog->is_visible()) {
		members_dialog->grab_focus();
		return;
	}
	members_dialog->popup();
}

void AnimationNodeBlendTreeEditor::_popup_request(const Vector2 &p_position) {
	if (read_only) {
		return;
	}

	_popup(false, p_position);
}

void AnimationNodeBlendTreeEditor::_connection_to_empty(const String &p_from, int p_from_slot, const Vector2 &p_release_position) {
	if (read_only) {
		return;
	}

	Ref<AnimationNode> node = blend_tree->get_node(p_from);
	if (node.is_valid()) {
		from_node = p_from;
		_popup(true, p_release_position);
	}
}

void AnimationNodeBlendTreeEditor::_connection_from_empty(const String &p_to, int p_to_slot, const Vector2 &p_release_position) {
	if (read_only) {
		return;
	}

	Ref<AnimationNode> node = blend_tree->get_node(p_to);
	if (node.is_valid()) {
		to_node = p_to;
		to_slot = p_to_slot;
		_popup(false, p_release_position);
	}
}

void AnimationNodeBlendTreeEditor::_popup_hide() {
	to_node = "";
	to_slot = -1;
}

void AnimationNodeBlendTreeEditor::_node_dragged(const Vector2 &p_from, const Vector2 &p_to, const StringName &p_which) {
	updating = true;
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Node Moved"));
	undo_redo->add_do_method(blend_tree.ptr(), "set_node_position", p_which, p_to / EDSCALE);
	undo_redo->add_undo_method(blend_tree.ptr(), "set_node_position", p_which, p_from / EDSCALE);
	undo_redo->add_do_method(this, "update_graph");
	undo_redo->add_undo_method(this, "update_graph");
	undo_redo->commit_action();
	updating = false;
}

void AnimationNodeBlendTreeEditor::_connection_request(const String &p_from, int p_from_index, const String &p_to, int p_to_index) {
	if (read_only) {
		return;
	}

	AnimationNodeBlendTree::ConnectionError err = blend_tree->can_connect_node(p_to, p_to_index, p_from);

	if (err == AnimationNodeBlendTree::CONNECTION_ERROR_CONNECTION_EXISTS) {
		blend_tree->disconnect_node(p_to, p_to_index);
		err = blend_tree->can_connect_node(p_to, p_to_index, p_from);
	}

	if (err != AnimationNodeBlendTree::CONNECTION_OK) {
		EditorNode::get_singleton()->show_warning(TTR("Unable to connect, port may be in use or connection may be invalid."));
		return;
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Nodes Connected"));
	undo_redo->add_do_method(blend_tree.ptr(), "connect_node", p_to, p_to_index, p_from);
	undo_redo->add_undo_method(blend_tree.ptr(), "disconnect_node", p_to, p_to_index);
	undo_redo->add_do_method(this, "update_graph");
	undo_redo->add_undo_method(this, "update_graph");
	undo_redo->commit_action();
}

void AnimationNodeBlendTreeEditor::_disconnection_request(const String &p_from, int p_from_index, const String &p_to, int p_to_index) {
	if (read_only) {
		return;
	}

	graph->disconnect_node(p_from, p_from_index, p_to, p_to_index);

	updating = true;
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Nodes Disconnected"));
	undo_redo->add_do_method(blend_tree.ptr(), "disconnect_node", p_to, p_to_index);
	undo_redo->add_undo_method(blend_tree.ptr(), "connect_node", p_to, p_to_index, p_from);
	undo_redo->add_do_method(this, "update_graph");
	undo_redo->add_undo_method(this, "update_graph");
	undo_redo->commit_action();
	updating = false;
}

void AnimationNodeBlendTreeEditor::_anim_selected(int p_index, Array p_options, const String &p_node) {
	String option = p_options[p_index];

	Ref<AnimationNodeAnimation> anim = blend_tree->get_node(p_node);
	ERR_FAIL_COND(!anim.is_valid());

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Set Animation"));
	undo_redo->add_do_method(anim.ptr(), "set_animation", option);
	undo_redo->add_undo_method(anim.ptr(), "set_animation", anim->get_animation());
	undo_redo->add_do_method(this, "update_graph");
	undo_redo->add_undo_method(this, "update_graph");
	undo_redo->commit_action();
}

void AnimationNodeBlendTreeEditor::_delete_node_request(const String &p_which) {
	if (read_only) {
		return;
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Delete Node"));
	undo_redo->add_do_method(blend_tree.ptr(), "remove_node", p_which);
	undo_redo->add_undo_method(blend_tree.ptr(), "add_node", p_which, blend_tree->get_node(p_which), blend_tree.ptr()->get_node_position(p_which));

	List<AnimationNodeBlendTree::NodeConnection> conns;
	blend_tree->get_node_connections(&conns);

	for (const AnimationNodeBlendTree::NodeConnection &E : conns) {
		if (E.output_node == p_which || E.input_node == p_which) {
			undo_redo->add_undo_method(blend_tree.ptr(), "connect_node", E.input_node, E.input_index, E.output_node);
		}
	}

	undo_redo->add_do_method(this, "update_graph");
	undo_redo->add_undo_method(this, "update_graph");
	undo_redo->commit_action();
}

void AnimationNodeBlendTreeEditor::_delete_nodes_request(const TypedArray<StringName> &p_nodes) {
	if (read_only) {
		return;
	}

	List<StringName> to_erase;

	if (p_nodes.is_empty()) {
		for (int i = 0; i < graph->get_child_count(); i++) {
			GraphNode *gn = Object::cast_to<GraphNode>(graph->get_child(i));
			if (gn && gn->is_selected()) {
				Ref<AnimationNode> anode = blend_tree->get_node(gn->get_name());
				if (anode->is_closable()) {
					to_erase.push_back(gn->get_name());
				}
			}
		}
	} else {
		for (int i = 0; i < p_nodes.size(); i++) {
			Ref<AnimationNode> anode = blend_tree->get_node(p_nodes[i]);
			if (anode->is_closable()) {
				to_erase.push_back(p_nodes[i]);
			}
		}
	}

	if (to_erase.is_empty()) {
		return;
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Delete Node(s)"));

	for (const StringName &F : to_erase) {
		_delete_node_request(F);
	}

	undo_redo->commit_action();
}

void AnimationNodeBlendTreeEditor::_node_selected(Object *p_node) {
	if (read_only) {
		return;
	}

	GraphNode *gn = Object::cast_to<GraphNode>(p_node);
	ERR_FAIL_NULL(gn);

	String name = gn->get_name();

	Ref<AnimationNode> anode = blend_tree->get_node(name);
	ERR_FAIL_COND(!anode.is_valid());

	EditorNode::get_singleton()->push_item(anode.ptr(), "", true);
}

void AnimationNodeBlendTreeEditor::_open_in_editor(const String &p_which) {
	Ref<AnimationNode> an = blend_tree->get_node(p_which);
	ERR_FAIL_COND(!an.is_valid());
	AnimationTreeEditor::get_singleton()->enter_editor(p_which);
}

void AnimationNodeBlendTreeEditor::_filter_toggled() {
	updating = true;
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Toggle Filter On/Off"));
	undo_redo->add_do_method(_filter_edit.ptr(), "set_filter_enabled", filter_enabled->is_pressed());
	undo_redo->add_undo_method(_filter_edit.ptr(), "set_filter_enabled", _filter_edit->is_filter_enabled());
	undo_redo->add_do_method(this, "_update_filters", _filter_edit);
	undo_redo->add_undo_method(this, "_update_filters", _filter_edit);
	undo_redo->commit_action();
	updating = false;
}

void AnimationNodeBlendTreeEditor::_filter_edited() {
	TreeItem *edited = filters->get_edited();
	ERR_FAIL_NULL(edited);

	NodePath edited_path = edited->get_metadata(0);
	bool filtered = edited->is_checked(0);

	updating = true;
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Change Filter"));
	undo_redo->add_do_method(_filter_edit.ptr(), "set_filter_path", edited_path, filtered);
	undo_redo->add_undo_method(_filter_edit.ptr(), "set_filter_path", edited_path, _filter_edit->is_path_filtered(edited_path));
	undo_redo->add_do_method(this, "_update_filters", _filter_edit);
	undo_redo->add_undo_method(this, "_update_filters", _filter_edit);
	undo_redo->commit_action();
	updating = false;
}

bool AnimationNodeBlendTreeEditor::_update_filters(const Ref<AnimationNode> &anode) {
	if (updating || _filter_edit != anode) {
		return false;
	}

	AnimationTree *tree = AnimationTreeEditor::get_singleton()->get_animation_tree();
	if (!tree) {
		return false;
	}

	Node *base = tree->get_node(tree->get_root_node());
	if (!base) {
		EditorNode::get_singleton()->show_warning(TTR("Animation player has no valid root node path, so unable to retrieve track names."));
		return false;
	}

	updating = true;

	HashSet<String> paths;
	HashMap<String, RBSet<String>> types;
	{
		List<StringName> animation_list;
		tree->get_animation_list(&animation_list);

		for (const StringName &E : animation_list) {
			Ref<Animation> anim = tree->get_animation(E);
			for (int i = 0; i < anim->get_track_count(); i++) {
				String track_path = anim->track_get_path(i);
				paths.insert(track_path);

				String track_type_name;
				Animation::TrackType track_type = anim->track_get_type(i);
				switch (track_type) {
					case Animation::TrackType::TYPE_ANIMATION: {
						track_type_name = TTR("Anim Clips");
					} break;
					case Animation::TrackType::TYPE_AUDIO: {
						track_type_name = TTR("Audio Clips");
					} break;
					case Animation::TrackType::TYPE_METHOD: {
						track_type_name = TTR("Functions");
					} break;
					default: {
					} break;
				}
				if (!track_type_name.is_empty()) {
					types[track_path].insert(track_type_name);
				}
			}
		}
	}

	filter_enabled->set_pressed(anode->is_filter_enabled());
	filters->clear();
	TreeItem *root = filters->create_item();

	HashMap<String, TreeItem *> parenthood;

	for (const String &E : paths) {
		NodePath path = E;
		TreeItem *ti = nullptr;
		String accum;
		for (int i = 0; i < path.get_name_count(); i++) {
			String name = path.get_name(i);
			if (!accum.is_empty()) {
				accum += "/";
			}
			accum += name;
			if (!parenthood.has(accum)) {
				if (ti) {
					ti = filters->create_item(ti);
				} else {
					ti = filters->create_item(root);
				}
				parenthood[accum] = ti;
				ti->set_text(0, name);
				ti->set_selectable(0, false);
				ti->set_editable(0, false);

				if (base->has_node(accum)) {
					Node *node = base->get_node(accum);
					ti->set_icon(0, EditorNode::get_singleton()->get_object_icon(node, "Node"));
				}

			} else {
				ti = parenthood[accum];
			}
		}

		Node *node = nullptr;
		if (base->has_node(accum)) {
			node = base->get_node(accum);
		}
		if (!node) {
			continue; //no node, can't edit
		}

		if (path.get_subname_count()) {
			String concat = path.get_concatenated_subnames();

			Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(node);
			if (skeleton && skeleton->find_bone(concat) != -1) {
				//path in skeleton
				const String &bone = concat;
				int idx = skeleton->find_bone(bone);
				List<String> bone_path;
				while (idx != -1) {
					bone_path.push_front(skeleton->get_bone_name(idx));
					idx = skeleton->get_bone_parent(idx);
				}

				accum += ":";
				for (List<String>::Element *F = bone_path.front(); F; F = F->next()) {
					if (F != bone_path.front()) {
						accum += "/";
					}

					accum += F->get();
					if (!parenthood.has(accum)) {
						ti = filters->create_item(ti);
						parenthood[accum] = ti;
						ti->set_text(0, F->get());
						ti->set_selectable(0, false);
						ti->set_editable(0, false);
						ti->set_icon(0, get_editor_theme_icon(SNAME("BoneAttachment3D")));
					} else {
						ti = parenthood[accum];
					}
				}

				ti->set_editable(0, !read_only);
				ti->set_selectable(0, true);
				ti->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
				ti->set_text(0, concat);
				ti->set_checked(0, anode->is_path_filtered(path));
				ti->set_icon(0, get_editor_theme_icon(SNAME("BoneAttachment3D")));
				ti->set_metadata(0, path);

			} else {
				//just a property
				ti = filters->create_item(ti);
				ti->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
				ti->set_text(0, concat);
				ti->set_editable(0, !read_only);
				ti->set_selectable(0, true);
				ti->set_checked(0, anode->is_path_filtered(path));
				ti->set_metadata(0, path);
			}
		} else {
			if (ti) {
				//just a node, not a property track
				String types_text = "[";
				if (types.has(path)) {
					RBSet<String>::Iterator F = types[path].begin();
					types_text += *F;
					while (F) {
						types_text += " / " + *F;
						;
						++F;
					}
				}
				types_text += "]";
				ti = filters->create_item(ti);
				ti->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
				ti->set_text(0, types_text);
				ti->set_editable(0, !read_only);
				ti->set_selectable(0, true);
				ti->set_checked(0, anode->is_path_filtered(path));
				ti->set_metadata(0, path);
			}
		}
	}

	updating = false;

	return true;
}

void AnimationNodeBlendTreeEditor::_inspect_filters(const String &p_which) {
	if (read_only) {
		filter_dialog->set_title(TTR("Inspect Filtered Tracks:"));
	} else {
		filter_dialog->set_title(TTR("Edit Filtered Tracks:"));
	}

	filter_enabled->set_disabled(read_only);

	Ref<AnimationNode> anode = blend_tree->get_node(p_which);
	ERR_FAIL_COND(!anode.is_valid());

	_filter_edit = anode;
	if (!_update_filters(anode)) {
		return;
	}

	filter_dialog->popup_centered(Size2(500, 500) * EDSCALE);
}

void AnimationNodeBlendTreeEditor::_update_editor_settings() {
	graph->get_panner()->setup((ViewPanner::ControlScheme)EDITOR_GET("editors/panning/sub_editors_panning_scheme").operator int(), ED_GET_SHORTCUT("canvas_item_editor/pan_view"), bool(EDITOR_GET("editors/panning/simple_panning")));
	graph->set_warped_panning(bool(EDITOR_GET("editors/panning/warped_mouse_panning")));
}

void AnimationNodeBlendTreeEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_update_editor_settings();

			add_options.clear();
			add_options.push_back(AddOption("Animation", "AnimationNodeAnimation", _get_brief_class_description("AnimationNodeAnimation")));
			add_options.push_back(AddOption("OneShot", "AnimationNodeOneShot", _get_brief_class_description("AnimationNodeOneShot"), 2));
			add_options.push_back(AddOption("Add2", "AnimationNodeAdd2", _get_brief_class_description("AnimationNodeAdd2"), 2));
			add_options.push_back(AddOption("Add3", "AnimationNodeAdd3", _get_brief_class_description("AnimationNodeAdd3"), 3));
			add_options.push_back(AddOption("Blend2", "AnimationNodeBlend2", _get_brief_class_description("AnimationNodeBlend2"), 2));
			add_options.push_back(AddOption("Blend3", "AnimationNodeBlend3", _get_brief_class_description("AnimationNodeBlend3"), 3));
			add_options.push_back(AddOption("Sub2", "AnimationNodeSub2", _get_brief_class_description("AnimationNodeSub2"), 2));
			add_options.push_back(AddOption("TimeSeek", "AnimationNodeTimeSeek", _get_brief_class_description("AnimationNodeTimeSeek"), 1));
			add_options.push_back(AddOption("TimeScale", "AnimationNodeTimeScale", _get_brief_class_description("AnimationNodeTimeScale"), 1));
			add_options.push_back(AddOption("Transition", "AnimationNodeTransition", _get_brief_class_description("AnimationNodeTransition")));
			add_options.push_back(AddOption("BlendTree", "AnimationNodeBlendTree", _get_brief_class_description("AnimationNodeBlendTree")));
			add_options.push_back(AddOption("BlendSpace1D", "AnimationNodeBlendSpace1D", _get_brief_class_description("AnimationNodeBlendSpace1D")));
			add_options.push_back(AddOption("BlendSpace2D", "AnimationNodeBlendSpace2D", _get_brief_class_description("AnimationNodeBlendSpace2D")));
			add_options.push_back(AddOption("StateMachine", "AnimationNodeStateMachine", _get_brief_class_description("AnimationNodeStateMachine")));

			_update_options_menu();
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			_update_editor_settings();
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			error_panel->add_theme_style_override("panel", get_theme_stylebox(SNAME("panel"), SNAME("Tree")));
			error_label->add_theme_color_override("font_color", get_theme_color(SNAME("error_color"), EditorStringName(Editor)));

			if (is_visible_in_tree()) {
				update_graph();
			}
		} break;

		case NOTIFICATION_PROCESS: {
			AnimationTree *tree = AnimationTreeEditor::get_singleton()->get_animation_tree();
			if (!tree) {
				return; // Node has been changed.
			}

			String error;

			if (!tree->is_active()) {
				error = TTR("AnimationTree is inactive.\nActivate to enable playback, check node warnings if activation fails.");
			} else if (tree->is_state_invalid()) {
				error = tree->get_invalid_state_reason();
			}

			if (error != error_label->get_text()) {
				error_label->set_text(error);
				if (!error.is_empty()) {
					error_panel->show();
				} else {
					error_panel->hide();
				}
			}

			List<AnimationNodeBlendTree::NodeConnection> conns;
			blend_tree->get_node_connections(&conns);
			for (const AnimationNodeBlendTree::NodeConnection &E : conns) {
				float activity = 0;
				StringName path = AnimationTreeEditor::get_singleton()->get_base_path() + E.input_node;
				if (!tree->is_state_invalid()) {
					activity = tree->get_connection_activity(path, E.input_index);
				}
				graph->set_connection_activity(E.output_node, 0, E.input_node, E.input_index, activity);
			}

			for (const KeyValue<StringName, ProgressBar *> &E : animations) {
				Ref<AnimationNodeAnimation> an = blend_tree->get_node(E.key);
				if (an.is_valid()) {
					if (tree->has_animation(an->get_animation())) {
						Ref<Animation> anim = tree->get_animation(an->get_animation());
						if (anim.is_valid()) {
							E.value->set_max(anim->get_length());
							//StringName path = AnimationTreeEditor::get_singleton()->get_base_path() + E.input_node;
							StringName time_path = AnimationTreeEditor::get_singleton()->get_base_path() + String(E.key) + "/time";
							E.value->set_value(tree->get(time_path));
						}
					}
				}
			}

			for (int i = 0; i < visible_properties.size(); i++) {
				visible_properties[i]->update_property();
			}
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			set_process(is_visible_in_tree());
		} break;
	}
}

void AnimationNodeBlendTreeEditor::_scroll_changed(const Vector2 &p_scroll) {
	if (read_only) {
		return;
	}

	if (updating) {
		return;
	}
	updating = true;
	blend_tree->set_graph_offset(p_scroll / EDSCALE);
	updating = false;
}

void AnimationNodeBlendTreeEditor::_bind_methods() {
	ClassDB::bind_method("update_graph", &AnimationNodeBlendTreeEditor::update_graph);
	ClassDB::bind_method("_update_filters", &AnimationNodeBlendTreeEditor::_update_filters);
}

AnimationNodeBlendTreeEditor *AnimationNodeBlendTreeEditor::singleton = nullptr;

// AnimationNode's "node_changed" signal means almost update_input.
void AnimationNodeBlendTreeEditor::_node_changed(const StringName &p_node_name) {
	// TODO:
	// Here is executed during the commit of EditorNode::undo_redo, it is not possible to create an undo_redo action here.
	// The disconnect when the number of enabled inputs decreases is done in AnimationNodeBlendTree and update_graph().
	// This means that there is no place to register undo_redo actions.
	// In order to implement undo_redo correctly, we may need to implement AnimationNodeEdit such as AnimationTrackKeyEdit
	// and add it to _node_selected() with EditorNode::get_singleton()->push_item(AnimationNodeEdit).
	update_graph();
}

void AnimationNodeBlendTreeEditor::_node_renamed(const String &p_text, Ref<AnimationNode> p_node) {
	if (blend_tree.is_null()) {
		return;
	}

	AnimationTree *tree = AnimationTreeEditor::get_singleton()->get_animation_tree();
	if (!tree) {
		return;
	}

	String prev_name = blend_tree->get_node_name(p_node);
	ERR_FAIL_COND(prev_name.is_empty());
	GraphNode *gn = Object::cast_to<GraphNode>(graph->get_node(prev_name));
	ERR_FAIL_NULL(gn);

	const String &new_name = p_text;

	ERR_FAIL_COND(new_name.is_empty() || new_name.contains(".") || new_name.contains("/"));

	if (new_name == prev_name) {
		return; //nothing to do
	}

	const String &base_name = new_name;
	int base = 1;
	String name = base_name;
	while (blend_tree->has_node(name)) {
		base++;
		name = base_name + " " + itos(base);
	}

	String base_path = AnimationTreeEditor::get_singleton()->get_base_path();

	updating = true;
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Node Renamed"));
	undo_redo->add_do_method(blend_tree.ptr(), "rename_node", prev_name, name);
	undo_redo->add_undo_method(blend_tree.ptr(), "rename_node", name, prev_name);
	undo_redo->add_do_method(this, "update_graph");
	undo_redo->add_undo_method(this, "update_graph");
	undo_redo->commit_action();
	updating = false;
	gn->set_name(new_name);
	gn->set_size(gn->get_minimum_size());

	//change editors accordingly
	for (int i = 0; i < visible_properties.size(); i++) {
		String pname = visible_properties[i]->get_edited_property().operator String();
		if (pname.begins_with(base_path + prev_name)) {
			String new_name2 = pname.replace_first(base_path + prev_name, base_path + name);
			visible_properties[i]->set_object_and_property(visible_properties[i]->get_edited_object(), new_name2);
		}
	}

	//recreate connections
	graph->clear_connections();

	List<AnimationNodeBlendTree::NodeConnection> node_connections;
	blend_tree->get_node_connections(&node_connections);

	for (const AnimationNodeBlendTree::NodeConnection &E : node_connections) {
		StringName from = E.output_node;
		StringName to = E.input_node;
		int to_idx = E.input_index;

		graph->connect_node(from, 0, to, to_idx);
	}

	//update animations
	for (const KeyValue<StringName, ProgressBar *> &E : animations) {
		if (E.key == prev_name) {
			animations[new_name] = animations[prev_name];
			animations.erase(prev_name);
			break;
		}
	}

	update_graph(); // Needed to update the signal connections with the new name.
	current_node_rename_text = String();
}

void AnimationNodeBlendTreeEditor::_node_renamed_focus_out(Ref<AnimationNode> p_node) {
	if (current_node_rename_text.is_empty()) {
		return; // The text_submitted signal triggered the graph update and freed the LineEdit.
	}
	_node_renamed(current_node_rename_text, p_node);
}

void AnimationNodeBlendTreeEditor::_node_rename_lineedit_changed(const String &p_text) {
	current_node_rename_text = p_text;
}

bool AnimationNodeBlendTreeEditor::can_edit(const Ref<AnimationNode> &p_node) {
	Ref<AnimationNodeBlendTree> bt = p_node;
	return bt.is_valid();
}

void AnimationNodeBlendTreeEditor::edit(const Ref<AnimationNode> &p_node) {
	if (blend_tree.is_valid()) {
		blend_tree->disconnect("node_changed", callable_mp(this, &AnimationNodeBlendTreeEditor::_node_changed));
	}

	blend_tree = p_node;

	read_only = false;

	if (blend_tree.is_null()) {
		hide();
	} else {
		read_only = EditorNode::get_singleton()->is_resource_read_only(blend_tree);

		blend_tree->connect("node_changed", callable_mp(this, &AnimationNodeBlendTreeEditor::_node_changed));

		update_graph();
	}

	add_node->set_disabled(read_only);
	graph->set_show_arrange_button(!read_only);
}

AnimationNodeBlendTreeEditor::AnimationNodeBlendTreeEditor() {
	singleton = this;
	updating = false;

	graph = memnew(GraphEdit);
	add_child(graph);
	graph->add_valid_right_disconnect_type(0);
	graph->add_valid_left_disconnect_type(0);
	graph->set_v_size_flags(SIZE_EXPAND_FILL);
	graph->connect("connection_request", callable_mp(this, &AnimationNodeBlendTreeEditor::_connection_request), CONNECT_DEFERRED);
	graph->connect("disconnection_request", callable_mp(this, &AnimationNodeBlendTreeEditor::_disconnection_request), CONNECT_DEFERRED);
	graph->connect("node_selected", callable_mp(this, &AnimationNodeBlendTreeEditor::_node_selected));
	graph->connect("scroll_offset_changed", callable_mp(this, &AnimationNodeBlendTreeEditor::_scroll_changed));
	graph->connect("delete_nodes_request", callable_mp(this, &AnimationNodeBlendTreeEditor::_delete_nodes_request));
	graph->connect("popup_request", callable_mp(this, &AnimationNodeBlendTreeEditor::_popup_request));
	graph->connect("connection_to_empty", callable_mp(this, &AnimationNodeBlendTreeEditor::_connection_to_empty));
	graph->connect("connection_from_empty", callable_mp(this, &AnimationNodeBlendTreeEditor::_connection_from_empty));
	float graph_minimap_opacity = EDITOR_GET("editors/visual_editors/minimap_opacity");
	graph->set_minimap_opacity(graph_minimap_opacity);
	float graph_lines_curvature = EDITOR_GET("editors/visual_editors/lines_curvature");
	graph->set_connection_lines_curvature(graph_lines_curvature);

	VBoxContainer *members_vb = memnew(VBoxContainer);
	members_vb->set_v_size_flags(SIZE_EXPAND_FILL);

	HBoxContainer *filter_hb = memnew(HBoxContainer);
	members_vb->add_child(filter_hb);

	node_filter = memnew(LineEdit);
	filter_hb->add_child(node_filter);
	node_filter->connect("text_changed", callable_mp(this, &AnimationNodeBlendTreeEditor::_member_filter_changed));
	node_filter->connect("gui_input", callable_mp(this, &AnimationNodeBlendTreeEditor::_sbox_input));
	node_filter->set_h_size_flags(SIZE_EXPAND_FILL);
	node_filter->set_placeholder(TTR("Search"));

	members = memnew(Tree);
	members_vb->add_child(members);
	//SET_DRAG_FORWARDING_GCD(members, AnimationNodeBlendTreeEditor);
	members->set_h_size_flags(SIZE_EXPAND_FILL);
	members->set_v_size_flags(SIZE_EXPAND_FILL);
	members->set_hide_root(true);
	members->set_allow_reselect(true);
	members->set_hide_folding(false);
	members->set_custom_minimum_size(Size2(180 * EDSCALE, 200 * EDSCALE));
	members->connect("item_activated", callable_mp(this, &AnimationNodeBlendTreeEditor::_member_create));
	members->connect("item_selected", callable_mp(this, &AnimationNodeBlendTreeEditor::_member_selected));
	members->connect("nothing_selected", callable_mp(this, &AnimationNodeBlendTreeEditor::_member_unselected));

	HBoxContainer *desc_hbox = memnew(HBoxContainer);
	members_vb->add_child(desc_hbox);

	Label *desc_label = memnew(Label);
	desc_hbox->add_child(desc_label);
	desc_label->set_text(TTR("Description:"));

	desc_hbox->add_spacer();

	node_desc = memnew(RichTextLabel);
	members_vb->add_child(node_desc);
	node_desc->set_h_size_flags(SIZE_EXPAND_FILL);
	node_desc->set_v_size_flags(SIZE_FILL);
	node_desc->set_custom_minimum_size(Size2(0, 70 * EDSCALE));

	members_dialog = memnew(ConfirmationDialog);
	members_dialog->set_title(TTR("Create Animation Blend Tree Node"));
	members_dialog->set_exclusive(true);
	members_dialog->add_child(members_vb);
	members_dialog->set_ok_button_text(TTR("Create"));
	members_dialog->get_ok_button()->connect("pressed", callable_mp(this, &AnimationNodeBlendTreeEditor::_member_create));
	members_dialog->get_ok_button()->set_disabled(true);
	members_dialog->connect("canceled", callable_mp(this, &AnimationNodeBlendTreeEditor::_member_cancel));
	add_child(members_dialog);

	VSeparator *vs = memnew(VSeparator);
	graph->get_menu_hbox()->add_child(vs);
	graph->get_menu_hbox()->move_child(vs, 0);

	add_node = memnew(Button);
	add_node->set_flat(true);
	add_node->set_disabled(read_only);
	add_node->set_text(TTR("Add Node..."));
	graph->get_menu_hbox()->add_child(add_node);
	graph->get_menu_hbox()->move_child(add_node, 0);
	add_node->connect("pressed", callable_mp(this, &AnimationNodeBlendTreeEditor::_show_members_dialog).bind(false, false));

	error_panel = memnew(PanelContainer);
	add_child(error_panel);
	error_label = memnew(Label);
	error_panel->add_child(error_label);
	error_label->set_text("eh");

	filter_dialog = memnew(AcceptDialog);
	add_child(filter_dialog);
	filter_dialog->set_title(TTR("Edit Filtered Tracks:"));

	VBoxContainer *filter_vbox = memnew(VBoxContainer);
	filter_dialog->add_child(filter_vbox);

	filter_enabled = memnew(CheckBox);
	filter_enabled->set_text(TTR("Enable Filtering"));
	filter_enabled->connect("pressed", callable_mp(this, &AnimationNodeBlendTreeEditor::_filter_toggled));
	filter_vbox->add_child(filter_enabled);

	filters = memnew(Tree);
	filter_vbox->add_child(filters);
	filters->set_v_size_flags(SIZE_EXPAND_FILL);
	filters->set_hide_root(true);
	filters->connect("item_edited", callable_mp(this, &AnimationNodeBlendTreeEditor::_filter_edited));

	open_file = memnew(EditorFileDialog);
	add_child(open_file);
	open_file->set_title(TTR("Open Animation Node"));
	open_file->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
	open_file->connect("file_selected", callable_mp(this, &AnimationNodeBlendTreeEditor::_file_opened));
}
