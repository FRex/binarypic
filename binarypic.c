#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_SIMD
#define STB_IMAGE_STATIC
#define STBI_WINDOWS_UTF8
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#define STBI_WINDOWS_UTF8
#include "stb_image_write.h"

static int my_utf8_main(int argc, char ** argv);
#define BLA_WMAIN_FUNC my_utf8_main
#include "blawmain.h"

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
    fprintf(stderr, "%s - convert binary file to greyscale png or back\n", argv0);
    fprintf(stderr, "Info : BLA_WMAIN_USING_WMAIN_BOOLEAN = %d\n", BLA_WMAIN_USING_WMAIN_BOOLEAN);
    fprintf(stderr, "Usage: %s input output.png\n", argv0);
    fprintf(stderr, "Usage: %s -d input.png output\n", argv0);
    return 1;
}

static int calculate_image_sizes(size_t s, int * x, int * y)
{
    int i, j;

    for(i = 4; i < 5000; i = i * 2)
    {
        for(j = i / 2; j <= (i + i / 2 + i / 4); ++j)
        {
            if(s <= (size_t)(i * j))
            {
                *x = i;
                *y = j;
                return 1;
            }
        }
    }

    return 0;
}

static FILE * my_utf8_fopen_binary(const char * fname, int write)
{
#ifndef _MSC_VER
    return fopen(fname, write ? "wb" : "rb");
#else
    FILE * ret;
    const int utf16chars = strlen(fname) + 5;
    wchar_t * fname16 = (wchar_t*)calloc(utf16chars, sizeof(wchar_t));
    if(!fname16)
        return NULL;

    MultiByteToWideChar(CP_UTF8, 0, fname, -1, fname16, utf16chars);
    ret = _wfopen(fname16, write ? L"wb" : L"rb");
    free(fname16);
    return ret;
#endif
}

static unsigned char contrasting(unsigned char c)
{
    return ~c;
}

static void set_contrasting_padding(void * buff, int good, int buffsize)
{
    unsigned char * b = (unsigned char*)buff;
    unsigned char p = (good > 0) ? contrasting(b[good - 1]) : 0xff;
    memset(b + good, p, buffsize - good);
}

static int encode(const char * ifile, const char * ofile)
{
    void * buff;
    size_t buffsize = 4096 * 4096;
    size_t read;
    FILE * binary;
    int x, y;

    buff = calloc(buffsize + 1, 1);
    if(!buff)
    {
        fprintf(stderr, "calloc failed\n");
        return 1;
    }

    binary = my_utf8_fopen_binary(ifile, 0);
    if(!binary)
    {
        free(buff);
        fprintf(stderr, "can't open file '%s'\n", ifile);
        return 1;
    }

    read = fread(buff, 1, buffsize + 1, binary); /* TODO: retry until eof? */
    fclose(binary);
    if(read > buffsize)
    {
        free(buff);
        fprintf(stderr, "file '%s' is too big (over %.1f MiB)\n", ifile, buffsize / (1024.0 * 1024.0));
        return 1;
    }

    /* read + 1 to always get at least one byte of padding! */
    if(calculate_image_sizes(read + 1, &x, &y))
    {
        set_contrasting_padding(buff, (int)read, x * y);
        if(!stbi_write_png(ofile, x, y, 1, buff, 0))
            fprintf(stderr, "stbi_write_png to file '%s' failed\n", ofile);
    }
    else
    {
        fprintf(stderr, "failed to calculate image sizes\n");
    }

    free(buff);
    return 0;
}

static int size_without_padding(const unsigned char * pixels, int size)
{
    unsigned char last;

    if(size == 0)
        return 0;

    last = pixels[size - 1];
    while(size > 0 && pixels[size - 1] == last)
        --size;

    return size;
}

static int decode(const char * ifile, const char * ofile)
{
    int w, h, comp, datasize;
    unsigned char * grey;
    size_t written;
    FILE * binary;

    grey = stbi_load(ifile, &w, &h, &comp, 0);
    if(grey == NULL)
    {
        fprintf(stderr, "stbi_load returned NULL\n");
        return 1;
    }

    if(comp != 1)
    {
        fprintf(stderr, "this isn't a greyscale image\n");
        stbi_image_free(grey);
        return 1;
    }

    binary = my_utf8_fopen_binary(ofile, 1);
    if(!binary)
    {
        fprintf(stderr, "can't open file '%s'\n", ofile);
        stbi_image_free(grey);
        return 1;
    }

    datasize = size_without_padding(grey, w * h);
    written = fwrite(grey, 1, datasize, binary);
    if(written != datasize)
        fprintf(stderr, "failed to fwrite all %d vs %d\n", (int)written, datasize);

    stbi_image_free(grey);
    fclose(binary);
    return (written == datasize) ? 0 : 1;
}

static int my_utf8_main(int argc, char ** argv)
{
    if(argc == 3)
        return encode(argv[1], argv[2]);

    if(argc == 4 && 0 == strcmp(argv[1], "-d"))
        return decode(argv[2], argv[3]);

    return print_usage(argv[0]);
}
