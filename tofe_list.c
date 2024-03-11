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

int cutfile_parse_filename(cutfile *cutfile) {
    cutfile->basename = tofe_basename(cutfile->filename);
    if(!cutfile->basename) {
        tofe_list_msg(TOFE_LIST_ERROR, "Could not parse filename \"%s\"\n", cutfile->filename);
        return -1;
    }
    char *ext = cutfile->basename + strcspn(cutfile->basename, "."); /* all extensions, i.e. string after first occurence of '.' */
    if(*ext != '\0') {
        ext++;
        char *nuclide = strdup(ext);
        nuclide[strcspn(nuclide, ".")] = 0; /* Replaces first occurence of '.' with '\0', terminating the string, i.e. we extract foo from blabla.foo.blabla... */
        char *element;
        cutfile->A = strtol(nuclide, &element, 10);
        if(cutfile->A < 0 || cutfile->A > 300) {
            tofe_list_msg(TOFE_LIST_ERROR, "Mass number out of range or conversion error with \"%s\"\n", nuclide);
            return -1;
        }
        if(*element != '\0') {
            free(cutfile->element_str);
            cutfile->element_str = strdup(element); /* Will be parsed later */
        } else {
            tofe_list_msg(TOFE_LIST_ERROR, "Could not parse element from filename \"%s\"", cutfile->filename);
            return -1;
        }
        free(nuclide);
    }
    return 0;
}

int cutfile_read_headers(cutfile *cutfile) {
    char *line = NULL, *line_data;
    size_t line_size = 0;
    FILE *f = fopen(cutfile->filename, "r");
    int lineno = 0;
    if(!f) {
        tofe_list_msg(TOFE_LIST_ERROR, "Could not open file \"%s\"", cutfile->filename);
        return -1;
    }
    int error = 0;
    while(getline(&line, &line_size, f) > 0) {
        lineno++;
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
                          lineno, line);
            error++;
            break;
        }
        while(*(line_data) == ' ') { line_data++; }
        if(*line_data == '\0') {
            tofe_list_msg(TOFE_LIST_WARNING, "File %s line %i: \"%s\", argument is empty.", cutfile->filename,
                          lineno, line);
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
                tofe_list_msg(TOFE_LIST_WARNING, "File %s line %i: unknown header \"%s\"", cutfile->filename, lineno, line);
                break;
        }
    }
    free(line);
    cutfile->header_lines = lineno;
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
    jibal_element_free(cutfile->element);
}

int cutfile_convert(FILE *out, const cutfile *cutfile) {
    /* Load stopping of recoils/scattered ions in carbon foil */
    /* Load relevant efficiency file */
    /* Skip headers */
    /* Convert ToF to E and add back energy lost in foil */
    /* Convert position coordinate to angle */
    /* Create properly formatted output */
    /* Check that number of events matches with the header */
    FILE *in = fopen(cutfile->filename, "r");
    if(!in) {
        tofe_list_msg(TOFE_LIST_ERROR, "Could not open file \"%s\" (reading for conversion)!\n", cutfile->filename);
        return -1;
    }
    char *line = NULL;
    size_t line_size = 0;
    int lineno = 0;
    int Z = cutfile->element->Z;
    double mass = cutfile->element->avg_mass / C_U;
    double weight_cutfile = cutfile->event_weight;
    const char *type_str = tofe_scatter_types[cutfile->type].s;
    while(getline(&line, &line_size, in) > 0) {
        lineno++;
        if(lineno <= cutfile->header_lines) {
            continue;
        }
        if(strncmp(line, "ToF, Energy, Event number", 25) == 0) { /* TODO: this is for backward compatibility, consider removing! */
            continue;
        }
        int tof, e, ang1, evnum;
        if(sscanf(line, "%i %i %i %i", &tof, &e, &ang1, &evnum) == 4) {
            //angle1=ang1*input.acalib1+input.acalib2;
        } else if(sscanf(line, "%i %i %i", &tof, &e, &evnum) == 3) {
            //angle1=0.0;
        } else {
            tofe_list_msg(TOFE_LIST_ERROR, "Error in scanning input file %s on line %i: %s", cutfile->filename, lineno, line);
            break;
        }

        double weight = weight_cutfile; /* TODO: efficiency correction */
        double energy = 1.0; /* TODO: energy conversion (+ stopping) */
        //0.000000e+00 0.000000e+00    2.98967  14  28.0855 ERD  1.000 947984
        fprintf(out, "%12e %12e %8.5lf %3i %8.4lf %4s %.5lf %i\n", 0.0, 0.0, 1.0, Z, mass, type_str, weight, evnum);
    }
    fclose(in);
    return 0;
}

list_files *tofe_files_from_argv(jibal *jibal, int argc, char **argv) { /* Argument vector should contain filenames. Reads headers, closes files. */
    if(argc == 0 || argv == NULL) {
        tofe_list_msg(TOFE_LIST_ERROR, "No files.");
        return NULL;
    }
    list_files *files = malloc(sizeof(list_files));
    files->n_files = argc;
    files->cutfiles = calloc(files->n_files, sizeof(cutfile));
    int error = 0;
    for(int i = 0; i < argc; i++) {
        cutfile *cutfile = &files->cutfiles[i];
        cutfile_reset(cutfile);
        cutfile->filename = strdup(argv[i]);
        if(cutfile_parse_filename(cutfile)) {
            error = 1;
            break;
        }
        if(cutfile_read_headers(cutfile)) { /* Encountered some error, cleanup and return */
            error = 1;
            break;
        }
        if(!cutfile->element_str) {
            tofe_list_msg(TOFE_LIST_ERROR, "No element could be determined from filename \"%s\"", cutfile->filename);
        }
        const jibal_element *element = jibal_element_find(jibal->elements, cutfile->element_str);
        if(!element) {
            tofe_list_msg(TOFE_LIST_ERROR, "File %s: element \"%s\" not found", cutfile->filename, cutfile->element_str) ;
        }
        cutfile->element = jibal_element_copy(element, cutfile->A);
        if(!cutfile->element) {
            tofe_list_msg(TOFE_LIST_ERROR, "File %s: element %s, Z = %i with A = %i could not be created (JIBAL issue?).", cutfile->filename, element->name, cutfile->A); ;
        }
    }
    if(error) {
        tofe_files_free(files);
        return NULL;
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
    if(!basename_r(path, bname)) {
        free(bname);
        return NULL;
    }
    return bname;
#endif
}

void tofe_files_print(list_files *files) {
    if(!files) {
        return;
    }
    fprintf(stderr, "%32s | type |   A |   Z |    mass | weight | head |   counts\n", "filename");
    for(size_t i = 0; i < files->n_files; i++) {
        cutfile *cutfile = &files->cutfiles[i];
        fprintf(stderr, "%32s | %4s | %3i | %3i | %7.4lf | %6.3lf | %4i | %8zu\n",
                cutfile->basename, tofe_scatter_types[cutfile->type].s, cutfile->A,
                cutfile->element->Z,  cutfile->element->avg_mass / C_U,
                cutfile->event_weight, cutfile->header_lines, cutfile->n_counts);
    }
}

int tofe_files_convert(list_files *files) {
    for(size_t i = 0; i < files->n_files; i++) {
        cutfile_convert(stdout, &files->cutfiles[i]);
    }
    return 0;
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
    jibal *jibal = jibal_init(NULL);
    list_files *files = tofe_files_from_argv(jibal, argc, argv);
    if(!files) {
        return EXIT_FAILURE;
    }
    tofe_files_print(files);
    tofe_files_convert(files);
    tofe_files_free(files);
    jibal_free(jibal);
    return EXIT_SUCCESS;
}
