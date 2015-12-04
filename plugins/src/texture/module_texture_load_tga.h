#include <texture/vsx_texture.h>
#include <bitmap/loader/vsx_bitmap_loader_tga.h>

class module_texture_load_tga: public vsx_module
{
  // in
  vsx_module_param_resource* filename_in;
  vsx_module_param_int* reload_in;
  vsx_module_param_int* mipmaps_in;
  vsx_module_param_int* flip_vertical_in;
  vsx_module_param_int* min_mag_filter_in;

  // out
  vsx_module_param_texture* texture_out;

  // internal
  vsx_string<> current_filename;
  vsx_texture<>* texture = 0x0;
  int flip_vertical_cache = 0;
  int mipmaps_cache = 0;
  int min_mag_filter_cache = 1;

public:

  void module_info(vsx_module_info* info)
  {
    info->identifier =
      "texture;loaders;tga_tex_load";

    info->description =
      "Loads a TGA image from\n"
      "disk and outputs a\n"
      "texture.\n"
    ;

    info->in_param_spec =
      "filename:resource,"
      "reload:enum?no|yes&nc=1,"
      "mipmaps:enum?no|yes&nc=1,"
      "flip_vertical:enum?no|yes&nc=1,"
      "min_mag_filter:enum?nearest|linear&nc=1"
     ;

    info->out_param_spec =
      "texture:texture";

    info->component_class =
      "texture";
  }

  void declare_params(vsx_module_param_list& in_parameters, vsx_module_param_list& out_parameters)
  {
    filename_in = (vsx_module_param_resource*)in_parameters.create(VSX_MODULE_PARAM_ID_RESOURCE,"filename");
    filename_in->set("");
    current_filename = "";

    reload_in = (vsx_module_param_int*)in_parameters.create(VSX_MODULE_PARAM_ID_INT, "reload");
    mipmaps_in = (vsx_module_param_int*)in_parameters.create(VSX_MODULE_PARAM_ID_INT, "mipmaps");
    flip_vertical_in = (vsx_module_param_int*)in_parameters.create(VSX_MODULE_PARAM_ID_INT, "flip_vertical");

    min_mag_filter_in = (vsx_module_param_int*)in_parameters.create(VSX_MODULE_PARAM_ID_INT, "min_mag_filter");
    min_mag_filter_in->set(1); // linear

    texture_out = (vsx_module_param_texture*)out_parameters.create(VSX_MODULE_PARAM_ID_TEXTURE,"texture");
    texture_out->valid = false;
  }


  void run()
  {
    if (texture && texture->texture->bitmap->data_ready)
    {
      loading_done = true;
      message = "module||ok";
    }

    bool reload = false;

    if (current_filename != filename_in->get())
      reload = true;

    if (reload_in->get())
      reload = true;

    if (flip_vertical_in->get() != flip_vertical_cache)
      reload = true;

    if (mipmaps_in->get() != mipmaps_cache)
      reload = true;

    if (min_mag_filter_in->get() != min_mag_filter_cache)
      reload = true;

    if (!reload)
      return;

    mipmaps_cache = mipmaps_in->get();
    flip_vertical_cache = flip_vertical_in->get();
    min_mag_filter_cache = min_mag_filter_in->get();

    bool do_reload = reload_in->get();
    reload_in->set(0);

    if (!verify_filesuffix(filename_in->get(),"tga"))
    {
      filename_in->set(current_filename);
      message = "module||ERROR! This is not a TGA image file!";
      return;
    }

    current_filename = filename_in->get();

    if (texture && !do_reload)
      vsx_texture_loader::destroy(texture);

    uint64_t bitmap_loader_hint = 0;
    bitmap_loader_hint |= vsx_bitmap::flip_vertical_hint * flip_vertical_cache;

    uint64_t hint = 0;
    hint |= vsx_texture_gl::mipmaps_hint * mipmaps_cache;
    hint |= vsx_texture_gl::linear_interpolate_hint * min_mag_filter_cache;

    texture = vsx_texture_loader::load(
      current_filename,
      engine->filesystem,
      true, // threaded
      bitmap_loader_hint,
      hint,
      do_reload
    );
    texture_out->set(texture);
  }

  void output(vsx_module_param_abs* param)
  {
  }

  void stop() {
  }

  void start() {
  }

  void on_delete()
  {
    if (texture)
      vsx_texture_loader::destroy(texture);
  }

};

