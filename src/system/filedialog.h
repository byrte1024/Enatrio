#pragma once

#ifdef DEBUG

#include <string.h>
#include <tinyfiledialogs.h>
#include "utils.h"

static const char *FileDialog_Open(const char *title, const char *default_subdir,
                                   const char *filter_pattern,
                                   const char *filter_description) {
    char default_path[512] = {0};
    if (default_subdir) {
        AppPath_Build(default_path, sizeof(default_path), default_subdir, NULL);
        int len = (int)strlen(default_path);
        if (len > 0 && len < (int)sizeof(default_path) - 1 &&
            default_path[len - 1] != PATH_SEP_CHAR) {
            default_path[len] = PATH_SEP_CHAR;
            default_path[len + 1] = '\0';
        }
    }
    return tinyfd_openFileDialog(
        title ? title : "Open File",
        default_path,
        filter_pattern ? 1 : 0,
        filter_pattern ? &filter_pattern : NULL,
        filter_description,
        0);
}

static const char *FileDialog_Save(const char *title, const char *default_subdir,
                                   const char *default_filename,
                                   const char *filter_pattern,
                                   const char *filter_description) {
    char default_path[640] = {0};
    if (default_subdir && default_filename) {
        AppPath_Build(default_path, sizeof(default_path), default_subdir, default_filename);
    } else if (default_subdir) {
        AppPath_Build(default_path, sizeof(default_path), default_subdir, NULL);
        int len = (int)strlen(default_path);
        if (len > 0 && len < (int)sizeof(default_path) - 1 &&
            default_path[len - 1] != PATH_SEP_CHAR) {
            default_path[len] = PATH_SEP_CHAR;
            default_path[len + 1] = '\0';
        }
    }
    return tinyfd_saveFileDialog(
        title ? title : "Save File",
        default_path,
        filter_pattern ? 1 : 0,
        filter_pattern ? &filter_pattern : NULL,
        filter_description);
}

#endif
