#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <jibal.h>
#include <jibal_stop.h>
#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "win_compat.h"
#else
#include <libgen.h> /* for basename() */
#include <sys/param.h> /* for MAXPATHLEN */
#endif
#include "message.h"
#include "tof_in.h"
#include "tofe_list.h"

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
        const jibal_element *element_sample = jibal_element_find(jibal->elements, rbs_element_str);
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
        case CUTFILE_HEADER_NONE:
        default:
            tofe_list_msg(TOFE_LIST_WARNING, "File %s line %i: unknown header \"%s\"", cutfile->filename, lineno, header);
            break;
    }
    return 0;
}

int cutfile_read_headers(cutfile *cutfile) {
    char *line = NULL, *line_data;
    size_t line_size = 0;
    int lineno = 0;
    FILE *f = fopen(cutfile->filename, "r");
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
    efficiencyfile_free(cutfile->ef);
    jibal_element_free(cutfile->element);
    jibal_element_free(cutfile->element_sample);
}

double tofelist_stop(jibal_gsto *workspace, int Z1, double mass, const jibal_material *target, double E) {
    double sum = 0.0;
    double em = E/mass;
    for (int i = 0; i < target->n_elements; i++) {
        jibal_element *element = &target->elements[i];
        sum += target->concs[i]*(jibal_gsto_get_em(workspace, GSTO_STO_ELE, Z1, element->Z, em) + jibal_gsto_stop_nuclear_universal(E, Z1, mass, element->Z, element->avg_mass));
    }
   //fprintf(stderr, "Stop of Z1 = %i, mass = %g u at E = %g keV is %g eV/tfu\n", Z1, mass / C_U, E / C_KEV, sum / C_EV_TFU);
    return sum;
}

int cutfile_convert(jibal *jibal, FILE *out, const tofin_file *tofin, const cutfile *cutfile, const jibal_material *foil) {
    FILE *in = fopen(cutfile->filename, "r");
    if(!in) {
        tofe_list_msg(TOFE_LIST_ERROR, "Could not open file \"%s\" (reading for conversion)!\n", cutfile->filename);
        return -1;
    }
    char *line = NULL;
    size_t line_size = 0;
    int lineno = 0;
    const int Z_out = cutfile->element_sample->Z;
    const double mass_out = cutfile->element_sample->avg_mass / C_U;
    double weight_cutfile = cutfile->event_weight;
    const char *type_str = tofe_scatter_types[cutfile->type].s;
    size_t n_counts = 0;
    double weight_avg = 0.0;
    while(getline(&line, &line_size, in) > 0) {
        lineno++;
        if(lineno <= cutfile->header_lines) {
            continue;
        }
        if(strncmp(line, "ToF, Energy, Event number", 25) == 0) { /* TODO: this is for backward compatibility, consider removing! */
            continue;
        }
        int tof, e, ang1, evnum;
        double angle1, angle2 = 0.0;
        if(sscanf(line, "%i %i %i %i", &tof, &e, &ang1, &evnum) == 4) {
            angle1 = ang1 * tofin->angle_slope + tofin->angle_offset;
        } else if(sscanf(line, "%i %i %i", &tof, &e, &evnum) == 3) {
            angle1 = 0.0;
        } else {
            tofe_list_msg(TOFE_LIST_ERROR, "Error in scanning input file %s on line %i: %s", cutfile->filename, lineno, line);
            break;
        }
        double energy = energy_from_tof(tofin, tof, cutfile->element->avg_mass);
        double stop = tofelist_stop(jibal->gsto, cutfile->element->Z, cutfile->element->avg_mass, foil, energy) * tofin->foil_thickness;
        energy += stop;

        double weight = weight_cutfile;
        if(cutfile->ef) {
            double eff = efficiencyfile_get_weight(cutfile->ef, energy);
            if(eff < 1e-6) {
                tofe_list_msg(TOFE_LIST_WARNING, "Efficiency %g too low, setting weight of count (event number = %i, line number = %i) in file %s to zero.\n", eff, evnum, lineno, cutfile->basename);
                weight = 0.0;
            } else {
                weight *= 1.0 / eff;
            }
        }
        weight_avg += weight;

        fprintf(out, "%8.5lf %8.5lf %8.5lf %3i %8.4lf %3s %8.5lf %8i\n", angle1, angle2, energy / C_MEV, Z_out, mass_out, type_str, weight, evnum);
        n_counts++;
    }
    fclose(in);
    free(line);
    if(n_counts != cutfile->n_counts) {
        tofe_list_msg(TOFE_LIST_ERROR, "Number of counts expected in file \"%s\" was %zu, but I got %zu.", cutfile->filename, cutfile->n_counts, n_counts);
        return -1;
    }
    weight_avg /= (double)n_counts;
    tofe_list_msg(TOFE_LIST_INFO, "File \"%s\": converted %zu counts, cutfile weight %.5lf, average weight %.5lf, ratio of these %.5lf", cutfile->basename, n_counts, cutfile->event_weight, weight_avg, weight_avg/cutfile->event_weight);
    return 0;
}

list_files *tofe_files_from_argv(jibal *jibal, const tofin_file *tofin, int argc, char **argv) { /* Argument vector should contain filenames. Reads headers, closes files. */
    if(argc == 0 || argv == NULL) {
        tofe_list_msg(TOFE_LIST_ERROR, "No files.");
        return NULL;
    }
    list_files *files = malloc(sizeof(list_files));
    files->n_files = argc;
    files->cutfiles = calloc(files->n_files, sizeof(cutfile));
    if(!files->cutfiles) {
        return NULL;
    }
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
        if(cutfile_load_efficiencyfile(cutfile, tofin)) {
            tofe_list_msg(TOFE_LIST_ERROR, "Error while loading efficiency file for cutfile \"%s\" (%s)\n", cutfile->filename, cutfile->element->name);
            return NULL;
        }
    }
    if(error) {
        tofe_files_free(files);
        return NULL;
    }
    return files;
}

char *tofe_basename(const char *path) { /*  e.g. /bla/bla/tofe2363.O.ERD.0.cut => tofe2363.O.ERD.0.cut  */
    if(!path || *path == 0) {
        return NULL;
    }
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
    char *path_copy = strdup(path);
    char *bname = basename(path_copy); /* basename can't be trusted, take a copy */
    if(!bname) {
        free(path_copy);
        return NULL;
    }
    char *bname_copy = strdup(bname); /* caller should free memory, but basename may return a pointer to static memory or do something silly. We'll take a copy. */
    free(path_copy); /* Now it is safe to remove this */
    return bname_copy;
#endif
}

void tofe_files_print(list_files *files) {
    if(!files) {
        return;
    }
    int longest_name = 0;
    for(size_t i = 0; i < files->n_files; i++) {
        cutfile *cutfile = &files->cutfiles[i];
        int len = strlen(cutfile->basename);
        longest_name = len > longest_name ? len : longest_name;
    }
    fprintf(stderr, "%*s | type | element | isotopes |   Z |    mass | sample element | weight | eff? | headers |   counts\n",
            longest_name, "filename");
    for(size_t i = 0; i < files->n_files; i++) {
        cutfile *cutfile = &files->cutfiles[i];
        fprintf(stderr, "%*s | %4s | %7s |  %7zu | %3i | %7.4lf | %14s | %6.3lf | %4s | %7i | %8zu\n", longest_name,
                cutfile->basename, tofe_scatter_types[cutfile->type].s, cutfile->element->name,
                cutfile->element->n_isotopes, cutfile->element->Z,  cutfile->element->avg_mass / C_U,
                cutfile->element_sample->name, cutfile->event_weight, cutfile->ef?"yes":"no", cutfile->header_lines,
                cutfile->n_counts);
    }
}

int tofe_files_convert(jibal *jibal, const tofin_file *tofin, list_files *files, const jibal_material *foil) {
    size_t n_eff = 0;
    size_t eff_str_len = 1; /* must have at least terminating \0 */
    char *eff_str = NULL;
    for(size_t i = 0; i < files->n_files; i++) {
        cutfile *cutfile =  &files->cutfiles[i];
        if(cutfile_convert(jibal, stdout, tofin, cutfile, foil)) {
            tofe_list_msg(TOFE_LIST_ERROR, "Error while processing cutfile \"%s\"\n", cutfile->filename);
            return -1;
        }
        if(cutfile->ef) {
            n_eff++;
            eff_str_len += strlen(cutfile->ef->basename) + 3; /* +3 to account for spaces between names and quotes around them */
        }
    }
    if(n_eff == 0) {
        return 0;
    }
    eff_str = calloc(eff_str_len, sizeof(char));
    if(!eff_str) {
        return -1;
    }
    for(size_t i = 0; i < files->n_files; i++) {
        cutfile *cutfile =  &files->cutfiles[i];
        if(cutfile->ef) {
            strcat(eff_str, " \""); /* 2 characters */
            strcat(eff_str, cutfile->ef->basename);
            strcat(eff_str, "\""); /* 1 character */
        }
    }
    tofe_list_msg(TOFE_LIST_INFO, "Used efficiency files (%zu):%s", n_eff, eff_str);
    free(eff_str);
    return 0;
}

int tofe_files_assign_stopping(jibal *jibal, const list_files *files, const jibal_material *foil) {
    for(size_t i = 0; i < files->n_files; i++) {
        const cutfile *cutfile = &files->cutfiles[i];
        for(size_t i_elem = 0; i_elem < foil->n_elements; i_elem++) {
            const jibal_element *foil_element = &foil->elements[i_elem];
            if(!jibal_gsto_auto_assign(jibal->gsto, cutfile->element->Z, foil_element->Z)) {
                tofe_list_msg(TOFE_LIST_ERROR, "Assignment of GSTO stopping Z1 = %i, Z2 = %i fails.\n", cutfile->element->Z, foil_element->Z);
                return -1;
            }
        }
    }
    return 0;
}

double energy_from_tof(const tofin_file *tofin, int ch, double mass) {
    double tof = ((double)ch + ((double)(rand())/RAND_MAX) - 0.5) * tofin->tof_slope + tofin->tof_offset;
    //fprintf(stdout, "tof = %g s, ch = %i mass = %g\n", tof, ch, mass / C_U);
    if(tof < 1.0 * C_NS) {
        return 0.0;
    }
    return 0.5 * mass * pow2(tofin->toflen/tof);
}

int efficiencyfile_points_realloc(efficiencyfile *ef, size_t n_points) {
    if(!ef) {
        return -1;
    }
    if(n_points == ef->n_points) {
        return 0;
    }
    efficiencypoint *p_new  = realloc(ef->p, n_points * sizeof(efficiencypoint));
    if(!p_new) {
        return -1;
    }
    ef->p = p_new;
    ef->n_points = n_points;
    return 0;
}

efficiencyfile *efficiencyfile_load(const char *filename) {
    FILE *f = fopen(filename, "r");
    if(!f) { /* Fail silently; the file might not exist */
        return NULL;
    }
    efficiencyfile *ef = calloc(1, sizeof(efficiencyfile));
    ef->basename = tofe_basename(filename);
    char *line = NULL;
    size_t line_size = 0;
    size_t n = 0;
    if(efficiencyfile_points_realloc(ef, EFFICIENCY_FILE_POINTS_INITIAL_ALLOC)) {
        tofe_list_msg(TOFE_LIST_WARNING, "Initial allocation of efficiency file failed. This shouldn't happen.");
        return NULL;
    }
    double eff_avg = 0.0;
    while(getline(&line, &line_size, f) > 0) {
        if(*line == '#') { /* Comment */
            continue;
        }
        line[strcspn(line, "\r\n")] = 0; /* Strips all kinds of newlines! */
        if(*line == '\0') { /* Empty line */
            continue;
        }
        if(n >= ef->n_points) {
            if(efficiencyfile_points_realloc(ef, ef->n_points * 2)) {
                tofe_list_msg(TOFE_LIST_WARNING, "Allocation of efficiency file failed (old n = %zu)", ef->n_points);
                return NULL;
            }
        }
        if(sscanf(line, "%lf %lf", &ef->p[n].E, &ef->p[n].eff) != 2) {
            break;
        }
        ef->p[n].E *= C_MEV;
        eff_avg += ef->p[n].eff;
        n++;
    }
    if(!feof(f)) {
        tofe_list_msg(TOFE_LIST_WARNING, "Reading of efficiency file \"%s\" was aborted before the end was reached.\n");
        efficiencyfile_free(ef);
        return NULL;
    }
    eff_avg /= (double) n;
    tofe_list_msg(TOFE_LIST_INFO, "Got %zu points from efficiency file \"%s\". Highest energy %g MeV. Simple arithmetic average of efficiencies: %g.", n, filename, ef->p[n-1].E / C_MEV, eff_avg);
    ef->n_points = n; /* We could also realloc to save unused space */
    return ef;
}
void efficiencyfile_free(efficiencyfile *ef) {
    if(!ef) {
        return;
    }
    free(ef->basename);
    free(ef->p);
    free(ef);
}

char *cutfile_efficiencyfile_name(const cutfile *cutfile, const tofin_file *tofin) {
    if(!cutfile || !tofin || !tofin->efficiency_directory) {
        return NULL;
    }
    char *filename = calloc(strlen(tofin->efficiency_directory) + 1 + strlen(cutfile->element->name) + 4 + 1, sizeof(char));
    strcat(filename, tofin->efficiency_directory);
    strcat(filename, "/");
    strcat(filename, cutfile->element->name);
    strcat(filename, ".eff");
    return filename;
}

int cutfile_load_efficiencyfile(cutfile *cutfile, const tofin_file *tofin) {
    char *eff_filename = cutfile_efficiencyfile_name(cutfile, tofin);
    if(!eff_filename) {
        tofe_list_msg(TOFE_LIST_ERROR, "Efficiency filename error.\n");
        return -1;
    }
    cutfile->ef = efficiencyfile_load(eff_filename);
    return 0;
}

double efficiencyfile_get_weight(efficiencyfile *ef, double E) {
    size_t lo = 0, mi, hi = ef->n_points - 1;
    while (hi - lo > 1) {
        mi = (hi + lo) / 2;
        if (E >= ef->p[mi].E) {
            lo = mi;
        } else {
            hi = mi;
        }
    }
    if(E > ef->p[ef->n_points - 1].E || E < ef->p[0].E) {
        tofe_list_msg(TOFE_LIST_WARNING, "Energy %g MeV is out of bounds for efficiency file. Assuming zero efficiency!\n", E / C_MEV);
        return 0.0;
    }
    double out = jibal_linear_interpolation(ef->p[lo].E, ef->p[lo + 1].E, ef->p[lo].eff, ef->p[lo + 1].eff, E);
#ifdef DEBUG
    fprintf(stderr, "Interpolation, E = %g keV = [%g keV, %g keV], eff = [%lf, %lf], output = %g\n", E / C_KEV, ef->p[lo].E / C_KEV, ef->p[lo + 1].E / C_KEV, ef->p[lo].eff, ef->p[lo + 1].eff, out);
#endif
    return out;
}


int main(int argc, char **argv) {
    tofe_list_msg(TOFE_LIST_INFO, "tofe_list (erd_depth) version %s\n", ERD_DEPTH_VERSION);
#ifdef DEBUG
    for(int i = 0; i < argc; i++) {
        fprintf(stderr, "tofe_list argv[%i] = %s\n", i, argv[i]);
    }
#endif
    if(argc < 3) {
        tofe_list_msg(TOFE_LIST_ERROR, "Not enough arguments. Usage: tofe_list <tof.in file> <cutfile1> <cutfile2> ...");
        return EXIT_FAILURE;
    }
    jibal *jibal = jibal_init(NULL);
    if(!jibal) {
        tofe_list_msg(TOFE_LIST_ERROR, "JIBAL initialization by tofe_list failed.");
        return EXIT_FAILURE;
    }
    argc--;
    argv++;
    tofin_file *tofin = tofin_file_load(argv[0]);
    if(!tofin) {
        tofe_list_msg(TOFE_LIST_ERROR, "Could not load or parse settings file \"%s\".", argv[0]);
        return EXIT_FAILURE;
    }
    argc--;
    argv++;
    list_files *files = tofe_files_from_argv(jibal, tofin, argc, argv);
    if(!files) {
        tofe_list_msg(TOFE_LIST_ERROR, "Error in reading cutfiles.");
        return EXIT_FAILURE;
    }
    tofe_files_print(files);
    jibal_material *foil = jibal_material_create(jibal->elements, "C");
    if(!foil) {
        tofe_list_msg(TOFE_LIST_ERROR, "Could not create carbon foil data structure. JIBAL issue?\n");
        return EXIT_FAILURE;
    }
    tofin->foil_thickness /= foil->elements[0].avg_mass; /* TODO: works only on monoelemental foils. Not a massive issue, giving foil thickness in ug/cm2 is quite stupid in this case. */
    tofe_list_msg(TOFE_LIST_INFO, "Carbon foil thickness %g tfu", tofin->foil_thickness / C_TFU);
    tofe_list_msg(TOFE_LIST_INFO, "ToF length %g m, calibration slope %.6lf ns/ch, offset %.3lf ns", tofin->toflen, tofin->tof_slope / C_NS, tofin->tof_offset / C_NS);
    if(tofe_files_assign_stopping(jibal, files, foil)) {
        tofe_list_msg(TOFE_LIST_ERROR, "Could not assign stopping. JIBAL issue?");
        return EXIT_FAILURE;
    }
    if(jibal_gsto_load_all(jibal->gsto) == 0) {
        tofe_list_msg(TOFE_LIST_ERROR, "Could not load stopping. JIBAL issue?");
        return EXIT_FAILURE;
    }
    jibal_gsto_print_assignments(jibal->gsto);
    tofe_list_msg(TOFE_LIST_INFO, "Starting conversion of %zu cutfiles.", files->n_files);
    tofe_files_convert(jibal, tofin, files, foil);
    jibal_material_free(foil);
    tofe_files_free(files);
    tofin_file_free(tofin);
    jibal_free(jibal);
    tofe_list_msg(TOFE_LIST_INFO, "Clean exit from tofe_list. Have a nice day.");
    return EXIT_SUCCESS;
}
