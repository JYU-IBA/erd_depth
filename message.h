#ifndef TOFELIST_MESSAGE_H
#define TOFELIST_MESSAGE_H

typedef enum tofe_list_msg_level {
    TOFE_LIST_NORMAL = 0,
    TOFE_LIST_INFO = 1,
    TOFE_LIST_WARNING = 2,
    TOFE_LIST_ERROR = 3
} tofe_list_msg_level;

const char *msg_level_str(tofe_list_msg_level level);
void tofe_list_msg(tofe_list_msg_level level, const char *restrict format, ...);
#endif // TOFELIST_MESSAGE_H
