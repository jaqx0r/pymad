/* $Id$
 *
 * python interface to libmad (the mpeg audio decoder library)
 *
 * Copyright (c) 2002 Jamie Wilkinson
 *
 * This program is free software, you may copy and/or modify as per
 * the  GNU General Public License (version 2, or at your discretion,
 * any later version).  This is the same license as libmad.
 *
 * the code py_madfile_read was copied from the madlld program, which
 * can be found at http://www.bsd-dk.dk/~elrond/audio/madlld/
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "madmodule.h"
#include "pymadfile.h"

#if PY_VERSION_HEX < 0x01060000
#define PyObject_DEL(op) PyMem_DEL((op))
#endif

PyTypeObject py_madfile_t = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "MadFile",
  sizeof(py_madfile),
  0,
  /* standard methods */
  (destructor) py_madfile_dealloc,
  (printfunc) 0,
  (getattrfunc) py_madfile_getattr,
  (setattrfunc) 0,
  (cmpfunc) 0,
  (reprfunc) 0,
  /* type categories */
  0, /* as number */
  0, /* as sequence */
  0, /* as mapping */
  0, /* hash */
  0, /* binary */
  0, /* repr */
  0, /* getattro */
  0, /* setattro */
  0, /* as buffer */
  0, /* tp_flags */
  NULL
};

/* functions */

PyObject * py_madfile_new(PyObject * self, PyObject * args) {
  py_madfile * mf;
  char * fname;
  char errmsg[512]; /* FIXME: use MSG_SIZE or something */
  unsigned long int bufsiz = 4096;
  int n;
  
  if (!PyArg_ParseTuple(args, "s|l:MadFile", &fname, &bufsiz)) {
    return NULL;
  }
  
  /* bufsiz must be an integer multiple of 4 */
#if 0
  switch (bufsiz % 4) {
  case 3: bufsiz--;
  case 2: bufsiz--;
  case 1: bufsiz--;
  default:
  }
#endif
  if ((n = bufsiz % 4)) bufsiz -= n;
  if (bufsiz <= 4096) bufsiz = 4096;
  
  mf = PyObject_NEW(py_madfile, &py_madfile_t);
  if ((mf->f = fopen(fname, "r")) == NULL) {
    snprintf(errmsg, 512, "Couldn't open file: %s", fname);
    PyErr_SetString(PyExc_IOError, errmsg);
    PyObject_DEL(mf);
    return NULL;
  }
  /* initialise the mad structs */
  mad_stream_init(&mf->stream);
  mad_frame_init(&mf->frame);
  mad_synth_init(&mf->synth);
  mad_timer_reset(&mf->timer);
  
  mf->framecount = 0;
  
  mf->buffy = malloc(bufsiz * sizeof(unsigned char));
  mf->bufsiz = bufsiz;
  
  return (PyObject *) mf;
}

static void py_madfile_dealloc(PyObject * self, PyObject * args) {
  if (PY_MADFILE(self)->f) {
    free(MAD_BUFFY(self));
    MAD_BUFFY(self) = NULL;
    MAD_BUFSIZ(self) = 0;
    fclose(PY_MADFILE(self)->f);
    mad_synth_finish(&MAD_SYNTH(self));
    /* for some reason, this function is segfaulting python when it
     * cleans up */
    /* mad_frame_finish(&MAD_FRAME(self)); */
    mad_stream_finish(&MAD_STREAM(self));
  }
  PyObject_DEL(self);
}

/* convert the mad format to an unsigned short */
static signed short int
madfixed_to_short(mad_fixed_t sample) {
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
   * The unsigned short value is formed by the least significant
   * whole part bit, followed by the 15 most significant fractional
   * part bits.
   * 
   * This algorithm was taken from input/mad/mad_engine.c in
   * alsaplayer, because the one in madlld sucked arse.
   * http://www.geocrawler.com/archives/3/15440/2001/9/0/6629549/
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

static PyObject * 
py_madfile_read(PyObject * self, PyObject * args) {
  PyObject * pybuf, * tuple; /* output objects */
  char * buffy; /* output buffer */
  unsigned int i, size;
  int nextframe = 0;
  char errmsg[512];

  /* if we are at EOF, then return None */
  if (feof(PY_MADFILE(self)->f)) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  
  /* nextframe might get set during this loop, in which case the
   * bucket needs to be refilled */
  do {
    nextframe = 0; /* reset */
    
    /* The input bucket must be filled if it becomes empty or if
     * it's the first execution on this file */
    if ((MAD_STREAM(self).buffer == NULL) ||
	(MAD_STREAM(self).error == MAD_ERROR_BUFLEN)) {
      size_t readsize, remaining;
      unsigned char * readstart;
      
      /* [1] libmad may not consume all bytes of the input buffer.
       *     If the last frame in the buffer is not wholly contained
       *     by it, then that frame's start is pointed to by the
       *     next_frame member of the mad_stream structure.  This
       *     common situation occurs when mad_frame_decode():
       *      - fails,
       *      - sets the stream error to MAD_ERROR_BUFLEN, and
       *      - sets the next_frame pointer to a non NULL value.
       *     (See also the comment marked [2] below)
       *
       *     When this occurs, the remaining unused bytes must be put
       *     back at the beginning of the buffer and taken into
       *     account before refilling the buffer.  This means that
       *     the input buffer must be large enough to hold a whole
       *     frame at the highest observable bitrate (currently
       *     448kbps).
       */
      if (MAD_STREAM(self).next_frame != NULL) {
	remaining = MAD_STREAM(self).bufend - MAD_STREAM(self).next_frame;
	memmove(MAD_BUFFY(self), MAD_STREAM(self).next_frame, remaining);
	readstart = MAD_BUFFY(self) + remaining;
	readsize = MAD_BUFSIZ(self) - remaining;
      } else
	readstart = MAD_BUFFY(self),
	  readsize = MAD_BUFSIZ(self),
	  remaining = 0;
      
      /* Fill in the buffer.  If an error occurs, make like a tree */
      readsize = fread(readstart, 1, readsize, PY_MADFILE(self)->f);
      if (readsize <= 0) {
	if (ferror(PY_MADFILE(self)->f)) {
	  snprintf(errmsg, 513, "read error: %s", strerror(errno));
	  PyErr_SetString(PyExc_IOError, errmsg);
	  return NULL;
	}
	/* again, if we're at EOF, return None */
	if (feof(PY_MADFILE(self)->f)) {
	  Py_INCREF(Py_None);
	  return Py_None;
	}
      }
      
      /* Pipe the new buffer content to libmad's stream decode
       * facility */
      mad_stream_buffer(&MAD_STREAM(self), MAD_BUFFY(self),
			readsize + remaining);
      MAD_STREAM(self).error = 0;
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
    if (mad_frame_decode(&MAD_FRAME(self), &MAD_STREAM(self))) {
      if (MAD_RECOVERABLE(MAD_STREAM(self).error)) {
	fprintf(stderr, "mad: recoverable frame level error: %s\n", mad_stream_errorstr(&MAD_STREAM(self)));
	fflush(stderr);
	/* go onto the next frame */
	nextframe = 1;
      } else {
	if (MAD_STREAM(self).error == MAD_ERROR_BUFLEN) {
	  /* not enough data to decode */
	  nextframe = 1;
	} else {
	  snprintf(errmsg, 512, "unrecoverable frame level error: %s", mad_stream_errorstr(&MAD_STREAM(self)));
	  PyErr_SetString(PyExc_RuntimeError, errmsg);
	  return NULL;
	}
      }
    }
	
  } while (nextframe);
  
  /* The characteristics of the streams first frame are printed on
   * stderr. The first frame is representative of the entire stream.
   */
#if 0
  if (PY_MADFILE(self)->framecount == 0) {
    if (printframeinfo(stderr, &MAD_FRAME(self).header)) {
      return NULL;
    }
  }
#endif
  
  /* Accounting.  The computed frame duration is in the frame header
   * structure.  It is expressed as a fixed point number whose data
   * type is mad_timer_t.  It is different from the fixed point
   * format of the samples and unlike it, cannot be directly added
   * or subtracted.  The timer module provides several functions to
   * operate on such numbers.  Be careful though, as some functions
   * of mad's timer module receive their mad_timer_t arguments by
   * value! */
  PY_MADFILE(self)->framecount++;
  mad_timer_add(&MAD_TIMER(self), MAD_FRAME(self).header.duration);
  
  /* Once decoded, the frame can be synthesised to PCM samples.
   * No errors are reported by mad_synth_frame() */
  mad_synth_frame(&MAD_SYNTH(self), &MAD_FRAME(self));
  
  /* Create the buffer to store the PCM samples in, so python can
   * use it.  We do 4 pointer increments per sample in the buffer,
   * so make it 4 times as big as the number of samples */
  size = MAD_SYNTH(self).pcm.length * 4;
  pybuf = PyBuffer_New(size);
  
  /* create a tuple object so we can get at the stuff in buf */
  tuple = PyTuple_New(1);
  Py_INCREF(pybuf);
  PyTuple_SET_ITEM(tuple, 0, pybuf);
  if (!PyArg_ParseTuple(tuple, "t#", &buffy, &size)) {
    PyErr_SetString(PyExc_TypeError, "borken buffer tuple!");
    return NULL;
  }
  /* no longer use tuple */
  Py_DECREF(tuple);
  
  /* die if we don't have the space */
  if (size < MAD_SYNTH(self).pcm.length * 4) {
    PyErr_SetString(PyExc_MemoryError, "allocated buffer too small");
    return NULL;
  }
  
  Py_BEGIN_ALLOW_THREADS
    
    /* Synthesised samples must be converted from mad's fixed point
     * format to the consumer format.  Here we use signed 16 bit
     * big endian ints on two channels.  Integer samples are 
     * temporarily stored in a buffer that is flushed when full. */
    for (i = 0; i < MAD_SYNTH(self).pcm.length; i++) {
      signed short sample;
      
      /* left channel */
      sample = madfixed_to_short(MAD_SYNTH(self).pcm.samples[0][i]);
      *(buffy++) = sample & 0xFF;
      *(buffy++) = sample >> 8;
      
      /* right channel. 
       * if the decoded stream is monophonic then the right channel
       * is the same as the left one */
      if (MAD_NCHANNELS(&MAD_FRAME(self).header) == 2)
	sample = madfixed_to_short(MAD_SYNTH(self).pcm.samples[1][i]);
      *(buffy++) = sample & 0xFF;
      *(buffy++) = sample >> 8;
    }
  
  Py_END_ALLOW_THREADS
    
    return pybuf;
}

/* get stream info from mad */
static PyObject * py_madfile_info(PyObject * self, PyObject * args) {
  /* FIXME: actually implement this function
    PyObject * dict, * item;
    char * key, * val;
    
    dict = PyDict_New();
    
    PyDict_SetItemString
  */
  return NULL;
}

/* housekeeping */

static PyMethodDef madfile_methods[] = {
  { "read", py_madfile_read, METH_VARARGS, "" },
  { "info", py_madfile_info, METH_VARARGS, "" },
  { NULL, 0, 0, NULL }
};

static PyObject * py_madfile_getattr(PyObject * self, char * name) {
  return Py_FindMethod(madfile_methods, self, name);
}
