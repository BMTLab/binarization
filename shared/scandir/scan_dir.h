#ifndef SCAN_DIR_H
#define SCAN_DIR_H

#define DIR_PATH_EXAMPLES_QR_FILES "..\\images\\"
#define MAX_FILES 256
#define MAX_NAME_LENGTH 128

#ifdef __cplusplus
extern "C" {
#endif

extern char current_file[MAX_FILES][MAX_NAME_LENGTH];

extern inline const char* concat(const char*, const char*);
extern int scan_dir(char*);

#ifdef __cplusplus
}
#endif

#endif //SCAN_DIR_H
