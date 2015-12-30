/* $Id: madmodule.c,v 1.4 2002/08/18 06:21:48 jaq Exp $
 *
 * python interface to libmad (the mpeg audio decoder library)
 *
 * Copyright (c) 2002 Jamie Wilkinson
 *
 * This program is free software, you may copy and/or modify as per
 * the GNU General Public License version 2.
 */

#include <mad.h>
#include "madmodule.h"

#if PY_MAJOR_VERSION >= 3
#define MOD_DEF(ob, name, doc, methods)                                        \
  static struct PyModuleDef moduledef = {                                      \
      PyModuleDef_HEAD_INIT, name, doc, -1, methods,                           \
  };                                                                           \
  ob = PyModule_Create(&moduledef);
#else
#define MOD_DEF(ob, name, doc, methods) ob = Py_InitModule3(name, methods, doc);
#endif

static PyMethodDef mad_methods[] = {
    {"MadFile", py_madfile_new, METH_VARARGS, ""}, {NULL, 0, 0, NULL}};

/* this handy tool for passing C constants to Python-land from
 * http://starship.python.net/crew/arcege/extwriting/pyext.html
 */
#if PY_MAJOR_VERSION >= 3
#define PY_CONST(x) PyDict_SetItemString(dict, #x, PyLong_FromLong(MAD_##x))
#else
#define PY_CONST(x) PyDict_SetItemString(dict, #x, PyInt_FromLong(MAD_##x))
#endif

extern PyTypeObject py_madfile_t;

static PyObject *moduleinit(void) {
  PyObject *module, *dict;

  if (PyType_Ready(&py_madfile_t) < 0)
    return NULL;

  MOD_DEF(module, "mad", "", mad_methods);
  dict = PyModule_GetDict(module);

  PyDict_SetItemString(dict, "__version__", PyUnicode_FromString(VERSION));

  /* layer */
  PY_CONST(LAYER_I);
  PY_CONST(LAYER_II);
  PY_CONST(LAYER_III);

  /* mode */
  PY_CONST(MODE_SINGLE_CHANNEL);
  PY_CONST(MODE_DUAL_CHANNEL);
  PY_CONST(MODE_JOINT_STEREO);
  PY_CONST(MODE_STEREO);

  /* emphasis */
  PY_CONST(EMPHASIS_NONE);
  PY_CONST(EMPHASIS_50_15_US);
  PY_CONST(EMPHASIS_CCITT_J_17);

  if (PyErr_Occurred())
    PyErr_SetString(PyExc_ImportError, "mad: init failed");

  return module;
}

#if PY_MAJOR_VERSION < 3
PyMODINIT_FUNC initmad(void) { moduleinit(); }
#else
PyMODINIT_FUNC PyInit_mad(void) { return moduleinit(); }
#endif
