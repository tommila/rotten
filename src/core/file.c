#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

time_t fileReadModTime(const char* filePath) {
  struct stat sb;
  int ret = stat(filePath, &sb);
  if (ret == 0) {
    return sb.st_mtime;
  }
  return 0;
}
