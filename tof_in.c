#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tof_in.h"


int tofin_file_parse_header(tofin_file *tofin, const char *header, const char *data) {
    char *end;
    switch (jibal_option_get_value(tofin_headers, header)) {
        case TOFIN_HEADER_BEAM:
            break;
        case TOFIN_HEADER_ENERGY:
            break;
        case TOFIN_HEADER_DETECTOR_ANGLE:
            break;
        case TOFIN_HEADER_TARGET_ANGLE:
            break;
        case TOFIN_HEADER_TOFLEN:
            tofin->toflen = strtod(data, &end);
            if(end == data) {
                fprintf(stderr, "ToF length could not be parsed.\n");
            }
            break;
        case TOFIN_HEADER_CARBON_FOIL_THICKNESS:
            break;
        case TOFIN_HEADER_TARGET_DENSITY:
            break;
        case TOFIN_HEADER_TOF_CALIBRATION:
            if(sscanf(data, "%lf %lf", &tofin->tof_slope, &tofin->tof_offset) != 2) {
                fprintf(stderr, "ToF calibration could not be parsed.\n");
                return -1;
            }
            break;
        case TOFIN_HEADER_ANGLE_CALIBRATION:
            if(sscanf(data, "%lf %lf", &tofin->angle_slope, &tofin->angle_offset) != 2) {
                fprintf(stderr, "Angle calibration could not be parsed.\n");
                return -1;
            }
            break;
        case TOFIN_HEADER_DEPTH_STEPS:
            break;
        case TOFIN_HEADER_DEPTH_STEP_STOPPING:
            break;
        case TOFIN_HEADER_DEPTH_STEP_OUTPUT:
            break;
        case TOFIN_HEADER_DEPTHS_CONCENTRATION_SCALING:
            break;
        case TOFIN_HEADER_CROSS_SECTION:
            break;
        case TOFIN_HEADER_ITERATIONS:
            break;
        case TOFIN_HEADER_EFFICIENCY_DIRECTORY:
            free(tofin->efficiency_directory);
            tofin->efficiency_directory = strdup(data);
            break;
        case TOFIN_HEADER_NONE:
        default:
            fprintf(stderr, "Warning: unknown header \"%s\"", header);
            break;
    }
    return 0;
}

tofin_file *tofin_file_load(const char *filename) {
    FILE *f = fopen(filename, "r");
    if(!f) {
        return NULL;
    }
    tofin_file *tofin = calloc(1, sizeof(tofin_file));
    char *line = NULL, *line_data;
    size_t line_size = 0;
    int lineno = 0;
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
        while(*line_data == ' ') {line_data++;} /* Skip whitespace */
        if(tofin_file_parse_header(tofin, line, line_data)) {
            fprintf(stderr, "Parse error on line %i of file \"%s\".\n", lineno, filename);
            error++;
            break;
        }
    }
    if(error) {
        tofin_file_free(tofin);
        return NULL;
    }
    free(line);
    fclose(f);
    fprintf(stderr, "Read %i lines from \"%s\".\n", lineno, filename);
    return tofin;
}

void tofin_file_free(tofin_file *tofin) {
    free(tofin->efficiency_directory);
    free(tofin);
}
