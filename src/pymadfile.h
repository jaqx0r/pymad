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
 * OH GOD THE VIRUS IS EATING MY BRAIN
 */

#ifndef __PY_MADFILE_H__
#define __PY_MADFILE_H__

#include <Python.h>
#include <mad.h>

typedef struct {
	PyObject_HEAD
	FILE * f;
	struct mad_stream stream;
	struct mad_frame  frame;
	struct mad_synth  synth;
	mad_timer_t timer;
	unsigned char * buffy;
	unsigned int bufsiz;
	unsigned int framecount;
} py_madfile; /* MadFile */

#define PY_MADFILE(x) ((py_madfile *) x)
#define MAD_STREAM(x) (PY_MADFILE(x)->stream)
#define MAD_FRAME(x)  (PY_MADFILE(x)->frame)
#define MAD_SYNTH(x)  (PY_MADFILE(x)->synth)
#define MAD_BUFFY(x)  (PY_MADFILE(x)->buffy)
#define MAD_BUFSIZ(x) (PY_MADFILE(x)->bufsiz)
#define MAD_TIMER(x)  (PY_MADFILE(x)->timer)

extern PyTypeObject py_madfile_t;

static void py_madfile_dealloc(PyObject * self, PyObject * args);
static PyObject * py_madfile_read(PyObject * self, PyObject * args);
static PyObject * py_madfile_getattr(PyObject * self, char * name);
static PyObject * py_madfile_info(PyObject * self, PyObject * args);

#endif /* __PY_MADFILE_H__ */
