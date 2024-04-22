#ifndef TOFE_LIST_H
#define TOFE_LIST_H

#include <jibal_option.h>

#define EFFICIENCY_FILE_POINTS_INITIAL_ALLOC (1024)

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

typedef struct efficiencypoint {
    double E;
    double eff;
} efficiencypoint;

typedef struct efficiencyfile {
    char *basename;
    efficiencypoint *p;
    size_t n_points;
} efficiencyfile;

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
    efficiencyfile *ef;
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


void tofe_files_free(list_files *files);
int cutfile_parse_filename(jibal *jibal, cutfile *cutfile);
int cutfile_parse_extensions(jibal *jibal, cutfile *cutfile, char **extensions, int n_ext);
int cutfile_read_headers(cutfile *cutfile);
void cutfile_reset(cutfile *cutfile);
void cutfile_free(cutfile *cutfile);
int cutfile_convert(jibal *jibal, FILE *out, const tofin_file *tofin, const cutfile *cutfile, const jibal_material *foil);
list_files *tofe_files_from_argv(jibal *jibal, const tofin_file *tofin, int argc, char **argv);
char *tofe_basename(const char *path);
void tofe_files_print(list_files *files);
int tofe_files_assign_stopping(jibal *jibal, const list_files *files, const jibal_material *foil);
int tofe_files_convert(jibal *jibal, const tofin_file *tofin, list_files *files, const jibal_material *foil);
double energy_from_tof(const tofin_file *tofin, int ch, double mass);
efficiencyfile *efficiencyfile_load(const char *filename);
void efficiencyfile_free(efficiencyfile *ef);
int efficiencyfile_points_realloc(efficiencyfile *ef, size_t n_points);
double efficiencyfile_get_weight(efficiencyfile *ef, double E);
char *cutfile_efficiencyfile_name(const cutfile *cutfile, const tofin_file *tofin);
int cutfile_load_efficiencyfile(cutfile *cutfile, const tofin_file *tofin);
#endif //TOFE_LIST_H
