/* $Id$
 *
 * python interface to libmad (the mpeg audio decoder library)
 *
 * Copyright (c) 2002 Jamie Wilkinson
 *
 * This program is free software, you may copy and/or modify as per
 * the GNU General Public License (version 2, or at your discretion,
 * any later version).  This is the same license as libmad.
 *
 * The structure of this module was copied from the Ogg Vorbis python
 * module.
 *
 * TODO:
 * - add info functinos, for stuff like bitrate and so on
 */

#include <mad.h>
#include "madmodule.h"

static PyMethodDef mad_methods[] = {
	{ "MadFile", py_madfile_new, METH_VARARGS, "" },
	{ NULL, 0, 0, NULL }
};

void initmad(void) {
	PyObject * module, * dict;

	module = Py_InitModule("mad", mad_methods);
	dict = PyModule_GetDict(module);

	PyDict_SetItemString(dict, "__version__",
		PyString_FromString(mad_version));

	if (PyErr_Occurred())
		PyErr_SetString(PyExc_ImportError, "mad: init failed");
}
