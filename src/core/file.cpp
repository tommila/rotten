#pragma once

#include <fcntl.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>
#include <cstdlib>

#include "../platform.h"

bool fileIsLocked(const char* filePath) {
  int fd = open(filePath, O_RDWR);
  bool isLocked = false;
  struct flock fl;
  if (fd == -1) {
    SDL_LOG(LOG_LEVEL_ERROR, "Error opening file %s\n", filePath);
    isLocked = true;
  } else {
    fl.l_type = F_WRLCK;     // Set a write lock
    fl.l_whence = SEEK_SET;  // Start at beginning of file
    fl.l_start = 0;          // Offset from beginning of file
    fl.l_len = 0;            // Lock entire file
    fl.l_pid = getpid();     // Set process ID
    isLocked = fcntl(fd, F_SETLKW, &fl) == -1;
  }

  if (isLocked) {
    SDL_LOG(LOG_LEVEL_VERBOSE, "File is locked\n");
    fl.l_type = F_UNLCK;      // Release the lock
    fcntl(fd, F_SETLK, &fl);  // Unlock the file
  }
  // sleep(10);                // Sleep for 10 seconds
  // fl.l_type = F_UNLCK;      // Release the lock
  // fcntl(fd, F_SETLK, &fl);  // Unlock the file

  // if (flock(fd, LOCK_SH | LOCK_NB) == -1) {
  //   // Another process holds a lock, indicating active writing.
  //   printf("Another process is writing to the file.\n");
  //   return true;
  // } else {
  //   // No other process holds a lock, indicating the file is not being
  //   written
  //   // to.
  //   printf("No other process is writing to the file.\n");
  //   flock(fd, LOCK_UN);  // Release the lock when done.
  // }

  close(fd);
  return isLocked;
}

int fileLock(const char* filePath, struct flock* lock) {
  int fd = -1;

  // Open the lock file
  fd = open(filePath, O_RDWR | O_EXCL | O_CREAT);
  if (fd == -1) {
    SDL_LOG(LOG_LEVEL_ERROR, "Lock file open failed\n");
    return -1;
  }

  // Set up the lock
  lock->l_type = F_WRLCK;
  lock->l_whence = SEEK_SET;
  lock->l_start = 0;
  lock->l_len = 0;

  // Try to acquire the lock
  if (fcntl(fd, F_SETLK, &lock) == -1) {
    SDL_LOG(LOG_LEVEL_ERROR, "File locking failed\n");
    close(fd);
    return -1;
  }
  return fd;
}

bool fileUnlock(int fd, struct flock* lock) {
  // Release the lock
  lock->l_type = F_UNLCK;
  if (fcntl(fd, F_SETLK, &lock) == -1) {
    SDL_LOG(LOG_LEVEL_ERROR, "File unlock failed\n");
    return false;
  }

  close(fd);
  return true;
}

bool fileWaitForLock(const char* filePath) {
  bool waitSuccessful = false;
  SDL_LOG(LOG_LEVEL_INFO, "Wait for file lock: %s\n", filePath);
  int32_t fileHandle;
  if ((fileHandle = open(filePath, O_RDWR)) >= 0) {
    // NOTE(JRC): The 'flock' function will block on a file if it has been
    // f'locked by another process; it's used here to wait on the flock and
    // then immediately continue processing.
    waitSuccessful = !flock(fileHandle, LOCK_EX) && !flock(fileHandle, LOCK_UN);
    waitSuccessful &= !close(fileHandle);
  }

  if (!waitSuccessful) {
    SDL_LOG(LOG_LEVEL_ERROR, "Failed to wait for lock: %s\n", filePath);
    assert(false);
  } else {
    SDL_LOG(LOG_LEVEL_INFO, "Wait for file lock successful\n");
  }
  return waitSuccessful;
}

time_t fileReadModTime(const char* filePath) {
  struct stat sb;
  int ret = stat(filePath, &sb);
  if (ret == 0) {
    return sb.st_mtime;
  }
  return 0;
}
