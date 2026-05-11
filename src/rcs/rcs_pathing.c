#include "rcs/rcs_pathing.h"
#include "cleanupstack.h"
#include "common.h"
#include "context.h"
#include "linalg.h"
#include "rcs/rcs_ubo.h"
#include "res.h"
#include <Python.h>
#include <string.h>

PyObject* loadpyfunc(PyObject* module, const char* name) {
    PyObject* pfunc = PyObject_GetAttrString(module, name);

    if (pfunc && PyCallable_Check(pfunc)) {
        return pfunc;
    } else {
        return NULL;
    }
}

typedef struct PathingResourcesCleanup {
    u32* dummy;
} PathingResourcesCleanup;

void destroy_pathingresources(void* obj) { Py_Finalize(); }

void make_rcs_pathingresources(PathingResources* out_pathing,
                               CleanupStack* cs) {
    *out_pathing = (PathingResources){};
    Py_Initialize();
    PyRun_SimpleString("import sys; sys.path.append(\".\")");

    CLEANUP_START_NORES(PathingResourcesCleanup){NULL} CLEANUP_END(
        pathingresources);

    const char* pymodname = "param_path";

    PyObject* pname = PyUnicode_DecodeFSDefault(pymodname);
    PyObject* pmodule = PyImport_Import(pname);
    Py_XDECREF(pname);

    constexpr u32 N_FUNCS = 7;

    const char* loadfuncs[N_FUNCS] = {"path_init",
                                      "path_advance",
                                      "path_is_complete",
                                      "path_get_colnames",
                                      "path_get_colvals",
                                      "path_get_params",
                                      "path_datawrite_settings"};

    PyObject** outputlocs[N_FUNCS] = {&out_pathing->pypath_init,
                                      &out_pathing->pypath_advance,
                                      &out_pathing->pypath_is_complete,
                                      &out_pathing->pypath_get_colnames,
                                      &out_pathing->pypath_get_colvals,
                                      &out_pathing->pypath_get_params,
                                      &out_pathing->pypath_datawritesettings};

    if (pmodule != NULL) {
        for (u32 i = 0; i < N_FUNCS; i++) {
            PyObject* f = loadpyfunc(pmodule, loadfuncs[i]);
            if (f != NULL) {
                *outputlocs[i] = f;
            } else {
                printf("%s.py: missing or uncallable function: %s, quitting\n",
                       pymodname, loadfuncs[i]);
                abort();
            }
        }
    } else {
        printf("Failed to load %s.py, quitting\n", pymodname);
    }
    Py_XDECREF(pmodule);
}

void path_init(PathingResources* res, Path* p) {
    PyObject* path = PyObject_CallObject(res->pypath_init, NULL);

    if (path == NULL) {
        printf("path_init didn't return valid object\n");
        abort();
    }

    p->pypath = path;
}

void path_advance(PathingResources* res, Path* p) {
    PyObject* args = PyTuple_Pack(1, p->pypath);

    PyObject* pnewpathstate = PyObject_CallObject(res->pypath_advance, args);

    if (pnewpathstate == NULL) {
        PyObject* exc = PyErr_GetRaisedException();

        PyObject* err = PyObject_Str(exc);

        const char* message = PyUnicode_AsUTF8(err);

        printf("path_advance: failed to advance path: %s\n", message);

        Py_XDECREF(exc);
        Py_XDECREF(err);
        abort();
    }

    Py_XDECREF(p->pypath);
    p->pypath = pnewpathstate;

    Py_DECREF(args);
}

bool path_is_complete(PathingResources* pres, Path* p) {

    PyObject* args = PyTuple_Pack(1, p->pypath);

    PyObject* iscomplete = PyObject_CallObject(pres->pypath_is_complete, args);

    Py_DECREF(args);

    i32 truth = PyObject_IsTrue(iscomplete);

    if (truth == -1) {
        printf("path_iscomplete: couldn't determine if true or not\n");
    }

    return truth;
}

void path_write_statnames(PathingResources* pres, Path* p, FILE* fp) {
    PyObject* args = PyTuple_Pack(1, p->pypath);

    PyObject* retlist = PyObject_CallObject(pres->pypath_get_colnames, args);

    Py_DECREF(args);

    if (retlist && PyList_Check(retlist)) {
        const Py_ssize_t len = PyList_Size(retlist);

        for (u32 i = 0; i < len; i++) {
            PyObject* rawitem = PyList_GetItem(retlist, i);

            PyObject* str = PyObject_Str(rawitem);

            if (str != NULL) {
                const char* c_str = PyUnicode_AsUTF8(str);
                fprintf(fp, "%s, ", c_str);
            } else {
                printf("path_get_colnames: value couldn't be turned into "
                       "string!\n");
                fprintf(fp, "unknown_col%u", i);
            }
            Py_XDECREF(str);
        }
    } else {
        printf("path_get_colnames: didn't return a list of names\n");
    }

    Py_XDECREF(retlist);
}

void path_write_statcols(PathingResources* pres, Path* p, FILE* fp) {

    PyObject* args = PyTuple_Pack(1, p->pypath);

    PyObject* retlist = PyObject_CallObject(pres->pypath_get_colvals, args);

    Py_DECREF(args);

    if (retlist && PyList_Check(retlist)) {
        const Py_ssize_t len = PyList_Size(retlist);

        for (u32 i = 0; i < len; i++) {
            PyObject* rawitem = PyList_GetItem(retlist, i);

            PyObject* str = PyObject_Str(rawitem);

            if (str != NULL) {
                const char* c_str = PyUnicode_AsUTF8(str);
                fprintf(fp, "%s, ", c_str);
            } else {
                printf("path_get_colvals: value couldn't be turned into "
                       "string! writing 0\n");
                fprintf(fp, "0.0, ");
            }
            Py_XDECREF(str);
        }
    } else {
        printf("path_get_colvals: didn't return a list of floats\n");
    }

    Py_XDECREF(retlist);
}

void try_assign_float(f32* dst, PyObject* val) {
    double myval = PyFloat_AsDouble(val);

    if (myval == -1.0 && PyErr_Occurred()) {
        printf("value could not be made into a float! ignoring\n");
    } else {
        *dst = (f32)myval;
    }
}

void try_assign_bool(bool* dst, PyObject* val) {
    u32 res = PyObject_IsTrue(val);

    if (res == -1) {
        printf("value failed to be interpreted as bool! ignoring\n");
    } else {
        *dst = res;
    }
}

// params is array of
/*
scale xyz
translation xyz
rotation xyz
lambda
*/

void raw_write_rcsubo(void* mapping, PathParameters params) {

    f32 p_scalex = params.scale.x;
    f32 p_scaley = params.scale.y;
    f32 p_scalez = params.scale.z;
    f32 p_posx = params.pos.x;
    f32 p_posy = params.pos.y;
    f32 p_posz = params.pos.z;
    f32 p_rotx = params.rot.x;
    f32 p_roty = params.rot.y;
    f32 p_rotz = params.rot.z;
    f32 p_lambda = params.lambda;

    RcsUbo ubo = {0};
    Mat4 ident4 = {{1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0,
                    0.0, 0.0, 0.0, 1.0}};

    ubo.model = ident4;
    ubo.view = ident4;
    ubo.proj = ident4;
    ubo.norm_trans = ident4;
    ubo.resolution_xy_L_lambda =
        (Vec4){RCS_RESOLUTION, RCS_RESOLUTION, RCS_RANGE, p_lambda};

    Mat4 scale = ident4;

    *pindex_m4(&scale, 0, 0) = p_scalex;
    *pindex_m4(&scale, 1, 1) = p_scaley;
    *pindex_m4(&scale, 2, 2) = p_scalez;

    Mat4 transl = ident4;

    *pindex_m4(&transl, 0, 3) = p_posx;
    *pindex_m4(&transl, 1, 3) = p_posy;
    *pindex_m4(&transl, 2, 3) = p_posz;

    Mat4 rotx = ident4;
    Mat4 roty = ident4;
    Mat4 rotz = ident4;

    *pindex_m4(&rotx, 1, 1) = cosf(p_rotx);
    *pindex_m4(&rotx, 1, 2) = -sinf(p_rotx);
    *pindex_m4(&rotx, 2, 1) = sinf(p_rotx);
    *pindex_m4(&rotx, 2, 2) = cosf(p_rotx);

    *pindex_m4(&roty, 0, 0) = cosf(p_roty);
    *pindex_m4(&roty, 0, 2) = -sinf(p_roty);
    *pindex_m4(&roty, 2, 0) = sinf(p_roty);
    *pindex_m4(&roty, 2, 2) = cosf(p_roty);

    *pindex_m4(&rotz, 0, 0) = cosf(p_rotz);
    *pindex_m4(&rotz, 0, 1) = -sinf(p_rotz);
    *pindex_m4(&rotz, 1, 0) = sinf(p_rotz);
    *pindex_m4(&rotz, 1, 1) = cosf(p_rotz);

    ubo.model = mul_m4(transl, mul_m4(rotx, mul_m4(roty, mul_m4(rotz, scale))));

    Mat3 norm = transpose_m3(invert_m3(subm4_m3(ubo.model)));

    Mat4 norm4 = zeroed_from_m3(norm);
    *pindex_m4(&norm4, 3, 3) = 1.0f; // set corner

    ubo.norm_trans = norm4;

    const f32 near = -RCS_BOXSIZE / 2.0f, far = RCS_BOXSIZE / 2.0f,
              left = -RCS_BOXSIZE / 2.0f, right = RCS_BOXSIZE / 2.0f,
              top = -RCS_BOXSIZE / 2.0f, bot = RCS_BOXSIZE / 2.0f;

    *pindex_m4(&ubo.proj, 0, 0) = 2.0f / (right - left);
    *pindex_m4(&ubo.proj, 1, 1) = 2.0f / (bot - top);
    *pindex_m4(&ubo.proj, 2, 2) = 1.0f / (far - near);

    *pindex_m4(&ubo.proj, 0, 3) = -(right + left) / (right - left);
    *pindex_m4(&ubo.proj, 1, 3) = -(bot + top) / (bot - top);
    *pindex_m4(&ubo.proj, 2, 3) = -(near) / (far - near);

    f32 disablestatus = 0.0f;
    if (!params.ILDC_active) {
        disablestatus = -1.0;
        printf("ILDC OFF\n");
    }
    if (!params.PO_active) {
        disablestatus = 1.0;
        printf("PO OFF\n");
    }

    ubo.cropfraction_boxsize_disablestatus_linewidth = (Vec4){
        (f32)RCS_CROPFRACTION, RCS_BOXSIZE, disablestatus, RCS_LINEWIDTH};

    // THIS NEEDS TO HAVE LENGTH 1
    ubo.infield =
        (Vec4){cosf(params.pol_angle), 0.0, sinf(params.pol_angle), 0.0};

    memcpy(mapping, &ubo, sizeof(ubo));
}

void manualcontrol_write_rcsubo(RenderContext* ctx, void* mapping) {

    PathParameters pars = {
        {1.0, 1.0, 1.0},
        {0.0, 0.0, 0.0},
        {ctx->manual_control.pitch, ctx->manual_control.yaw, 0.0},
        ctx->manual_control.lambda,
        ctx->manual_control.pol_angle,
        ctx->manual_control.PO_active,
        ctx->manual_control.ILDC_active};

    raw_write_rcsubo(mapping, pars);
}

// writes into param buffer of 10 floats
void get_path_params(PathingResources* pres, Path* p,
                     PathParameters* out_params) {
    f32 p_rotx = 0.0f;
    f32 p_roty = 0.0f;
    f32 p_rotz = 0.0f;
    f32 p_posx = 0.0f;
    f32 p_posy = 0.0f;
    f32 p_posz = 0.0f;
    f32 p_scalex = 1.0f;
    f32 p_scaley = 1.0f;
    f32 p_scalez = 1.0f;
    f32 p_lambda = 15e-2f;
    f32 p_pol_angle = 0.0f;
    bool po_enable = true;
    bool ildc_enable = true;

    {
        PyObject* args = PyTuple_Pack(1, p->pypath);

        PyObject* vals_dict =
            PyObject_CallObject(pres->pypath_get_params, args);

        Py_DECREF(args);

        if (vals_dict && PyDict_Check(vals_dict)) {

            PyObject *key, *val;
            Py_ssize_t i = 0;

            while (PyDict_Next(vals_dict, &i, &key, &val)) {
                if (PyUnicode_Check(key)) {
                    // here it's guaranteed to be a string

                    if (PyUnicode_CompareWithASCIIString(key, "rot.x") == 0) {
                        try_assign_float(&p_rotx, val);
                    } else if (PyUnicode_CompareWithASCIIString(key, "rot.y") ==
                               0) {
                        try_assign_float(&p_roty, val);
                    } else if (PyUnicode_CompareWithASCIIString(key, "rot.z") ==
                               0) {
                        try_assign_float(&p_rotz, val);
                    } else if (PyUnicode_CompareWithASCIIString(
                                   key, "pos.x ") == 0) {
                        try_assign_float(&p_posx, val);
                    } else if (PyUnicode_CompareWithASCIIString(key, "pos.y") ==
                               0) {
                        try_assign_float(&p_posy, val);
                    } else if (PyUnicode_CompareWithASCIIString(key, "pos.z") ==
                               0) {
                        try_assign_float(&p_posz, val);
                    } else if (PyUnicode_CompareWithASCIIString(
                                   key, "scale.x") == 0) {
                        try_assign_float(&p_scalex, val);
                    } else if (PyUnicode_CompareWithASCIIString(
                                   key, "scale.y") == 0) {
                        try_assign_float(&p_scaley, val);
                    } else if (PyUnicode_CompareWithASCIIString(
                                   key, "scale.z") == 0) {
                        try_assign_float(&p_scalez, val);
                    } else if (PyUnicode_CompareWithASCIIString(
                                   key, "lambda") == 0) {
                        try_assign_float(&p_lambda, val);
                    } else if (PyUnicode_CompareWithASCIIString(
                                   key, "pol_angle") == 0) {
                        try_assign_float(&p_pol_angle, val);
                    } else if (PyUnicode_CompareWithASCIIString(
                                   key, "po_enable") == 0) {
                        try_assign_bool(&po_enable, val);
                    } else if (PyUnicode_CompareWithASCIIString(
                                   key, "ildc_enable") == 0) {
                        try_assign_bool(&ildc_enable, val);
                    }
                } else {
                    printf("non-string key in dict returned from "
                           "path_get_params! ignoring\n");
                }
            }
        } else {
            printf("path_get_params didn't return a dictionary! ignoring\n");
        }
        Py_XDECREF(vals_dict);
    }

    PathParameters built = {{p_scalex, p_scaley, p_scalez},
                            {p_posx, p_posy, p_posz},
                            {p_rotx, p_roty, p_rotz},
                            p_lambda,
                            p_pol_angle,
                            po_enable,
                            ildc_enable};

    *out_params = built;
}

void path_write_ubo(PathingResources* pres, Path* p, void* mapping) {

    PathParameters pars = {0};

    get_path_params(pres, p, &pars);

    raw_write_rcsubo(mapping, pars);
}

void path_discard(Path* p) { Py_XDECREF(p->pypath); }

void path_copy(Path* dst, Path* src) {
    Py_XDECREF(dst->pypath);
    Py_INCREF(src->pypath);
    dst->pypath = src->pypath;
}