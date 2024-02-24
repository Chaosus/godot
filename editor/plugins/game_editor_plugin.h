/**************************************************************************/
/*  game_editor_plugin.h                                                  */
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

#ifndef GAME_EDITOR_PLUGIN_H
#define GAME_EDITOR_PLUGIN_H

#include "editor/editor_plugin.h"
#include "editor/editor_plugin_settings.h"

class Game : public PanelContainer {
	GDCLASS(Game, PanelContainer);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	Game();
};

class GameEditorPlugin : public EditorPlugin {
	GDCLASS(GameEditorPlugin, EditorPlugin);

	Game *game = nullptr;

public:
	virtual String get_name() const override { return "Game"; }
	bool has_main_screen() const override { return true; }
	virtual void edit(Object *p_object) override {}
	virtual bool handles(Object *p_object) const override { return false; }
	virtual void make_visible(bool p_visible) override;
	virtual const Ref<Texture2D> get_icon() const override;

	GameEditorPlugin();
	~GameEditorPlugin();
};

#endif // GAME_EDITOR_PLUGIN_H