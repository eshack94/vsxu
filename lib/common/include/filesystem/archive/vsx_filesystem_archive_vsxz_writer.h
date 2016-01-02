#pragma once


#include <filesystem/archive/vsx_filesystem_archive_file_write.h>
#include <filesystem/archive/vsx_filesystem_archive_writer_base.h>

namespace vsx
{

class filesystem_archive_vsxz_writer
  : public filesystem_archive_writer_base
{
  FILE* file;
  vsx_string<> archive_name;
  vsx_nw_vector<filesystem_archive_file_write> archive_files;

  void file_add_all();
  void file_add_all_worker(vsx_nw_vector<filesystem_archive_file_write*>* work_list);
public:

  void create(const char* filename);
  int add_file(vsx_string<> filename, vsx_string<> disk_filename, bool deferred_multithreaded);
  int add_string(vsx_string<> filename, vsx_string<> payload, bool deferred_multithreaded);
  void close();

  ~filesystem_archive_vsxz_writer()
  {}

};

}
