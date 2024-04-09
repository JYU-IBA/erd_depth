#ifndef TOF_IN_H
#define TOF_IN_H

#include <jibal_option.h>
#define CARBON_DENSITY

typedef struct tofin_file {
    double toflen;
    double foil_thickness; /* initially in ug/cm2, then in tfu */
    double tof_slope;
    double tof_offset;
    double angle_slope;
    double angle_offset;
    char *efficiency_directory;
} tofin_file;

typedef enum {
    TOFIN_HEADER_NONE = 0,
    TOFIN_HEADER_BEAM = 1,
    TOFIN_HEADER_ENERGY = 2,
    TOFIN_HEADER_DETECTOR_ANGLE = 3,
    TOFIN_HEADER_TARGET_ANGLE = 4,
    TOFIN_HEADER_TOFLEN = 5,
    TOFIN_HEADER_CARBON_FOIL_THICKNESS = 6,
    TOFIN_HEADER_TARGET_DENSITY = 7,
    TOFIN_HEADER_TOF_CALIBRATION = 8,
    TOFIN_HEADER_ANGLE_CALIBRATION = 9,
    TOFIN_HEADER_DEPTH_STEPS = 10,
    TOFIN_HEADER_DEPTH_STEP_STOPPING = 11,
    TOFIN_HEADER_DEPTH_STEP_OUTPUT = 12,
    TOFIN_HEADER_DEPTHS_CONCENTRATION_SCALING = 13,
    TOFIN_HEADER_CROSS_SECTION = 14,
    TOFIN_HEADER_ITERATIONS = 15,
    TOFIN_HEADER_EFFICIENCY_DIRECTORY = 16
} tofin_header_type;

static const jibal_option tofin_headers[] = {
        {JIBAL_OPTION_STR_NONE,              TOFIN_HEADER_NONE},
        {"Beam",                             TOFIN_HEADER_BEAM},
        {"Energy",                           TOFIN_HEADER_ENERGY},
        {"Detector angle",                   TOFIN_HEADER_DETECTOR_ANGLE},
        {"Target angle",                     TOFIN_HEADER_TARGET_ANGLE},
        {"Toflen",                           TOFIN_HEADER_TOFLEN},
        {"Carbon foil thickness",            TOFIN_HEADER_CARBON_FOIL_THICKNESS},
        {"Target density",                   TOFIN_HEADER_TARGET_DENSITY},
        {"TOF calibration",                  TOFIN_HEADER_TOF_CALIBRATION},
        {"Angle calibration",                TOFIN_HEADER_ANGLE_CALIBRATION},
        {"Number of depth steps",            TOFIN_HEADER_DEPTH_STEPS},
        {"Depth step for stopping",          TOFIN_HEADER_DEPTH_STEP_STOPPING},
        {"Depth step for output",            TOFIN_HEADER_DEPTH_STEP_OUTPUT},
        {"Depths for concentration scaling", TOFIN_HEADER_DEPTHS_CONCENTRATION_SCALING},
        {"Cross section",                    TOFIN_HEADER_CROSS_SECTION},
        {"Number of iterations",             TOFIN_HEADER_ITERATIONS},
        {"Efficiency directory",             TOFIN_HEADER_EFFICIENCY_DIRECTORY},
        {0,                                  0}
};


tofin_file *tofin_file_load(const char *filename);
int tofin_file_parse_header(tofin_file *tofin, const char *header, const char *data);
void tofin_file_free(tofin_file *tofin);
#endif
