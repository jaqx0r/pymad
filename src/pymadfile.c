/* $Id: pymadfile.c,v 1.20 2003/02/05 06:29:23 jaq Exp $
 *
 * python interface to libmad (the mpeg audio decoder library)
 *
 * Copyright (c) 2002 Jamie Wilkinson
 *
 * This program is free software, you may copy and/or modify as per
 * the  GNU General Public License version 2.
 *
 * The code in py_madfile_read was copied from the madlld program, which
 * can be found at http://www.bsd-dk.dk/~elrond/audio/madlld/ and carries
 * the following copyright and license:
 *
 * The madlld program is © 2001 by Bertrand Petit, all rights reserved.
 * It is distributed under the terms of the license similar to the
 * Berkeley license reproduced below.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *
 *  3. Neither the name of the author nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <Python.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "madmodule.h"
#include "pymadfile.h"
#include "xing.h"

#if PY_VERSION_HEX < 0x01060000
#define PyObject_DEL(op) PyMem_DEL((op))
#endif

#ifndef PyVarObject_HEAD_INIT
#define PyVarObject_HEAD_INIT(type, size) PyObject_HEAD_INIT(type) size,
#endif

#if PY_MAJOR_VERSION >= 3
#define MADFILE_GETATTR 0
#define PyInt_FromLong PyLong_FromLong
#define PyInt_AsLong PyLong_AsLong
#else
#define MADFILE_GETATTR (getattrfunc) py_madfile_getattr
#define PyBytes_AsStringAndSize PyString_AsStringAndSize
#endif

#define ERROR_MSG_SIZE 512
#define MAD_BUF_SIZE                                                           \
  (5 * 8192) /* should be a multiple of 4 >= 4096 and large enough to capture  \
                possible                                                       \
                junk at beginning of MP3 file*/

/* local helpers */
static unsigned long calc_total_time(PyObject *);
static int16_t madfixed_to_int16(mad_fixed_t);

static PyMethodDef madfile_methods[] = {
    {"read", py_madfile_read, METH_VARARGS, ""},
    {"layer", py_madfile_layer, METH_VARARGS, ""},
    {"mode", py_madfile_mode, METH_VARARGS, ""},
    {"samplerate", py_madfile_samplerate, METH_VARARGS, ""},
    {"bitrate", py_madfile_bitrate, METH_VARARGS, ""},
    {"emphasis", py_madfile_emphasis, METH_VARARGS, ""},
    {"total_time", py_madfile_total_time, METH_VARARGS, ""},
    {"current_time", py_madfile_current_time, METH_VARARGS, ""},
    {"seek_time", py_madfile_seek_time, METH_VARARGS, ""},
    {NULL, 0, 0, NULL}};

PyTypeObject py_madfile_t = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0) "MadFile", /* tp_name */
    sizeof(py_madfile),                               /* tp_basicsize */
    0,                                                /* tp_itemsize */
    (destructor)py_madfile_dealloc,                   /* tp_dealloc */
    0,                                                /* (tp_print) */
    0,                                                /* (tp_getattr) */
    0,                                                /* (tp_setattr) */
    0,               /* (tp_reserved) [was tp_compare] */
    0,               /* tp_repr */
    0,               /* tp_as_number */
    0,               /* tp_as_sequence */
    0,               /* tp_as_mapping */
    0,               /* tp_hash */
    0,               /* tp_call */
    0,               /* tp_str */
    0,               /* tp_getattro */
    0,               /* tp_setattro */
    0,               /* tp_as_buffer */
    0,               /* tp_flags */
    0,               /* tp_doc */
    0,               /* tp_traverse */
    0,               /* tp_clear */
    0,               /* tp_richcompare */
    0,               /* tp_weaklistoffset */
    0,               /* tp_iter */
    0,               /* tp_iternext */
    madfile_methods, /* tp_methods */
    0,               /* tp_members */
    0,               /* tp_getset */
    0,               /* tp_base */
    0,               /* tp_dict */
    0,               /* tp_descr_get */
    0,               /* tp_descr_set */
    0,               /* tp_dictoffset */
    0,               /* tp_init */
    0,               /* tp_alloc */
    0,               /* tp_new */
    0,               /* tp_free */
    0,               /* tp_is_gc */
    0,               /* tp_bases */
    0,               /* tp_mro */
    0,               /* tp_cache */
    0,               /* tp_subclasses */
    0,               /* tp_weaklist */
    0,               /* tp_del */
    0,               /* tp_version_tag */
};

/* functions */

PyObject *py_madfile_new(PyObject *self, PyObject *args) {
  py_madfile *mf = NULL;
  int close_file = 0;
#if PY_MAJOR_VERSION >= 3
  int fd;
#endif
  char *fname;
  PyObject *fobject = NULL;
  char *initial;
  long ibytes = 0;
  unsigned long int bufsize = MAD_BUF_SIZE;
  int n;

  if (PyArg_ParseTuple(args, "s|l:MadFile", &fname, &bufsize)) {
#if PY_MAJOR_VERSION < 3
    fobject = PyFile_FromString(fname, "r");
#else
    if ((fd = open(fname, O_RDONLY)) < 0) {
      return NULL;
    }
    fobject = PyFile_FromFd(fd, fname, "rb", -1, NULL, NULL, NULL, 1);
#endif
    close_file = 1;
    if (fobject == NULL) {
      return NULL;
    }
  } else if (PyArg_ParseTuple(args, "O|sl:MadFile", &fobject, &initial,
                              &ibytes)) {
    /* clear the first failure */
    PyErr_Clear();
    /* make sure that if nothing else we can read it */
    if (!PyObject_HasAttrString(fobject, "read")) {
      Py_DECREF(fobject);
      PyErr_SetString(PyExc_IOError, "Object must have a read method");
      return NULL;
    }
  } else
    return NULL;

  /* bufsize must be an integer multiple of 4 */
  if ((n = bufsize % 4))
    bufsize -= n;
  if (bufsize <= 4096)
    bufsize = 4096;

  mf = PyObject_NEW(py_madfile, &py_madfile_t);
  Py_INCREF(fobject);
  mf->fobject = fobject;
  mf->close_file = close_file;

  /* initialise the mad structs */
  mad_stream_init(&PYMAD_STREAM(mf));
  mad_frame_init(&PYMAD_FRAME(mf));
  mad_synth_init(&PYMAD_SYNTH(mf));
  mad_timer_reset(&PYMAD_TIMER(mf));

  mf->framecount = 0;

  mf->input_buffer = malloc(bufsize * sizeof(unsigned char));
  mf->bufsize = bufsize;

  /* explicitly call read to fill the buffer and have the frame header
   * data immediately available to the caller */
  py_madfile_read((PyObject *)mf, NULL);

  mf->total_length = calc_total_time((PyObject *)mf);

  return (PyObject *)mf;
}

static void py_madfile_dealloc(PyObject *self, PyObject *args) {
  PyObject *o;

  if (PY_MADFILE(self)->fobject) {
    mad_synth_finish(&PYMAD_SYNTH(self));
    mad_frame_finish(&PYMAD_FRAME(self));
    mad_stream_finish(&PYMAD_STREAM(self));

    free(PYMAD_BUFFER(self));
    PYMAD_BUFFER(self) = NULL;
    PYMAD_BUFSIZE(self) = 0;

    if (PY_MADFILE(self)->close_file) {
      o = PyObject_CallMethod(PY_MADFILE(self)->fobject, "close", NULL);
      if (o != NULL) {
        Py_DECREF(o);
      }
    }

    Py_DECREF(PY_MADFILE(self)->fobject);
    PY_MADFILE(self)->fobject = NULL;
  }
  PyObject_DEL(self);
}

/* Calculates length of MP3 by stepping through the frames and summing up
 * the duration of each frame. */
static unsigned long calc_total_time(PyObject *self) {
  mad_timer_t timer;
  struct xing xing;
  struct stat buf;
  unsigned long r;
  PyObject *o;
  int fnum;

  xing_init(&xing);
  xing_parse(&xing, PYMAD_STREAM(self).anc_ptr, PYMAD_STREAM(self).anc_bitlen);

  if (xing.flags & XING_FRAMES) {
    timer = PYMAD_FRAME(self).header.duration;
    mad_timer_multiply(&timer, xing.frames);
    r = mad_timer_count(timer, MAD_UNITS_MILLISECONDS);
  } else {
    o = PyObject_CallMethod(PY_MADFILE(self)->fobject, "fileno", NULL);
    if (o == NULL) {
      /* no fileno method is provided, probably not a file */
      PyErr_Clear();
      return -1;
    }
    fnum = PyInt_AsLong(o);
    Py_DECREF(o);
    r = fstat(fnum, &buf);

    /* Figure out actual length of file by stepping through it.
     * This is a stripped down version of how madplay does it. */
    void *ptr = mmap(0, buf.st_size, PROT_READ, MAP_SHARED, fnum, 0);
    if (!ptr) {
      fprintf(stderr, "mmap failed, can't calculate length");
      return -1;
    }

    mad_timer_t time = mad_timer_zero;
    struct mad_stream stream;
    struct mad_header header;

    mad_stream_init(&stream);
    mad_header_init(&header);

    mad_stream_buffer(&stream, ptr, buf.st_size);

    while (1) {
      if (mad_header_decode(&header, &stream) == -1) {
        if (MAD_RECOVERABLE(stream.error))
          continue;
        else
          break;
      }

      mad_timer_add(&time, header.duration);
    }

    if (munmap(ptr, buf.st_size) == -1) {
      return -1;
    }

    r = time.seconds * 1000;
  }
  return r;
}

/* convert the MAD fixed point format to a signed 16 bit int */
static int16_t madfixed_to_int16(mad_fixed_t sample) {
  /* A fixed point number is formed of the following bit pattern:
   *
   * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
   * MSB                          LSB
   * S = sign
   * W = whole part bits
   * F = fractional part bits
   *
   * This pattern contains MAD_F_FRACBITS fractional bits, one should
   * always use this macro when working on the bits of a fixed point
   * number.  It is not guaranteed to be constant over the different
   * platforms supported by libmad.
   *
   * The int16_t value is formed by the least significant
   * whole part bit, followed by the 15 most significant fractional
   * part bits.
   *
   * This algorithm was taken from input/mad/mad_engine.c in alsaplayer,
   * which scales and rounds samples to 16 bits, unlike the version in
   * madlld.
   */

  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

static PyObject *py_madfile_read(PyObject *self, PyObject *args) {
  PyObject *pybuf;               /* return object containing output buffer*/
  int16_t *output_buffer = NULL; /* output buffer */
  int16_t *output = NULL;
  unsigned int i;
  Py_ssize_t size;
  int nextframe = 0;
  int result;
  char errmsg[ERROR_MSG_SIZE];

  /* if we are at EOF, then return None */
  // FIXME: move to if null read
  /*
  if (feof(PY_MADFILE(self)->f)) {
      Py_INCREF(Py_None);
      return Py_None;
  }
  */

  /* nextframe might get set during this loop, in which case the
   * bucket needs to be refilled */
  do {
    nextframe = 0; /* reset */

    /* The input bucket must be filled if it becomes empty or if
     * it's the first execution on this file */
    if ((PYMAD_STREAM(self).buffer == NULL) ||
        (PYMAD_STREAM(self).error == MAD_ERROR_BUFLEN)) {
      Py_ssize_t readsize, remaining;
      unsigned char *readstart;
      PyObject *o_read;
      char *o_buffer;

      /* [1] libmad may not consume all bytes of the input buffer.
       *     If the last frame in the buffer is not wholly contained
       *     by it, then that frame's start is pointed to by the
       *     next_frame member of the mad_stream structure.  This
       *     common situation occurs when mad_frame_decode():
       *      1) fails,
       *      2) sets the stream error to MAD_ERROR_BUFLEN, and
       *      3) sets the next_frame pointer to a non NULL value.
       *     (See also the comment marked [2] below)
       *
       *     When this occurs, the remaining unused bytes must be put
       *     back at the beginning of the buffer and taken into
       *     account before refilling the buffer.  This means that
       *     the input buffer must be large enough to hold a whole
       *     frame at the highest observable bitrate (currently
       *     448kbps).
       */
      if (PYMAD_STREAM(self).next_frame != NULL) {
        remaining = PYMAD_STREAM(self).bufend - PYMAD_STREAM(self).next_frame;
        memmove(PYMAD_BUFFER(self), PYMAD_STREAM(self).next_frame, remaining);
        readstart = PYMAD_BUFFER(self) + remaining;
        readsize = PYMAD_BUFSIZE(self) - remaining;
      } else
        readstart = PYMAD_BUFFER(self), readsize = PYMAD_BUFSIZE(self),
        remaining = 0;

      /* Fill in the buffer.  If an error occurs, make like a tree */
      o_read =
          PyObject_CallMethod(PY_MADFILE(self)->fobject, "read", "i", readsize);
      if (o_read == NULL) {
        // FIXME: should specifically handle read errors...
        Py_INCREF(Py_None);
        return Py_None;
      }
      PyBytes_AsStringAndSize(o_read, &o_buffer, &readsize);
      // EOF?
      if (readsize == 0) {
        Py_DECREF(o_read);
        Py_INCREF(Py_None);
        return Py_None;
      }
      memcpy(readstart, o_buffer, readsize);
      Py_DECREF(o_read);

      /* Pipe the new buffer content to libmad's stream decode
       * facility */
      mad_stream_buffer(&PYMAD_STREAM(self), PYMAD_BUFFER(self),
                        readsize + remaining);
      PYMAD_STREAM(self).error = 0;
    }

    /* Decode the next mpeg frame.  The streams are read from the
     * buffer, its constituents are broken down and stored in the
     * frame structure, ready for examination/alteration or PCM
     * synthesis.  Decoding options are carried to the Frame from
     * the Stream.
     *
     * Error handling: mad_frame_decode() returns a non-zero value
     * when an error occurs.  The error condition can be checked in
     * the error member of the Stream structure.  A mad error is
     * recoverable or fatal, the error status is checked with the
     * MAD_RECOVERABLE macro.
     *
     * [2] When a fatal error is encountered, all decoding activities
     *     shall be stopped, except when a MAD_ERROR_BUFLEN is
     *     signalled.  This condition means that the
     *     mad_frame_decode() function needs more input do its work.
     *     One should refill the buffer and repeat the
     *     mad_frame_decode() call.  Some bytes may be left unused
     *     at the end of the buffer if those bytes form an incomplete
     *     frame.  Before refilling, the remaining bytes must be
     *     moved to the beginning of the buffer and used for input
     *     for the next mad_frame_decode() invocation.  (See the
     *     comment marked [1] for earlier details)
     *
     * Recoverable errors are caused by malformed bitstreams, in
     * this case one can call again mad_frame_decode() in order to
     * skip the faulty part and resync to the next frame.
     */
    Py_BEGIN_ALLOW_THREADS;
    result = mad_frame_decode(&PYMAD_FRAME(self), &PYMAD_STREAM(self));
    Py_END_ALLOW_THREADS;
    if (result) {
      if (MAD_RECOVERABLE(PYMAD_STREAM(self).error)) {
        /* FIXME: prefer to return an error string to the caller
         * rather than print to stderr
        fprintf(stderr, "mad: recoverable frame level error: %s\n",
                mad_stream_errorstr(&PYMAD_STREAM(self)));
        fflush(stderr);
        */
        /* go onto the next frame */
        nextframe = 1;
      } else {
        if (PYMAD_STREAM(self).error == MAD_ERROR_BUFLEN) {
          /* not enough data to decode */
          nextframe = 1;
        } else {
          snprintf(errmsg, ERROR_MSG_SIZE,
                   "unrecoverable frame level error: %s",
                   mad_stream_errorstr(&PYMAD_STREAM(self)));
          PyErr_SetString(PyExc_RuntimeError, errmsg);
          return NULL;
        }
      }
    }

  } while (nextframe);

  Py_BEGIN_ALLOW_THREADS;

  /* Accounting.  The computed frame duration is in the frame header
   * structure.  It is expressed as a fixed point number whose data
   * type is mad_timer_t.  It is different from the fixed point
   * format of the samples and unlike it, cannot be directly added
   * or subtracted.  The timer module provides several functions to
   * operate on such numbers.  Be careful though, as some functions
   * of mad's timer module receive their mad_timer_t arguments by
   * value! */
  PY_MADFILE(self)->framecount++;
  mad_timer_add(&PYMAD_TIMER(self), PYMAD_FRAME(self).header.duration);

  /* Once decoded, the frame can be synthesised to PCM samples.
   * No errors are reported by mad_synth_frame() */
  mad_synth_frame(&PYMAD_SYNTH(self), &PYMAD_FRAME(self));

  Py_END_ALLOW_THREADS;

  /* Create the buffer to store the PCM samples in, so python can
   * use it.  We do 2 pointer increments per sample in the buffer,
   * so make it 2 times as big as the number of samples */
  size = PYMAD_SYNTH(self).pcm.length * 2 * sizeof(int16_t);

  output = output_buffer = malloc(size);
  if (!output_buffer) {
    PyErr_SetString(PyExc_MemoryError,
                    "could not allocate memory for output buffer");
    return NULL;
  }

  /* die if we don't have the space */
  if (size < PYMAD_SYNTH(self).pcm.length * 4) {
    PyErr_SetString(PyExc_MemoryError, "allocated buffer too small");
    return NULL;
  }

  Py_BEGIN_ALLOW_THREADS;

  /* Synthesised samples must be converted from mad's fixed point format to the
   * consumer format -- use signed 16 bit big-endian ints on two channels.
   * Integer samples are temporarily stored in a buffer that is flushed when
   * full. */
  for (i = 0; i < PYMAD_SYNTH(self).pcm.length; i++) {
    int16_t sample;

    /* left channel */
    *(output++) = sample =
        madfixed_to_int16(PYMAD_SYNTH(self).pcm.samples[0][i]);

    /* right channel.
     * if the decoded stream is monophonic then the right channel
     * is the same as the left one */
    if (MAD_NCHANNELS(&PYMAD_FRAME(self).header) == 2)
      sample = madfixed_to_int16(PYMAD_SYNTH(self).pcm.samples[1][i]);

    *(output++) = sample;
  }

  Py_END_ALLOW_THREADS;

  pybuf = PyByteArray_FromStringAndSize((const char *)output_buffer, size);
  free(output_buffer);

  return pybuf;
}

/* return the MPEG layer */
static PyObject *py_madfile_layer(PyObject *self, PyObject *args) {
  return PyInt_FromLong(PYMAD_FRAME(self).header.layer);
}

/* return the channel mode */
static PyObject *py_madfile_mode(PyObject *self, PyObject *args) {
  return PyInt_FromLong(PYMAD_FRAME(self).header.mode);
}

/* return the stream samplerate */
static PyObject *py_madfile_samplerate(PyObject *self, PyObject *args) {
  return PyInt_FromLong(PYMAD_FRAME(self).header.samplerate);
}

/* return the stream bitrate */
static PyObject *py_madfile_bitrate(PyObject *self, PyObject *args) {
  return PyInt_FromLong(PYMAD_FRAME(self).header.bitrate);
}

/* return the emphasis value */
static PyObject *py_madfile_emphasis(PyObject *self, PyObject *args) {
  return PyInt_FromLong(PYMAD_FRAME(self).header.emphasis);
}

/* return the estimated playtime of the track, in milliseconds */
static PyObject *py_madfile_total_time(PyObject *self, PyObject *args) {
  return PyInt_FromLong(PY_MADFILE(self)->total_length);
}

/* return the current position in the track, in milliseconds */
static PyObject *py_madfile_current_time(PyObject *self, PyObject *args) {
  return PyInt_FromLong(
      mad_timer_count(PYMAD_TIMER(self), MAD_UNITS_MILLISECONDS));
}

/* seek playback to the given position, in milliseconds, from the start
 * FIXME: this implementation is really evil -- amazing that it semi-works */
static PyObject *py_madfile_seek_time(PyObject *self, PyObject *args) {
  long pos, offset;
  struct stat buf;
  int r;
  PyObject *o;
  int fnum;

  if (!PyArg_ParseTuple(args, "l", &pos) || pos < 0) {
    PyErr_SetString(PyExc_TypeError, "invalid argument");
    return NULL;
  }

  o = PyObject_CallMethod(PY_MADFILE(self)->fobject, "fileno", NULL);
  if (o == NULL) {
    PyErr_SetString(PyExc_IOError, "couldn't get fileno");
    return NULL;
  }
  fnum = PyInt_AsLong(o);
  Py_DECREF(o);

  r = fstat(fnum, &buf);
  if (r != 0) {
    PyErr_SetString(PyExc_IOError, "couldn't stat file");
    return NULL;
  }

  offset = ((double)pos / PY_MADFILE(self)->total_length) * buf.st_size;

  o = PyObject_CallMethod(PY_MADFILE(self)->fobject, "seek", "l", offset);
  if (o == NULL) {
    /* most likely no seek method -- FIXME get better checking */
    PyErr_SetString(PyExc_IOError, "couldn't seek file");
    return NULL;
  }
  Py_DECREF(o);

  mad_stream_init(&PYMAD_STREAM(self));
  mad_frame_init(&PYMAD_FRAME(self));
  mad_synth_init(&PYMAD_SYNTH(self));
  mad_timer_reset(&PYMAD_TIMER(self));
  mad_timer_set(&PYMAD_TIMER(self), 0, pos, 1000);

  return Py_None;
}
