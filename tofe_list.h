#ifndef TOFE_LIST_H
#define TOFE_LIST_H

#include <jibal_option.h>

typedef enum scatter_type {
    SCATTER_NONE = 0,
    SCATTER_ERD = 1,
    SCATTER_RBS = 2
} scatter_type;

static const jibal_option tofe_scatter_types[] = {
        {JIBAL_OPTION_STR_NONE, SCATTER_NONE},
        {"ERD", SCATTER_ERD},
        {"RBS", SCATTER_RBS},
        {0, 0}
};

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
    char *basename; /* File without full path */
    size_t n_counts;
    scatter_type type;
    double beam_energy;
    jibal_isotope *incident;
    double event_weight;
    int header_lines;
    jibal_element *element; /* parsed element (in telescope), contains all isotopes with relevant concentrations */
    jibal_element *element_sample; /* element (in sample)  */
} cutfile;

typedef struct list_files {
    cutfile *cutfiles;
    size_t n_files;
} list_files;

typedef enum {
    CUTFILE_HEADER_NONE = 0,
    CUTFILE_HEADER_TYPE = 1,
    CUTFILE_HEADER_COUNT = 2,
    CUTFILE_HEADER_WEIGHT = 3,
    CUTFILE_HEADER_ENERGY = 4,
    CUTFILE_HEADER_ANGLE = 5,
    CUTFILE_HEADER_ELEMENT = 6,
    CUTFILE_HEADER_LOSSES = 7,
    CUTFILE_HEADER_SPLIT = 8,
} cutfile_header_type;

static const jibal_option cutfile_headers[] = {
        {JIBAL_OPTION_STR_NONE, CUTFILE_HEADER_NONE},
        {"Type",                CUTFILE_HEADER_TYPE},
        {"Count",               CUTFILE_HEADER_COUNT},
        {"Weight Factor",       CUTFILE_HEADER_WEIGHT},
        {"Energy",              CUTFILE_HEADER_ENERGY},
        {"Detector Angle",      CUTFILE_HEADER_ANGLE},
        {"Scatter Element",     CUTFILE_HEADER_ELEMENT},
        {"Element losses",      CUTFILE_HEADER_LOSSES},
        {"Split count",         CUTFILE_HEADER_SPLIT},
        {0,                  0}
};

typedef enum tofe_list_msg_level {
    TOFE_LIST_NORMAL = 0,
    TOFE_LIST_INFO = 1,
    TOFE_LIST_WARNING = 2,
    TOFE_LIST_ERROR = 3
} tofe_list_msg_level;

const char *msg_level_str(tofe_list_msg_level level);
void tofe_list_msg(tofe_list_msg_level level, const char *restrict format, ...);
void tofe_files_free(list_files *files);
int cutfile_parse_filename(jibal *jibal, cutfile *cutfile);
int cutfile_parse_extensions(jibal *jibal, cutfile *cutfile, char **extensions, int n_ext);
int cutfile_read_headers(cutfile *cutfile);
void cutfile_reset(cutfile *cutfile);
void cutfile_free(cutfile *cutfile);
int cutfile_convert(FILE *out, const cutfile *cutfile);
list_files *tofe_files_from_argv(jibal *jibal, int argc, char **argv);
char *tofe_basename(const char *path);
void tofe_files_print(list_files *files);
int tofe_files_convert(list_files *files);
int tofe_files_assign_stopping(jibal *jibal, const list_files *files, const jibal_material *foil);
#endif //TOFE_LIST_H
