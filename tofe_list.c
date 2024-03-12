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

int cutfile_set_elements(jibal *jibal, cutfile *cutfile, const char *element_str, int A, const char *rbs_element_str, int rbs_A) {
#ifdef DEBUG
    fprintf(stderr, "cutfile_set_elements(%p, %p, %s, %i, %s, %i)\n", jibal, cutfile, element_str, A, rbs_element_str, rbs_A);
#endif
    const jibal_element *element = jibal_element_find(jibal->elements, element_str);
    if(!element) {
        tofe_list_msg(TOFE_LIST_ERROR, "File %s: element \"%s\" not found", cutfile->filename, element_str);
        return -1;
    }
    cutfile->element = jibal_element_copy(element, A);
    if(!cutfile->element) {
        tofe_list_msg(TOFE_LIST_ERROR, "File %s: element %s, Z = %i with A = %i could not be created (JIBAL issue?).", cutfile->filename, element->name, A); ;
    }
    if(rbs_element_str) {
        const jibal_element *element_sample = jibal_element_find(jibal->elements, element_str);
        element_sample = jibal_element_find(jibal->elements, rbs_element_str);
        cutfile->element_sample = jibal_element_copy(element_sample, rbs_A);
    } else {
        cutfile->element_sample = jibal_element_copy(cutfile->element, -1);
    }
    return 0;
}

int cutfile_parse_extensions(jibal *jibal, cutfile *cutfile, char **extensions, int n_ext) {
    if(n_ext < 3) {
        tofe_list_msg(TOFE_LIST_ERROR, "Filename \"%s\" does not have sufficient number of extensions.\n", cutfile->filename);
        return -1;
    }
    char *nuclide = extensions[1];
    char *element_str; /* Follows mass number (which is optional) */
    int A = strtol(nuclide, &element_str, 10);
    int A_rbs = 0;
    if(A < 0 || A > 300) {
        tofe_list_msg(TOFE_LIST_ERROR, "Mass number out of range or conversion error with \"%s\"\n", nuclide);
        return -1;
    }
    if(*element_str == '\0') {
        tofe_list_msg(TOFE_LIST_ERROR, "Could not parse element from filename \"%s\"", cutfile->filename);
        return -1;
    }

    size_t i_sep = strcspn(extensions[2], "_"); /* Try to find underscore */
    char *rbs_element_str = extensions[2] + i_sep;
    if(*rbs_element_str == '_') { /* Underscore found, skip it */
        *rbs_element_str = 0; /* extensions[2] should now have only "RBS" */
        rbs_element_str++;
        A_rbs = strtol(rbs_element_str, &rbs_element_str, 10);
    } else { /* Underscore not found, probably ERD */
        rbs_element_str = NULL;
    }
    cutfile->type = jibal_option_get_value(tofe_scatter_types, extensions[2]);
    if(cutfile->type == SCATTER_NONE) {
        tofe_list_msg(TOFE_LIST_ERROR, "Could not parse type (reaction, RBS or ERD) of cutfile from \"%s\"\n", cutfile->filename);
        return -1;
    }
    if(cutfile->type == SCATTER_RBS && rbs_element_str == NULL) {
        tofe_list_msg(TOFE_LIST_ERROR, "Cutfile claims to be RBS, but there is no sample element given. File: \"%s\"\n", cutfile->filename);
        return -1;
    }

    if(cutfile->type == SCATTER_ERD && rbs_element_str != NULL) {
        tofe_list_msg(TOFE_LIST_ERROR, "Cutfile claims to be ERD, but there a sample element given. File: \"%s\"\n", cutfile->filename);
        return -1;
    }
    cutfile_set_elements(jibal, cutfile, element_str, A, rbs_element_str, A_rbs);
    return 0;
}


int cutfile_parse_filename(jibal *jibal, cutfile *cutfile) {
    cutfile->basename = tofe_basename(cutfile->filename);
    if(!cutfile->basename) {
        tofe_list_msg(TOFE_LIST_ERROR, "Could not parse filename \"%s\"\n", cutfile->filename);
        return -1;
    }
    char *basename = strdup(cutfile->basename);
    char *basename_orig = basename;
    char *extensions[6], **ext;
    int n_ext = 0;
    for(ext = extensions; (*ext = strsep(&basename, ".")) != NULL;) { /* Make an array of file extensions */
        if(**ext != '\0') {
            n_ext++;
            if(++ext >= &extensions[6]) {
                break;
            }
        }
    }
#ifdef DEBUG
    for(int i = 0; i < n_ext; i++) {
        fprintf(stderr, "extensions[%i] = %s\n", i, extensions[i]);
    }
#endif
    cutfile_parse_extensions(jibal, cutfile, extensions, n_ext);
    free(basename_orig);
    return 0;
}

int cutfile_parse_header(cutfile *cutfile, int lineno, const char *header, const char *data) {
    char *tmp;
    int type;
    switch (jibal_option_get_value(cutfile_headers, header)) {
        case CUTFILE_HEADER_NONE:
            break;
        case CUTFILE_HEADER_TYPE:
            type = jibal_option_get_value(tofe_scatter_types, data);
            if(type != cutfile->type) {
                tofe_list_msg(TOFE_LIST_ERROR, "Cutfile \"%s\" type %s given in header does not match type given in filename.", cutfile->filename, data);
                return -1;
            }
            break;
        case CUTFILE_HEADER_COUNT:
            cutfile->n_counts = strtol(data, NULL, 10);
            break;
        case CUTFILE_HEADER_WEIGHT:
            cutfile->event_weight = strtod(data, &tmp);
            if(*tmp != '\0') {
                tofe_list_msg(TOFE_LIST_WARNING, "Trailing characters in event weight: %s (got %g) in file \"%s\", was conversion successful?", data, cutfile->event_weight, cutfile->filename);
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
            tofe_list_msg(TOFE_LIST_WARNING, "File %s line %i: unknown header \"%s\"", cutfile->filename, lineno, header);
            break;
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
    int type = SCATTER_NONE;
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
#ifdef DEBUG
        fprintf(stderr, "%i: \"%s\" and \"%s\"\n", cutfile->lineno, line, line_data);
#endif
        if(cutfile_parse_header(cutfile, lineno, line, line_data)) {
            error++;
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
    jibal_element_free(cutfile->element_sampl);
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
        //fprintf(out, "%12e %12e %8.5lf %3i %8.4lf %4s %.5lf %i\n", 0.0, 0.0, 1.0, Z, mass, type_str, weight, evnum);
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
        if(cutfile_parse_filename(jibal, cutfile)) {
            error = 1;
            break;
        }
        if(cutfile_read_headers(cutfile)) { /* Encountered some error, cleanup and return */
            error = 1;
            break;
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
    fprintf(stderr, "%32s | type | isotopes |   Z |    mass | sample element | weight | headers |   counts\n", "filename");
    for(size_t i = 0; i < files->n_files; i++) {
        cutfile *cutfile = &files->cutfiles[i];
        fprintf(stderr, "%32s | %4s |   %6zu | %3i | %7.4lf | %14s | %6.3lf | %7i | %8zu\n",
                cutfile->basename, tofe_scatter_types[cutfile->type].s, cutfile->element->n_isotopes,
                cutfile->element->Z,  cutfile->element->avg_mass / C_U, cutfile->element_sample->name,
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
#ifdef DEBUG
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