#ifndef PATHING_H
#define PATHING_H

#include "cleanupstack.h"
#include <stdio.h>
#include <Python.h>
//typedef struct PyObject PyObject;

typedef struct PathingResources {
    PyObject* module;
    PyObject* pypath_init;
    PyObject* pypath_advance;
    PyObject* pypath_is_complete;
    PyObject* pypath_get_params;
    PyObject* pypath_get_colnames;
    PyObject* pypath_get_colvals;

} PathingResources;

typedef struct Path {
    PyObject* pypath;
} Path;

void make_rcs_pathingresources(PathingResources* out_pathing, CleanupStack* cs);

void path_init(PathingResources* res, Path* p);

void path_advance(PathingResources* res, Path* p);

bool path_is_complete(PathingResources* pres, Path* p);

void path_write_statnames(PathingResources* pres, Path* p, FILE* fp);

void path_write_statcols(PathingResources* pres, Path* p, FILE* fp);

void path_write_ubo(PathingResources* pres, Path* p, void* mapping);

typedef struct RenderContext RenderContext;

void manualcontrol_write_rcsubo(RenderContext* ctx, void* mapping);

void get_path_params(PathingResources* pres, Path* p, f32* out_params);

void raw_write_rcsubo(void* mapping, f32* pars);

void path_discard(Path* p);

void path_copy(Path* dst, Path* src);

#endif