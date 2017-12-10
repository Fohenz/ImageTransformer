/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if !defined(_VIEW_H)
#define _VIEW_H
#include <tizen.h>
#include <dlog.h>
#include <app.h>
#include <efl_extension.h>
#include <Elementary.h>

struct view_info {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *navi;
	Evas_Object *buttons[4];

	Evas_Object *width;
	Evas_Object *height;
};

Eina_Bool view_create(void *user_data);
Evas_Object *view_create_win(const char *pkg_name);
Evas_Object *view_create_layout(Evas_Object *parent, const char *file_path, const char *group_name, Eext_Event_Cb cb_function, void *user_data);
Evas_Object *view_create_conformant_without_indicator(Evas_Object *win);
Evas_Object *view_create_naviframe(Evas_Object *win);
void view_destroy(void);
void view_destroy_layout(Evas_Object *layout);
void _add_entry_text(const char *text);

#define _PRINT_MSG_LOG_BUFFER_SIZE_ 1024
#define PRINT_MSG(fmt, args...) do { char _log_[_PRINT_MSG_LOG_BUFFER_SIZE_]; \
    snprintf(_log_, _PRINT_MSG_LOG_BUFFER_SIZE_, fmt, ##args); _add_entry_text(_log_); } while (0)

typedef enum {
    CONVERT_BTN,
    RESIZE_BTN,
    ROTATE_BTN,
    CROP_BTN,
    BUTTON_COUNT
} app_button;

Evas_Object *_new_button(void *data, Evas_Object *display, char *name, void *cb);
Evas_Object *_create_new_cd_display(char *name, void *cb);
Eina_Bool _pop_cb(void *data, Elm_Object_Item *item);

void _disable_button(app_button button, Eina_Bool disabled);
void _create_button(app_button button, Evas_Object *display, char *name, void *callback);
void _pop_navi();

#endif
