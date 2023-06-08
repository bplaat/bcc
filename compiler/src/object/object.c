#include "object/object.h"

#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/stat.h>
#endif

#include "utils/utils.h"

void object_out(char *path, System system, Program *program) {
    // Create object file
    FILE *f = fopen(path, "wb");
    if (system == SYSTEM_LINUX) object_linx_out(f, program);
    fclose(f);

// Give object file exec rights
#ifndef _WIN32
    chmod(path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
}
