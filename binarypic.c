#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#define STBI_WINDOWS_UTF8
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

#ifdef _MSC_VER
/*for MultiByteToWideChar */
#include <Windows.h>
#endif

static FILE * my_utf8_fopen_rb(const char * fname)
{
#ifndef _MSC_VER
    return fopen(fname, "rb");
#else
    FILE * ret;
    const int utf16chars = strlen(fname) + 5;
    wchar_t * fname16 = (wchar_t*)calloc(utf16chars, sizeof(wchar_t));
    if(!fname16)
        return NULL;

    MultiByteToWideChar(CP_UTF8, 0, fname, -1, fname16, utf16chars);
    ret = _wfopen(fname16, L"rb");
    free(fname16);
    return ret;
#endif
}

static int my_utf8_main(int argc, char ** argv)
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

    binary = my_utf8_fopen_rb(argv[1]);
    if(!binary)
    {
        free(buff);
        fprintf(stderr, "can't open file\n");
        return 1;
    }

    read = fread(buff, 1, buffsize + 1, binary); /* TODO: retry until eof? */
    fclose(binary);
    if(read > buffsize)
    {
        free(buff);
        fprintf(stderr, "file too big\n");
        return 1;
    }

    if(calculate_image_sizes(read, &x, &y))
    {
        stbi_write_png(argv[2], x, y, 1, buff, 0); /* TODO: check this*/
    }
    else
    {
        fprintf(stderr, "failed to calculate image sizes\n");
    }
    free(buff);
    return 0;
}

#ifndef _MSC_VER

int main(int argc, char ** argv)
{
    return my_utf8_main(argc, argv);
}

#else

/* for wcslen */
#include <wchar.h>

int wmain(int argc, wchar_t ** argv)
{
    int i, retcode;
    char ** utf8argv = (char **)calloc(argc + 1, sizeof(char*));
    if(!utf8argv)
    {
        fputs("calloc error in wmain\n", stderr);
        return 1;
    }

    retcode = 0;
    for(i = 0; i < argc; ++i)
    {
        const size_t utf8len = wcslen(argv[i]) * 3 + 10;
        utf8argv[i] = (char*)calloc(utf8len, 1);
        if(!utf8argv[i])
        {
            retcode = 1;
            fputs("calloc error in wmain\n", stderr);
            break;
        }
        stbiw_convert_wchar_to_utf8(utf8argv[i], utf8len, argv[i]);
    }

    if(retcode == 0)
        retcode = my_utf8_main(argc, utf8argv);

    for(i = 0; i < argc; ++i)
        free(utf8argv[i]);

    free(utf8argv);
    return retcode;
}

#endif /* _MSC_VER */
