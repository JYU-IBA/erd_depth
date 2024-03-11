#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <jibal.h>
#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <libgen.h> /* for basename() */
#include <sys/param.h> /* for MAXPATHLEN */
#endif
#include "tofe_list.h"

const char *msg_level_str(tofe_list_msg_level level) {
    switch(level) {
        case TOFE_LIST_NORMAL:
            return "";
        case TOFE_LIST_INFO:
            return "info";
        case TOFE_LIST_WARNING:
            return "warning";
        case TOFE_LIST_ERROR:
            return "error";
        default:
            return "";
    }
}

void tofe_list_msg(tofe_list_msg_level level, const char *restrict format, ...) {
    va_list ap;
    va_start(ap, format);
    fprintf(stderr, "tofe_list %s: ", msg_level_str(level));
    vfprintf(stderr, format, ap);
    fputc('\n', stderr);
    va_end(ap);
}

void tofe_files_free(list_files *files) {
    if(!files) {
        return;
    }
    for(size_t i = 0; i < files->n_files; i++) {
        cutfile_free(&files->cutfiles[i]);
    }
    free(files->cutfiles);
    free(files);
}

int cutfile_read_headers(cutfile *cutfile) {
    char *line = NULL, *line_data;
    size_t line_size = 0;
    FILE *f = fopen(cutfile->filename, "r");
    if(!f) {
        tofe_list_msg(TOFE_LIST_ERROR, "Could not open file \"%s\"", cutfile->filename);
        return -1;
    }
    cutfile->basename = tofe_basename(cutfile->filename);
    int error = 0;
    while(getline(&line, &line_size, f) > 0) {
        cutfile->lineno++;
        if(*line == '#') /* Comments are allowed */
            continue;
        if(*line == '\r' || *line == '\n') { /* Empty line signals end of headers */
            break;
        }
        line[strcspn(line, "\r\n")] = 0; /* Strips all kinds of newlines! */
        line_data = line;
        strsep(&line_data, ":");
        if(line_data == NULL) { /* No argument, ignore */
            tofe_list_msg(TOFE_LIST_ERROR, "File %s line %i: \"%s\", no argument or separator ':'.", cutfile->filename,
                          cutfile->lineno, line);
            error++;
            break;
        }
        while(*(line_data) == ' ') { line_data++; }
        if(*line_data == '\0') {
            tofe_list_msg(TOFE_LIST_WARNING, "File %s line %i: \"%s\", argument is empty.", cutfile->filename,
                          cutfile->lineno, line);
            continue;
        }
        //fprintf(stderr, "%i: \"%s\" and \"%s\"\n", cutfile->lineno, line, line_data);
        char *tmp;
        switch (jibal_option_get_value(cutfile_headers, line)) {
            case CUTFILE_HEADER_NONE:
                break;
            case CUTFILE_HEADER_TYPE:
                cutfile->type = jibal_option_get_value(tofe_scatter_types, line_data);
                break;
            case CUTFILE_HEADER_COUNT:
                cutfile->n_counts = strtol(line_data, NULL, 10);
                break;
            case CUTFILE_HEADER_WEIGHT:
                cutfile->event_weight = strtod(line_data, &tmp);
                if(*tmp != '\0') {
                    tofe_list_msg(TOFE_LIST_WARNING, "Trailing characters in event weight: %s (got %g), was conversion successful?", line_data, cutfile->event_weight);
                }
                break;
            case CUTFILE_HEADER_ENERGY:
                break;
            case CUTFILE_HEADER_ANGLE:
                break;
            case CUTFILE_HEADER_ELEMENT:
                break;
            case CUTFILE_HEADER_LOSSES:
                break;
            case CUTFILE_HEADER_SPLIT:
                break;
            default:
                tofe_list_msg(TOFE_LIST_WARNING, "File %s line %i: unknown header \"%s\"", cutfile->filename, cutfile->lineno, line);
        }
    }
    fclose(f);
    return error;
}

void cutfile_reset(cutfile *cutfile) {
    free(cutfile->filename);
    cutfile->filename = NULL;
    cutfile->n_counts = 0;
    cutfile->event_weight = 1.0;
    cutfile->type = SCATTER_ERD;
}

void cutfile_free(cutfile *cutfile) {
    free(cutfile->filename);
    free(cutfile->basename);
}

list_files *tofe_files_from_argv(int argc, char **argv) { /* Argument vector should contain filenames. Reads headers, closes files. */
    if(argc == 0 || argv == NULL) {
        tofe_list_msg(TOFE_LIST_ERROR, "No files.");
        return NULL;
    }
    list_files *files = malloc(sizeof(list_files));
    files->n_files = argc;
    files->cutfiles = calloc(files->n_files, sizeof(cutfile));
    for(int i = 0; i < argc; i++) {
        cutfile *cutfile = &files->cutfiles[i];
        cutfile_reset(cutfile);
        cutfile->filename = strdup(argv[i]);
        if(cutfile_read_headers(cutfile)) { /* Encountered some error, cleanup and return */
            tofe_files_free(files);
            files = NULL;
            break;
        }
    }
    return files;
}

char *tofe_basename(const char *path) { /*  e.g. /bla/bla/tofe2363.O.ERD.0.cut => tofe2363.O.ERD.0.cut  */
#ifdef WIN32
    char *fname = malloc(_MAX_FNAME);
    char *fname_orig = fname;
    char *ext = malloc(_MAX_EXT);
    _splitpath(path, NULL, NULL, fname, ext);
    char *out = malloc(_MAX_FNAME+_MAX_EXT);
    *out = '\0';
    strcat(out, fname);
    strcat(out, ext);
    free(fname_orig);
    free(ext);
    return out;
#else
    char *bname = malloc(MAXPATHLEN);
    basename_r(path, bname);
    return bname;
#endif
}

void tofe_files_print(list_files *files) {
    if(!files) {
        return;
    }
    for(size_t i = 0; i < files->n_files; i++) {
        cutfile *cutfile = &files->cutfiles[i];
        fprintf(stderr, "%32s | %3s | %7.4lf | %8zu\n", cutfile->basename, tofe_scatter_types[cutfile->type].s, cutfile->event_weight, cutfile->n_counts);
    }
}



int main(int argc, char **argv) {
#if 0
    for(int i = 0; i < argc; i++) {
        fprintf(stderr, "tofe_list argv[%i] = %s\n", i, argv[i]);
    }
#endif
    if(argc < 2) {
        fprintf(stderr, "Not enough arguments. Usage: tofe_list <cutfile1> <cutfile2> ...\n");
        return EXIT_FAILURE;
    }
    argc--;
    argv++;
    list_files *files = tofe_files_from_argv(argc, argv);
    if(!files) {
        return EXIT_FAILURE;
    }
    tofe_files_print(files);
    tofe_files_free(files);
    return EXIT_SUCCESS;
}
