/* $Id: pymadfile.h,v 1.5 2003/01/11 13:15:39 jaq Exp $
 *
 * python interface to libmad (the mpeg audio decoder library)
 *
 * Copyright (c) 2002 Jamie Wilkinson
 *
 * This program is free software, you may copy and/or modify as per
 * the GNU General Public License version 2.
 */

#ifndef __PY_MADFILE_H__
#define __PY_MADFILE_H__

#include <Python.h>
#include <mad.h>

/* The definition of the MadFile python object. */
typedef struct {
  PyObject_HEAD PyObject *fobject;
  int close_file;
  struct mad_stream stream;
  struct mad_frame frame;
  struct mad_synth synth;
  mad_timer_t timer;
  unsigned char *input_buffer;
  unsigned int bufsize;
  unsigned int framecount;
  unsigned long total_length;
} py_madfile; /* MadFile */

/* Macros for accessing elements of the MadFile object, used internally. */
#define PY_MADFILE(x) ((py_madfile *)x)
#define PYMAD_STREAM(x) (PY_MADFILE(x)->stream)
#define PYMAD_FRAME(x) (PY_MADFILE(x)->frame)
#define PYMAD_SYNTH(x) (PY_MADFILE(x)->synth)
#define PYMAD_BUFFER(x) (PY_MADFILE(x)->input_buffer)
#define PYMAD_BUFSIZE(x) (PY_MADFILE(x)->bufsize)
#define PYMAD_TIMER(x) (PY_MADFILE(x)->timer)

/* Exported methods. */
static void py_madfile_dealloc(PyObject *self, PyObject *args);
static PyObject *py_madfile_read(PyObject *self, PyObject *args);
static PyObject *py_madfile_layer(PyObject *self, PyObject *args);
static PyObject *py_madfile_mode(PyObject *self, PyObject *args);
static PyObject *py_madfile_samplerate(PyObject *self, PyObject *args);
static PyObject *py_madfile_bitrate(PyObject *self, PyObject *args);
static PyObject *py_madfile_emphasis(PyObject *self, PyObject *args);
static PyObject *py_madfile_total_time(PyObject *self, PyObject *args);
static PyObject *py_madfile_current_time(PyObject *self, PyObject *args);
static PyObject *py_madfile_seek_time(PyObject *self, PyObject *args);

#endif /* __PY_MADFILE_H__ */
