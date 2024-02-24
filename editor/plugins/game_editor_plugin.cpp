/**************************************************************************/
/*  game_editor_plugin.cpp                                                */
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

#include "game_editor_plugin.h"
#include "editor/editor_node.h"

// Game

void Game::_notification(int p_what) {
}

void Game::_bind_methods() {
}

Game::Game() {
}

// GameEditorPlugin

const Ref<Texture2D> GameEditorPlugin::get_icon() const {
	return EditorNode::get_singleton()->get_class_icon(SNAME("Joypad"));
}

void GameEditorPlugin::make_visible(bool p_visible) {
	if (p_visible) {
		game->show();
	} else {
		game->hide();
	}
}

GameEditorPlugin::GameEditorPlugin() {
	game = memnew(Game);
	game->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	EditorNode::get_singleton()->get_main_screen_control()->add_child(game);
	game->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	game->hide();
}

GameEditorPlugin::~GameEditorPlugin() {
}
