#pragma once

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>


#define CAT1(a) a
#define CAT2(a,b) a##b
#define CAT3(a,b,c) a##b##c
#define CAT4(a,b,c,d) a##b##c##d
#define CAT5(a,b,c,d,e) a##b##c##d##e
#define CAT6(a,b,c,d,e,f) a##b##c##d##e##f
#define CAT7(a,b,c,d,e,f,g) a##b##c##d##e##f##g
#define CAT8(a,b,c,d,e,f,g,h) a##b##c##d##e##f##g##h
#define CAT9(a,b,c,d,e,f,g,h,i) a##b##c##d##e##f##g##h##i
#define CAT10(a,b,c,d,e,f,g,h,i,j) a##b##c##d##e##f##g##h##i##j

#define BAT1(a) CAT1(a)
#define BAT2(a,b) CAT2(a,b)
#define BAT3(a,b,c) CAT3(a,b,c)
#define BAT4(a,b,c,d) CAT4(a,b,c,d)
#define BAT5(a,b,c,d,e) CAT5(a,b,c,d,e)
#define BAT6(a,b,c,d,e,f) CAT6(a,b,c,d,e,f)
#define BAT7(a,b,c,d,e,f,g) CAT7(a,b,c,d,e,f,g)
#define BAT8(a,b,c,d,e,f,g,h) CAT8(a,b,c,d,e,f,g,h)
#define BAT9(a,b,c,d,e,f,g,h,i) CAT9(a,b,c,d,e,f,g,h,i)
#define BAT10(a,b,c,d,e,f,g,h,i,j) CAT10(a,b,c,d,e,f,g,h,i,j)

#define ASIS(x) (x)

#define AppLocal _app_local_path()

#define STR(x) #x

#define BSTR(x) STR(x)


#ifdef VERBOSE

#define LOG(lvl ,msg, ...) TraceLog(lvl, __FILE__ ":" BSTR(__LINE__) " " msg, ##__VA_ARGS__)

#else

#define LOG(lvl ,msg, ...) TraceLog(lvl, msg, ##__VA_ARGS__)

#endif

#define LOG_BUILD_INFO() do { \
    LOG_INFO("========================================"); \
    LOG_INFO("  " BSTR(PROJECT_NAME)); \
    LOG_INFO("  Build Hash: " BSTR(HASH)); \
    LOG_INFO("  Build Date: " BSTR(BUILD_DAY) "/" BSTR(BUILD_MONTH) "/" BSTR(BUILD_YEAR)); \
    LOG_INFO("  Build Time: " BSTR(BUILD_HOUR) ":" BSTR(BUILD_MINUTE) ":" BSTR(BUILD_SECOND)); \
    LOG_INFO("========================================"); \
} while (0)

static FILE *_log_file = NULL;
static int _log_min_level = LOG_ALL;

static void _log_sanitize(char *buf, int len) {
    for (int i = 0; i < len; i++) {
        if (buf[i] == '\n' || buf[i] == '\r') buf[i] = ' ';
        if (buf[i] == '\033') buf[i] = '?';
    }
}

static void _log_file_callback(int logLevel, const char *text, va_list args) {
    if (logLevel < _log_min_level) return;
    if (!text) return;

    const char *level_str = "";
    switch (logLevel) {
        case LOG_TRACE:   level_str = "TRACE";   break;
        case LOG_DEBUG:   level_str = "DEBUG";   break;
        case LOG_INFO:    level_str = "INFO";    break;
        case LOG_WARNING: level_str = "WARNING"; break;
        case LOG_ERROR:   level_str = "ERROR";   break;
        case LOG_FATAL:   level_str = "FATAL";   break;
    }

    char buf[1024];
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(buf, sizeof(buf), text, args_copy);
    va_end(args_copy);

    if (len < 0) return;
    if (len >= (int)sizeof(buf)) len = (int)sizeof(buf) - 1;
    _log_sanitize(buf, len);

    printf("[%s] %s\n", level_str, buf);

    if (_log_file) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        if (t) fprintf(_log_file, "[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
        fprintf(_log_file, "[%s] %s\n", level_str, buf);
        fflush(_log_file);
    }
}

#define START_LOGGING(filepath, level) do { \
    _log_file = fopen(filepath, "a"); \
    _log_min_level = level; \
    if (_log_file) { \
        fprintf(_log_file, "\n--- Session Start ---\n\n"); \
        fflush(_log_file); \
        SetTraceLogCallback(_log_file_callback); \
    } \
} while (0)

#define END_LOGGING() do { \
    SetTraceLogCallback(NULL); \
    if (_log_file) { fclose(_log_file); _log_file = NULL; } \
} while (0)

#define LOG_TRACE(msg, ...) LOG(LOG_INFO, msg, ##__VA_ARGS__)
#define LOG_DEBUG(msg, ...) LOG(LOG_DEBUG, msg, ##__VA_ARGS__)
#define LOG_INFO(msg, ...) LOG(LOG_INFO, msg, ##__VA_ARGS__)
#define LOG_WARNING(msg, ...) LOG(LOG_WARNING, msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) LOG(LOG_ERROR, msg, ##__VA_ARGS__)
#define LOG_FATAL(msg, ...) LOG(LOG_FATAL, msg, ##__VA_ARGS__)

static const char *_app_local_path(void) {
    static char path[512] = {0};
    if (path[0]) return path;
#ifdef _WIN32
    const char *userprofile = getenv("USERPROFILE");
    if (userprofile) snprintf(path, sizeof(path), "%s\\AppData\\LocalLow\\" BSTR(PROJECT_NAME), userprofile);
#else
    const char *xdg = getenv("XDG_DATA_HOME");
    if (xdg) snprintf(path, sizeof(path), "%s/" BSTR(PROJECT_NAME), xdg);
    else {
        const char *home = getenv("HOME");
        if (home) snprintf(path, sizeof(path), "%s/.local/share/" BSTR(PROJECT_NAME), home);
    }
#endif
    return path;
}