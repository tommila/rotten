#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

utime readFileModTime(const char* filePath) {
  struct stat sb;
  i32 ret = stat(filePath, &sb);
  if (ret == 0) {
    return sb.st_mtime;
  }
  return 0;
}
