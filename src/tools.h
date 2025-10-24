#ifndef TOOLS_H
    #define TOOLS_H

#pragma once
#include <gtk/gtk.h>

typedef struct AppState AppState;

typedef struct Tool {
    const char *name;
    void (*on_button_press)(AppState *app, double x, double y);
    void (*on_motion)(AppState *app, double x, double y);
    void (*on_button_release)(AppState *app, double x, double y);
} Tool;

extern Tool TOOL_BRUSH;
extern Tool TOOL_ERASER;

#endif
