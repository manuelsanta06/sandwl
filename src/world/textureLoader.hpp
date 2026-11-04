#pragma once

#include <cstddef>

unsigned int loadTextureFile(const char *path);
unsigned int loadTextureMemory(const unsigned char *data,size_t size);
