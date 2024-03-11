#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <jibal.h>

typedef enum scatter_type {
    SCATTER_NONE = 0,
    SCATTER_ERD = 1,
    SCATTER_RBS = 2
} scatter_type;

typedef struct tofe_list_event {
    int tof;
    int e;
    int ang;
    int eventno;
    double energy;
    double angle1;
    double angle2;
} tofe_list_event;

typedef struct cutfile {
    char *filename;
    size_t n_counts;
    scatter_type type;
    double beam_energy;
    jibal_isotope *incident;
    double event_weight;
    int lineno;
} cutfile;

typedef struct list_files {
    cutfile *cutfiles;
    size_t n_files;
} list_files;

typedef enum {
    CUTFILE_HEADER_NONE = 0,
    CUTFILE_HEADER_TYPE = 1
} cutfile_header_type;

static const jibal_option cutfile_headers[] = {
        {JIBAL_OPTION_STR_NONE, CUTFILE_HEADER_NONE},
        {"type",                CUTFILE_HEADER_TYPE},
        {NULL,                  0}
};

typedef enum tofe_list_msg_level {
    TOFE_LIST_NORMAL = 0,
    TOFE_LIST_INFO = 1,
    TOFE_LIST_WARNING = 2,
    TOFE_LIST_ERROR = 3
} tofe_list_msg_level;

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
        free(files->cutfiles[i].filename);
    }
    free(files->cutfiles);
    free(files);
}

int cutfile_read_headers(cutfile *cutfile) {
    char *line = NULL, *line_data;
    size_t line_size = 0;
    FILE *f = fopen(cutfile->filename, "r");
    if(!f) {
        fprintf(stderr, "Could not open file \"%s\"", cutfile->filename);
        return -1;
    }
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
    }
    fclose(f);
    return error;
}

list_files *tofe_files_allocate_from_argv(int argc, char **argv) {
    list_files *files = malloc(sizeof(list_files));
    files->n_files = argc;
    files->cutfiles = calloc(files->n_files, sizeof(cutfile));
    for(int i = 0; i < argc; i++) {
        cutfile *cutfile = &files->cutfiles[i];
        cutfile->filename = strdup(argv[i]);
        if(cutfile_read_headers(cutfile)) { /* Encountered some error, cleanup and return */
            tofe_files_free(files);
            files = NULL;
            break;
        }
    }
    return files;
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
    list_files *files = tofe_files_allocate_from_argv(argc, argv);
    tofe_files_free(files);
    return 0;
}
