#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#define STBIW_WINDOWS_UTF8
#include "stb_image_write.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char * filepath_to_filename(const char * path)
{
    size_t i, len, lastslash;

    len = strlen(path);
    lastslash = 0;
    for(i = 0; i < len; ++i)
        if(path[i] == '/' || path[i] == '\\')
            lastslash = i + 1;

    return path + lastslash;
}

static int print_usage(const char * argv0)
{
    argv0 = filepath_to_filename(argv0);
    fprintf(stderr, "%s - convert binary file to png\n", argv0);
    fprintf(stderr, "Usage: %s input output.png\n", argv0);
    return 1;
}

static int calculate_image_sizes(size_t s, int * x, int * y)
{
    int i, j;

    for(i = 4; i < 5000; i = i * 2)
    {
        for(j = i / 2; j <= (i + i / 2); ++j)
        {
            if(s < (size_t)(i * j))
            {
                *x = i;
                *y = j;
                return 1;
            }
        }
    }

    return 0;
}

int main(int argc, char ** argv)
{
    void * buff;
    size_t buffsize = 4096 * 4096;
    size_t read;
    FILE * binary;
    int x, y;

    if(argc < 3)
        return print_usage(argv[0]);

    buff = calloc(buffsize + 1, 1);
    if(!buff)
    {
        fprintf(stderr, "calloc failed\n");
        return 1;
    }

    binary = fopen(argv[1], "rb");
    if(!binary)
    {
        free(buff);
        fprintf(stderr, "can't open file\n");
        return 1;
    }

    read = fread(buff, 1, buffsize + 1, binary);
    fclose(binary);
    if(read > buffsize)
    {
        free(buff);
        fprintf(stderr, "file too big\n");
        return 1;
    }

    if(calculate_image_sizes(read, &x, &y))
    {
        stbi_write_png(argv[2], x, y, 1, buff, 0); /*TODO: check this*/
    }
    else
    {
        fprintf(stderr, "failed to calculate image sizes\n");
    }
    free(buff);
    return 0;
}
