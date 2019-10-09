/**
 * @file path.cxx
 *
 * Implementation of path related operations.
 */

#include <cstdlib>
#include <cstring>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

extern "C" {

char *mdbfs_path_lexically_normal(const char *path)
{
  std::string normalized_path = fs::u8path(path).lexically_normal().string();

  char *ret = (char *)malloc(normalized_path.size() + 1);
  strncpy(ret, normalized_path.c_str(), normalized_path.size() + 1);

  return ret;
}

int mdbfs_path_is_absolute(const char *path)
{
  fs::path cxx_path = fs::u8path(path).lexically_normal();

  if (cxx_path.is_absolute())
    return 1;
  else
    return 0;
}

}
