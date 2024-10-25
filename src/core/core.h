typedef enum LogLevel {
  LOG_LEVEL_VERBOSE,
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_WARN,
  LOG_LEVEL_ERROR,
} LogLevel;

#define CONCAT2(dest, str1, str2) \
  strcpy(dest, str1);             \
  strcat(dest, str2);             \
  dest[strlen(str1) + strlen(str2)] = '\0'

#define BYTES(val) ((val))
#define KILOBYTES(val) ((val)*1024)
#define MEGABYTES(val) (KILOBYTES(val) * 1024)
#define GIGABYTES(val) (MEGABYTES(val) * 1024)

#define INPUT_BUFFER_SIZE 32

#define InvalidDefaultCase default: ASSERT(0)

#define arrayLen(arr) sizeof(arr) / sizeof(arr[0])
