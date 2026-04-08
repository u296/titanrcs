#include "rcs/rcs_pathing.h"
#include "cleanupstack.h"
#include "common.h"
#include "rcs/rcs_ubo.h"
#include <Python.h>

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

    constexpr u32 N_FUNCS = 6;

    const char* loadfuncs[N_FUNCS] = {
        "path_init",         "path_advance",     "path_is_complete",
        "path_get_colnames", "path_get_colvals", "path_get_params"
    };

    PyObject** outputlocs[N_FUNCS] = {
        &out_pathing->pypath_init, &out_pathing->pypath_advance,
        &out_pathing->pypath_is_complete, &out_pathing->pypath_get_colnames,
        &out_pathing->pypath_get_colvals, &out_pathing->pypath_get_params};

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

    Py_XDECREF(p->pypath);
    p->pypath = pnewpathstate;

    Py_DECREF(args);
}



bool path_is_complete(PathingResources* pres, Path* p) { 

    PyObject* args = PyTuple_Pack(1, p->pypath);

    PyObject* iscomplete = PyObject_CallObject(pres->pypath_is_complete, args);

    Py_DECREF(args);

    i32 truth = PyObject_IsTrue(iscomplete);

    if(truth == -1) {
        printf("path_iscomplete: couldn't determine if true or not\n");
    }

    return truth;
}

void path_write_statcols(PathingResources* pres, Path* p, FILE* fp) {

    PyObject* args = PyTuple_Pack(1, p->pypath);

    PyObject* retlist = PyObject_CallObject(pres->pypath_get_colvals, args);

    Py_DECREF(args);

    if(retlist && PyList_Check(retlist)) {
        const Py_ssize_t len = PyList_Size(retlist);

        for (u32 i = 0; i < len; i++) {
            PyObject* rawitem = PyList_GetItem(retlist, i);

            PyObject* str = PyObject_Str(rawitem);

            if (str != NULL) {
                const char* c_str = PyUnicode_AsUTF8(str);
                fprintf(fp, "%s, ", c_str);
            } else {
                printf("path_get_colvals: value couldn't be turned into string! writing 0\n");
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
    if (PyFloat_Check(val)) {
        *dst = PyFloat_AS_DOUBLE(val);
    } else {
        printf("value was not a float! ignoring\n");
    }
}

void path_write_ubo(PathingResources* pres, Path* p, void* mapping) {
    RcsUbo ubo = {};

    f32 p_rotx = 0.0f;
    f32 p_roty = 0.0f;
    f32 p_rotz = 0.0f;
    f32 p_posx = 0.0f;
    f32 p_posy = 0.0f;
    f32 p_posz = 0.0f;
    f32 p_scalex = 1.0f;
    f32 p_scaley = 1.0f;
    f32 p_scalez = 1.0f;

    {
        PyObject* args = PyTuple_Pack(1, p->pypath);

        PyObject* vals_dict = PyObject_CallObject(pres->pypath_get_params, args);

        Py_DECREF(args);

        if (vals_dict && PyDict_Check(vals_dict)) {

            PyObject*key, *val;
            Py_ssize_t i = 0;

            while (PyDict_Next(vals_dict, &i,&key, &val)) {
                if (PyUnicode_Check(key)) {
                    // here it's guaranteed to be a string

                    if (PyUnicode_CompareWithASCIIString(key, "rot.x") == 0) {
                        try_assign_float(&p_rotx, val);
                    } else if (PyUnicode_CompareWithASCIIString(key, "rot.y") == 0) {
                        try_assign_float(&p_roty, val);
                    } else if (PyUnicode_CompareWithASCIIString(key, "rot.z") == 0) {
                        try_assign_float(&p_rotz, val);
                    } else if (PyUnicode_CompareWithASCIIString(key, "pos.x ") == 0) {
                        try_assign_float(&p_posx, val);
                    } else if (PyUnicode_CompareWithASCIIString(key, "pos.y") == 0) {
                        try_assign_float(&p_posy, val);
                    } else if (PyUnicode_CompareWithASCIIString(key, "pos.z") == 0) {
                        try_assign_float(&p_posz, val);
                    } else if (PyUnicode_CompareWithASCIIString(key, "scale.x") == 0) {
                        try_assign_float(&p_scalex, val);
                    } else if (PyUnicode_CompareWithASCIIString(key, "scale.y") == 0) {
                        try_assign_float(&p_scaley, val);
                    } else if (PyUnicode_CompareWithASCIIString(key, "scale.z") == 0) {
                        try_assign_float(&p_scalez, val);
                    }
                } else {
                    printf("non-string key in dict returned from path_get_params! ignoring\n");
                }
            }
        } else {
            printf("path_get_params didn't return a dictionary! ignoring\n");
        }
        Py_XDECREF(vals_dict);
    }


    Mat4 ident4 = {{1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0,
                    0.0, 0.0, 0.0, 1.0}};

    const f32 L = 1000.0f;

    ubo.model = ident4;
    ubo.view = ident4;
    ubo.proj = ident4;
    ubo.norm_trans = ident4;
    ubo.resolution_xy_L_ = (Vec4){RCS_RESOLUTION, RCS_RESOLUTION, L, 0.0};

    Mat4 scale = ident4;

    const f32 s = 1.0f;

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



    ubo.model = mul_m4(transl, mul_m4(rotx, mul_m4(roty, mul_m4(rotz,scale))));

    Mat3 norm = transpose_m3(invert_m3(subm4_m3(ubo.model)));

    Mat4 norm4 = zeroed_from_m3(norm);
    *pindex_m4(&norm4, 3, 3) = 1.0f; // set corner

    ubo.norm_trans = norm4;

    const f32 near = -10.0f, far = 10.0f, left = -10.0f, right = 10.0f,
              top = -10.0f, bot = 10.0f;

    *pindex_m4(&ubo.proj, 0, 0) = 2.0f / (right - left);
    *pindex_m4(&ubo.proj, 1, 1) = 2.0f / (bot - top);
    *pindex_m4(&ubo.proj, 2, 2) = 1.0f / (far - near);

    *pindex_m4(&ubo.proj, 0, 3) = -(right + left) / (right - left);
    *pindex_m4(&ubo.proj, 1, 3) = -(bot + top) / (bot - top);
    *pindex_m4(&ubo.proj, 2, 3) = -(near) / (far - near);

    memcpy(mapping, &ubo, sizeof(ubo));
}

void path_discard(Path* p) {
    Py_XDECREF(p->pypath);
}

void path_copy(Path* dst, Path* src) {
    Py_XDECREF(dst->pypath);
    Py_INCREF(src->pypath);
    dst->pypath = src->pypath;
}