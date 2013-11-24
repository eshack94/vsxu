class module_texture_load_jpeg_alpha : public vsx_module
{
  // in
  float time;
  vsx_module_param_resource* filename_in;
  vsx_module_param_resource* filename_alpha_in;
  // out
  vsx_module_param_bitmap* bitmap_out;
  vsx_module_param_texture* texture_out;
  // internal
  vsx_texture* texture;

  // threading stuff
  void* pti_l;

  static void* jpeg_worker_v(void *ptr)
  {
    module_texture_load_jpeg_alpha* mod = (module_texture_load_jpeg_alpha*)ptr;

    vsx_string ret;

    CJPEGTest* cj = new CJPEGTest;
    if
    (
     !(cj
       ->LoadJPEG(
       mod->current_filename,
       ret,
       mod->engine->filesystem
     ))
    )
    {
      mod->message = "module||"+ret+"\n"+((module_texture_load_jpeg_alpha*)ptr)->current_filename;
      mod->thread_state = -1;
      delete cj;
      return 0;
    }

    if (mod->current_alpha_filename != "")
    {

      CJPEGTest* cj_a = new CJPEGTest;
      if
      (
        !(cj_a
         ->LoadJPEG(
         mod->current_alpha_filename,
         ret,
         mod->engine->filesystem
       ))
      )
      {
        mod->message = "module||"+ret+"\n"+((module_texture_load_jpeg_alpha*)ptr)->current_filename;
        mod->thread_state = -1;
        delete cj;
        delete cj_a;
        return 0;
      }

      mod->bitm.size_x = cj->GetResX();
      mod->bitm.size_y = cj->GetResY();

      // allocate data in the vsx_bitmap
      vsx_bitmap_32bt b_c = (mod->bitm.size_x) * (mod->bitm.size_y);

      unsigned char* rgbcp = (unsigned char*)cj->m_pBuf;

      unsigned char* acp = (unsigned char*)cj_a->m_pBuf;

      mod->bitm.data = new vsx_bitmap_32bt[b_c*2];

      for (unsigned long i = 0; i < b_c; ++i)
      {
        ((vsx_bitmap_32bt*)mod->bitm.data)[i] =
            acp[i*3] << 24 |
            rgbcp[i*3+2] << 16 |
            rgbcp[i*3+1] << 8 |
            rgbcp[i*3];
      }

      delete cj;
      delete cj_a;

      mod->thread_state = 2;
      return 0;
    }

    // no alpha filename selected

    mod->bitm.size_x = cj->GetResX();
    mod->bitm.size_y = cj->GetResY();

    // allocate data in the vsx_bitmap
    vsx_bitmap_32bt b_c = (mod->bitm.size_x) * (mod->bitm.size_y);

    unsigned char* rgbcp = (unsigned char*)cj->m_pBuf;

    mod->bitm.data = new vsx_bitmap_32bt[b_c*2];

    for (unsigned long i = 0; i < b_c; ++i)
    {
      ((vsx_bitmap_32bt*)mod->bitm.data)[i] =
          0xFF000000 |
          rgbcp[i*3+2] << 16 |
          rgbcp[i*3+1] << 8 |
          rgbcp[i*3];
    }

    delete cj;

    mod->thread_state = 2;

    return 0;
  }

public:
  int m_type;

  vsx_string current_filename;
  vsx_string current_alpha_filename;

  vsx_bitmap bitm;
  int bitm_timestamp; // keep track of the timestamp for the bitmap internally
  int               thread_state;
  bool              thread_working;
  pthread_t         worker_t;
  pthread_attr_t    worker_t_attr;
  int texture_timestamp;

  void module_info(vsx_module_info* info)
  {
    info->description =
      "Loads 2 JPEGs from\n"
      "disk and outputs a \n"
      " - VSXu bitmap with alpha\n"
      "and\n"
      "- texture\n"
      "Alpha channel from 2nd jpeg\n"
      "since jpeg's don't have alpha chan.\n"
      "Texture is only loaded when used.\n"
      "This is to preserve memory."
    ;

    info->in_param_spec =
      "filename_rgb:resource,"
      "filename_alpha:resource"
    ;

    info->out_param_spec =
      "texture:texture,"
      "bitmap:bitmap"
    ;

    if (m_type == 0)
    {
      info->identifier = "bitmaps;loaders;jpeg_bitm_load_alpha";
      info->component_class = "bitmap";
    }
    else
    {
      info->identifier = "texture;loaders;jpeg_tex_load_alpha";
      info->component_class = "texture";
    }
  }

  void declare_params(vsx_module_param_list& in_parameters, vsx_module_param_list& out_parameters)
  {
    texture_timestamp = -1;
    loading_done = false;

    filename_in = (vsx_module_param_resource*)in_parameters.create(VSX_MODULE_PARAM_ID_RESOURCE,"filename_rgb");
    filename_in->set("");
    filename_alpha_in = (vsx_module_param_resource*)in_parameters.create(VSX_MODULE_PARAM_ID_RESOURCE,"filename_alpha");
    filename_alpha_in->set("");
    current_filename = "";
    current_alpha_filename = "";

    // out
    bitmap_out = (vsx_module_param_bitmap*)out_parameters.create(VSX_MODULE_PARAM_ID_BITMAP,"bitmap");

    bitm.size_x = 0;
    bitm.size_y = 0;
    bitm_timestamp = 0;
    bitm.valid = false;

    bitmap_out->set_p(bitm);
    thread_state = 0;
    texture_out = (vsx_module_param_texture*)out_parameters.create(VSX_MODULE_PARAM_ID_TEXTURE,"texture");

    texture = new vsx_texture;
    texture->init_opengl_texture_2d();
  }

  void run()
  {
    if (current_filename != filename_in->get())
    {
      if (thread_state == -1)
      {
        message = "module||ok";
      }

      // time to decode a new jpg
      if (thread_state == 1)
      {
        long* aa;
        pthread_join(worker_t, (void **)&aa);
      }

      if (!verify_filesuffix(filename_in->get(),"jpg"))
      {
        filename_in->set(current_filename);
        message = "module||ERROR! This is not a JPG image file!";
        return;
      }
      message = "module||ok";

      current_filename = filename_in->get();
      current_alpha_filename = filename_alpha_in->get();

      pthread_attr_init(&worker_t_attr);
      thread_state = 1;
      pthread_create(&(worker_t), &(worker_t_attr), &jpeg_worker_v, (void*)this);
    }
    if (thread_state == 2)
    {
      bitm.bpp = 4;
      bitm.bformat = GL_RGBA;
      bitm.valid = true;
      ++bitm.timestamp;
      thread_state = 3;
      loading_done = true;
      bitmap_out->set_p(bitm);
    }
  }

  void output(vsx_module_param_abs* param)
  {
    if (param == (vsx_module_param_abs*)texture_out)
    {
      if (texture_timestamp != bitm.timestamp && bitm.valid)
      {
        texture->upload_ram_bitmap_2d(&bitm,true);
        texture->valid = true;
        texture_out->set(texture);
        texture_timestamp = bitm.timestamp;
      }
    }
  }

  void on_delete()
  {
    if (thread_state == 1)
    pthread_join(worker_t, 0);
    if (bitm.valid)
    {
      delete[] (vsx_bitmap_32bt*)bitm.data;
    }
  }
};

