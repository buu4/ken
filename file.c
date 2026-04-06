#include "ken.h"

void file_init(File *file, char *path)
{
    file->name = path;

    // Read binary for portability
    file->d = fopen(path, "rb");
    if (!file->d)
        goto err;

    fseek(file->d, 0L, SEEK_END); // jump to end buffer
    file->length = (size_t)ftell(file->d);
    fseek(file->d, 0L, SEEK_SET); // jump to start buffer

    file->content = malloc(file->length + 1);
    if (!file->content)
        goto err;

    fread(file->content, 1, file->length, file->d);
    file->content[file->length] = '\0';
    return;

err:
    error("error: %s: %s", file->name, strerror(errno));
    file_close(file);
}

void file_close(File *file)
{
    if (file->content) free(file->content);
    if (file->d) fclose(file->d);
}
