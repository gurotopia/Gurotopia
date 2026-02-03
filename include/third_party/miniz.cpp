#define MINIZ_NO_STDIO
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_ARCHIVE_WRITING_APIS

extern "C" {
#include "miniz_tdef.c"
#include "miniz_tinfl.c"
#include "miniz.c"
}

