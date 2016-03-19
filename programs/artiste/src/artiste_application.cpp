/**
* Project: VSXu: Realtime modular visual programming language, music/audio visualizer.
*
* This file is part of Vovoid VSXu.
*
* @author Jonatan Wallmander, Robert Wenzel, Vovoid Media Technologies AB Copyright (C) 2003-2013
* @see The GNU Public License (GPL)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vsx_gl_global.h"
#include "pthread.h"
#include "vsx_command.h"
#include "vsx_param.h"
#include "vsx_module.h"
#include <texture/vsx_texture.h>
#include <time/vsx_timer.h>
#include <filesystem/vsx_filesystem.h>
#include "vsx_font.h"
#include <vsx_version.h>
#include <vsx_engine.h>
#include <vsx_module_list_factory.h>
#include <debug/vsx_error.h>
#include <debug/vsx_backtrace.h>
#include <vsx_data_path.h>

#include "log/vsx_log_a.h"

#include <vsx_widget.h>

#include "vsx_widget_window.h"
#include "artiste_desktop.h"
#include <vsx_command_client_server.h>
#include "vsx_widget/server/vsx_widget_server.h"
#include "vsx_widget/module_choosers/vsx_widget_module_chooser.h"
#include "vsx_widget/helpers/vsx_widget_object_inspector.h"
#include "vsx_widget/helpers/vsx_widget_preview.h"
#include "logo_intro.h"
//#define NO_INTRO
#include "artiste_application.h"
#include "vsxg.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <profiler/vsx_profiler_manager.h>
#if PLATFORM_FAMILY == PLATFORM_FAMILY_UNIX
#include <time.h>
#endif



// global vars
vsx_string<>fpsstring = "VSX Ultra "+vsx_string<>(vsxu_version)+" - 2012 Vovoid";

vsx_module_list_abs* module_list;
vsx_engine* vxe = 0x0;

// from the perspective (both for gui/server) from here towards the tcp thread
vsx_command_list system_command_queue;
vsx_command_list internal_cmd_in;
vsx_command_list internal_cmd_out;
vsx_widget_desktop *desktop = 0;
bool prod_fullwindow = false;
bool take_screenshot = false;
bool record_movie = false;
bool *gui_prod_fullwindow;
bool gui_prod_fullwindow_helptext = true;
bool reset_time_measurements = false;
vsx_font myf;

unsigned long frame_counter = 0;
unsigned long delta_frame_counter = 0;
float delta_frame_time = 0.0f;
float delta_fps;
float total_time = 0.0f;

float global_time;
vsx_timer time2;

void load_desktop_a(vsx_string<>state_name = "");

// draw-related variables
#include "vsx_artiste_draw.h"


vsx_artiste_draw* my_draw = 0x0;

void load_desktop_a(vsx_string<>state_name)
{
  //printf("{CLIENT} creating desktop:");
  desktop = new vsx_widget_desktop;
  //printf(" [DONE]\n");
  internal_cmd_in.clear_normal();
  internal_cmd_out.clear_normal();
  // connect server widget to command lists
  ((vsx_widget_server*)desktop->find("desktop_local"))->cmd_in = &internal_cmd_out;
  ((vsx_widget_server*)desktop->find("desktop_local"))->cmd_out = &internal_cmd_in;
  if (state_name != "")
    ((vsx_widget_server*)desktop->find("desktop_local"))->state_name =
      vsx_string_helper::str_replace<char>
      (
        "/",
        ";",
        vsx_string_helper::str_replace<char>(
          "//",
          ";",
          vsx_string_helper::str_replace<char>(
            "_states/",
            "",
            state_name
          )
        )
      );
  internal_cmd_out.add_raw(vsx_string<>("vsxu_welcome ")+vsx_string<>(vsxu_ver)+" 0", true);
  desktop->system_command_queue = &system_command_queue;
  vsx_widget* t_viewer = desktop->find("vsxu_preview");
  if (t_viewer)
  {
    gui_prod_fullwindow = ((vsx_window_texture_viewer*)desktop->find("vsxu_preview"))->get_fullwindow_ptr();
    LOG_A("found vsxu_preview widget")
  }
  if (!dual_monitor)
  ((vsx_widget_server*)desktop->find("desktop_local"))->engine = (void*)vxe;
  desktop->front(desktop->find("vsxu_preview"));

  desktop->init();
}

/*
 print out help texts
*/
void app_print_cli_help()
{
  printf(
         "VSXu Artiste command syntax:\n"
         "  -f             fullscreen mode\n"
         "  -ff            start preview in fullwindow mode (same as Ctrl+F)"
         "  -s 1920,1080   screen/window size\n"
         "  -p 100,100     window posision\n"
         "  -novsync       disable vsync\n"
         "  -gl_debug      enable nvidia's gl debug callback\n"
         "\n"
        );

  vsx_module_list_factory_create()->print_help();
}

void app_load(int id)
{
  if (dual_monitor && id == 0)
    return;


  module_list = vsx_module_list_factory_create();
  vxe = new vsx_engine(module_list);

  my_draw = new vsx_artiste_draw();

  gui_prod_fullwindow = &prod_fullwindow;
  //---------------------------------------------------------------------------
  myf.load( PLATFORM_SHARED_FILES + vsx_string<>("font/font-ascii_output.png"), vsx::filesystem::get_instance());

  if (dual_monitor) {
    vxe->start();
  }
}

void app_unload()
{
  myf.unload();
  vxe->stop();
  delete vxe;
  vsx_module_list_factory_destroy( module_list );
  desktop->stop();
  desktop->on_delete();
  delete desktop;
  vsx_command_process_garbage_exit();
}

void app_pre_draw() {
  vsx_command_s *c = 0;
  while ( (c = system_command_queue.pop()) )
  {
    vsx_string<>cmd = c->cmd;
    if (cmd == "system.shutdown")
    {
      app_close_window();
    }
    if (cmd == "fullscreen") {
      if (desktop)
        desktop->stop();
      internal_cmd_in.add(c);
    }
    c = 0;
  }
}

// id is 0 for first monitor, 1 for the next etc.
bool app_draw(int id)
{
  if (id == 0) {
    if (!my_draw)
      VSX_ERROR_RETURN_V("my draw is 0x0", false);
    my_draw->draw();
  } else
  {
    if (dual_monitor) {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer
      vxe->process_message_queue(&internal_cmd_in,&internal_cmd_out);
      vxe->render();
      glDisable(GL_DEPTH_TEST);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();											// Reset The Modelview Matrix
      glEnable(GL_BLEND);
      ++frame_counter;
      ++delta_frame_counter;
      float dt = time2.dtime();
      delta_frame_time+= dt;
      total_time += dt;
      if (delta_frame_counter == 100) {
        delta_fps = 100.0f/delta_frame_time;
        delta_frame_counter = 0;
        delta_frame_time = 0.0f;
      }
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glColor4f(0,0,0,0.4f);
      glBegin(GL_QUADS);									// Draw A Quad
        glVertex3f(-1.0f, 1.0f, 0.0f);					// Top Left
        glVertex3f( 1.0f, 1.0f, 0.0f);					// Top Right
        glVertex3f( 1.0f,0.92f, 0.0f);					// Bottom Right
        glVertex3f(-1.0f,0.92f, 0.0f);					// Bottom Left
      glEnd();											// Done Drawing The Quad
      myf.print(vsx_vector3<>(-1.0f,0.92f)," Fc "+vsx_string_helper::i2s(frame_counter)+" Fps "+vsx_string_helper::f2s(delta_fps)+" T "+vsx_string_helper::f2s(total_time)+" Tfps "+vsx_string_helper::f2s(frame_counter/total_time)+" MC "+vsx_string_helper::i2s(vxe->get_num_modules())+" VSX Ultra (c) Vovoid",0.07);
    }
  }
  return true;
}

void app_char(long key)
{
  req(desktop);
  desktop->key_down(key, app_alt, app_ctrl, app_shift);
}

void app_key_down(long key)
{
  if (app_ctrl && key == '5')
    vsx_profiler_manager::get_instance()->enable();

  if (app_ctrl && key == '4')
    vsx_profiler_manager::get_instance()->disable();

  if (desktop) {
    if (app_alt && app_ctrl && app_shift && key == 80) {
      if (record_movie == false)
      {
        my_draw->movie_frame_count = 0;
      }
      record_movie = !record_movie;
    }
    else
    if (app_alt && app_ctrl && key == 80)
      take_screenshot = true;

    if
      (
        !( key == 'F' && (app_alt || app_ctrl) )
        &&
        !( key == 'T' && (app_alt || app_ctrl) )
        &&
        !desktop->performance_mode
        &&
        *gui_prod_fullwindow
      )
    {
      vsx_engine_input_event eie;
      eie.type = VSX_ENGINE_INPUT_EVENT_KEY_DOWN;
      eie.key = -key;
      eie.ctrl = app_ctrl;
      eie.alt = app_alt;
      eie.shift = app_shift;
      if (vxe)
      {
        vxe->input_event(eie);
      }
      return;
    }

    if (*gui_prod_fullwindow && app_alt && !app_ctrl && !app_shift && key == 'T') gui_prod_fullwindow_helptext = !gui_prod_fullwindow_helptext;
    if (*gui_prod_fullwindow && !app_alt && app_ctrl && !app_shift && key == 'T') reset_time_measurements = true;
    desktop->set_key_modifiers(app_alt, app_ctrl, app_shift);
    desktop->key_down(-key, app_alt, app_ctrl, app_shift);
  }
}

void app_key_up(long key)
{
  //if (desktop)
  if (desktop) {
    if (
      !desktop->performance_mode
      &&
      *gui_prod_fullwindow
    )
    {
      vsx_engine_input_event eie;
      eie.type = VSX_ENGINE_INPUT_EVENT_KEY_UP;
      eie.key = key;
      eie.ctrl = app_ctrl;
      eie.alt = app_alt;
      eie.shift = app_shift;
      if (vxe)
      {
        vxe->input_event(eie);
      }
      return;
    }
    desktop->set_key_modifiers(app_alt, app_ctrl, app_shift);
    desktop->key_up(key,app_alt, app_ctrl, app_shift);
  }
}

void app_mouse_move_passive(int x, int y) {
  if (desktop) {
    if (
      !desktop->performance_mode
      &&
      *gui_prod_fullwindow
    )
    {
      vsx_engine_input_event eie;
      eie.type = VSX_ENGINE_INPUT_EVENT_MOUSE_HOVER;
      eie.x = (float)x;
      eie.y = (float)y;
      eie.ctrl = app_ctrl;
      eie.alt = app_alt;
      eie.shift = app_shift;
      if (vxe)
      {
        vxe->input_event(eie);
      }
      return;
    }
    desktop->set_key_modifiers(app_alt, app_ctrl, app_shift);
    desktop->mouse_move_passive((float)x,(float)y);
  }
}

void app_mouse_move(int x, int y)
{
  GLint	viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  int xx = x;
  int yy = y;
  if (xx < 0) xx = 0;
  if (yy < 0) yy = 0;
  if (xx > viewport[2]) xx = viewport[2]-1;
  if (yy > viewport[3]) yy = viewport[3]-1;
  if (desktop) {

    if (
      !desktop->performance_mode
      &&
      *gui_prod_fullwindow
    )
    {
      vsx_engine_input_event eie;
      eie.type = VSX_ENGINE_INPUT_EVENT_MOUSE_MOVE;
      eie.x = (float)x;
      eie.y = (float)y;
      eie.ctrl = app_ctrl;
      eie.alt = app_alt;
      eie.shift = app_shift;
      if (vxe)
      {
        vxe->input_event(eie);
      }
      return;
    }

    desktop->set_key_modifiers(app_alt, app_ctrl, app_shift);
    desktop->mouse_move(xx,yy);
  }
}

void app_mouse_down(unsigned long button,int x,int y)
{
  if (desktop) {
    if (
      !desktop->performance_mode
      &&
      *gui_prod_fullwindow
    )
    {
      vsx_engine_input_event eie;
      eie.type = VSX_ENGINE_INPUT_EVENT_MOUSE_DOWN;
      eie.key = button;
      eie.x = (float)x;
      eie.y = (float)y;
      eie.ctrl = app_ctrl;
      eie.alt = app_alt;
      eie.shift = app_shift;
      if (vxe)
      {
        vxe->input_event(eie);
      }
      return;
    }

    vsx_widget::set_key_modifiers(app_alt, app_ctrl, app_shift);
    desktop->mouse_down(x,y,button);
  }
}

void app_mouse_up(unsigned long button,int x,int y)
{
  if (desktop) {
    if (
      !desktop->performance_mode
      &&
      *gui_prod_fullwindow
    )
    {
      vsx_engine_input_event eie;
      eie.type = VSX_ENGINE_INPUT_EVENT_MOUSE_UP;
      eie.key = button;
      eie.x = (float)x;
      eie.y = (float)y;
      eie.ctrl = app_ctrl;
      eie.alt = app_alt;
      eie.shift = app_shift;
      if (vxe)
      {
        vxe->input_event(eie);
      }
      return;
    }
    desktop->set_key_modifiers(app_alt, app_ctrl, app_shift);
    desktop->mouse_up(x,y,button);
  }
}

void app_mousewheel(float diff,int x,int y)
{
  if (desktop)
  {
    if (
      !desktop->performance_mode
      &&
      *gui_prod_fullwindow
    )
    {
      vsx_engine_input_event eie;
      eie.type = VSX_ENGINE_INPUT_EVENT_MOUSE_WHEEL;
      eie.x = (float)x;
      eie.y = (float)y;
      eie.w = diff;
      eie.ctrl = app_ctrl;
      eie.alt = app_alt;
      eie.shift = app_shift;
      if (vxe)
      {
        vxe->input_event(eie);
      }
      return;
    }

    desktop->set_key_modifiers(app_alt, app_ctrl, app_shift);
    desktop->mouse_wheel(diff);
  }
}

