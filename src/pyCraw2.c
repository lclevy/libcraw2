/*
 * an -experimental- Python extension for libcraw2
 * 
 * gives easy access to:
 * - lossless jpeg properties
 * - lossless jpeg decoding
 * - TIFF tags values and content
 * - RGB or YUV data
 */

/*
 * https://docs.python.org/3/extending/newtypes_tutorial.html
 * you must setup C:\Users\username\AppData\Local\Programs\Python\Python36\libs and .. AppData\Local\Programs\Python\Python36\include
 */

/*
 * possible improvements:
 * - cr2 = craw2("pic.cr2")
 * - cr2.getRawData() returns directly a 2D array
 * ...
 */ 
 
#include <Python.h>
#include <structmember.h>

// public interface to libcraw2
#include"craw2.h"
#include"tiff.h"
#include"rgb.h"
#include"yuv.h"

#define NDIM 3

typedef struct {
  PyObject_HEAD
  //char *typeStr;	  
  struct cr2file cr2;
  Py_ssize_t shape[NDIM]; // not used
  Py_ssize_t strides[NDIM];
  Py_ssize_t suboffsets[NDIM];
} craw2_Object;

typedef struct {
  PyObject_HEAD
  struct cr2image img;
  craw2_Object *cr2obj;
} cr2image_Object;

// for exception
static PyObject *craw2Error;

//forward definition
static PyObject * cr2open(PyObject *self, PyObject *args);
static PyObject * cr2close(PyObject *self, PyObject *args);

// constructor
static PyObject * craw2_new(PyTypeObject *type, PyObject *args)
{
  craw2_Object *self;
  //char filename[260];

  self = (craw2_Object *)type->tp_alloc(type, 0);

  setVerboseLevel(0);
  puts("craw2_new");
  return (PyObject *)self;
}

// destructor
static void craw2_dealloc(craw2_Object* self)
{
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject * cr2open(PyObject *self, PyObject *args)
{
  char *filename = 0;
  craw2_Object *cr2obj = (craw2_Object *)self;

  struct cr2file *cr2 = &(cr2obj->cr2);
  //puts("cr2open");
  if (!PyArg_ParseTuple(args, "s", &filename))
    return NULL;

  setVerboseLevel(0);
  if (cRaw2Open(cr2, filename)<0) {
    PyErr_SetString(craw2Error, "cRaw2Open failed");
    return NULL;
  }
  if (cRaw2GetTags(cr2) <0) {
    PyErr_SetString(craw2Error, "cRaw2GetTags failed");
    return(NULL);
  }
  return Py_None;
}

// to close and free a CR2 file
static PyObject * cr2close(PyObject *self, PyObject *args)
{
  craw2_Object *cr2obj = (craw2_Object *)self;
  struct cr2file *cr2 = &(cr2obj->cr2);
  jpegFree(&(cr2->jpeg));
  cRaw2Close(cr2);
  
  return Py_None;
}

/* decompress and unslice
*/
static PyObject * cr2decode(PyObject *self)
{
  craw2_Object *cr2obj = (craw2_Object *)self;
  struct cr2file *cr2 = &(cr2obj->cr2);

  jpegParseHeaders(cr2->file, cr2->stripOffsets, &(cr2->jpeg));

  cr2->jpeg.rawBuffer = 0;
  cr2->jpeg.rawBufferLen = 0;
  jpegDecompress(&(cr2->jpeg), cr2->file, cr2->stripOffsets, cr2->stripByteCounts);

  cRaw2Unslice(cr2);
  //cr2obj->type = cr2->type;
  cRaw2SetType(cr2);

  return Py_None;
}

/*
* return RAW data as an 1D array
*/
// memory leak (pyBuf)
static PyObject * cr2getRawData(PyObject *self) {
  craw2_Object *cr2obj = (craw2_Object *)self;
  struct cr2file *cr2 = &(cr2obj->cr2);
  PyObject *pixels = NULL;

  Py_buffer *pyBuf = (Py_buffer*)malloc(sizeof(Py_buffer)); // http://docs.python.org/2/c-api/buffer.html#the-new-style-py-buffer-struct
  pyBuf->obj = NULL;
  pyBuf->buf = cr2->unsliced;
  pyBuf->len = cr2->unslicedLen * sizeof(short);
  pyBuf->readonly = 0; // writable
  pyBuf->itemsize = sizeof(short);
  pyBuf->format = "H";
  pyBuf->ndim = 1;
  pyBuf->shape = NULL;
  pyBuf->strides = NULL;
  pyBuf->suboffsets = NULL;
  pyBuf->internal = NULL;

  pixels = PyMemoryView_FromBuffer(pyBuf);
  Py_INCREF(pixels);
  return pixels;
}

// double free using this method
static int craw2_unsliced_getbuffer(PyObject *obj, Py_buffer *view, int flags) {

  if (view == NULL) {
    PyErr_SetString(PyExc_ValueError, "NULL view in getbuffer");
    return -1;
  }

  craw2_Object *cr2obj = (craw2_Object *)obj;
  struct cr2file *cr2 = &(cr2obj->cr2);
  view->obj = (PyObject*)cr2obj;
  view->buf = (void*)cr2->unsliced;
  view->len = cr2->unslicedLen * sizeof(short);
  view->readonly = 0; // writable
  view->itemsize = sizeof(short);
  view->format = "H";
  view->ndim = 1;
  view->shape = &cr2->unslicedLen;
  view->strides = &view->itemsize;
  view->suboffsets = NULL;
  view->internal = NULL;

  Py_INCREF(cr2obj);
  return 0;
}
//https://jakevdp.github.io/blog/2014/05/05/introduction-to-the-python-buffer-protocol/
static PyBufferProcs craw2_unsliced_buffer = {
  // this definition is only compatible with Python 3.3 and above
  (getbufferproc)craw2_unsliced_getbuffer,
  (releasebufferproc)0,  // we do not require any special release function
};

static PyObject * cr2yuvInterpolate(PyObject *self, PyObject *args)
{
  craw2_Object *cr2obj = (craw2_Object *)self;
  struct cr2file *cr2 = &(cr2obj->cr2);
  puts("yuvInterpolate");

  yuvInterpolate(cr2);

  return Py_None;
}

/*
* return YUV mask
*/
static PyObject * cr2getYuvMask(PyObject *self) {
  craw2_Object *cr2obj = (craw2_Object *)self;
  struct cr2file *cr2 = &(cr2obj->cr2);
  PyObject *bmask = NULL;
  Py_buffer *pyBuf;

  if (cr2->jpeg.bits != 15 || cr2->jpeg.hsf != 2)
    return NULL;

  pyBuf = (Py_buffer*)malloc(sizeof(Py_buffer)); // http://docs.python.org/2/c-api/buffer.html#the-new-style-py-buffer-struct
  pyBuf->obj = NULL;
  pyBuf->buf = cr2->yuvMask;
  pyBuf->len = cr2->jpeg.wide*cr2->jpeg.ncomp*cr2->jpeg.high * sizeof(char);
  pyBuf->readonly = 1;
  pyBuf->itemsize = sizeof(char);
  pyBuf->format = "B"; // this is also the default
  pyBuf->ndim = 1;
  pyBuf->shape = NULL;
  pyBuf->strides = NULL;
  pyBuf->suboffsets = NULL;
  pyBuf->internal = NULL;

  bmask = PyMemoryView_FromBuffer(pyBuf);
  Py_INCREF(bmask);
  return bmask;
}


static PyObject * cr2FindImageBorders(PyObject *self, PyObject *args) {
  craw2_Object *cr2obj = (craw2_Object *)self;
  struct cr2file *cr2 = &(cr2obj->cr2);
  int i = 0, len;
  unsigned short corners[4];
  PyObject *item, *obj, *seq;

  //printf("list=%d\n", PyList_Check(args));

  // http://effbot.org/zone/python-capi-sequences.htm
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    return NULL;
  }
  else {
    seq = PySequence_Fast(obj, "cr2FindImageBorders: expected a sequence");
    len = (int)PySequence_Size(obj);
    if (len != 4) {
      cRaw2FindImageBorders(cr2, 0);
      return Py_None;
    }
    for (i = 0; i<len; i++) {
      item = PySequence_Fast_GET_ITEM(seq, i);
      if (!PyLong_Check(item))
        return Py_None;
      corners[i] = (unsigned short)PyLong_AsLong(item);
      printf("%d ", corners[i]);
      //Py_DECREF(item);
    }
    printf("\n");
    cRaw2FindImageBorders(cr2, corners);
    Py_DECREF(seq);
  }

  return Py_None;
}

/*
 * cr2getRggbWb
 */
static PyObject * cr2getRggbWb(PyObject *self) {
  craw2_Object *cr2obj = (craw2_Object *)self;
  struct cr2file *cr2 = &(cr2obj->cr2);
  int i;

  PyObject *list = PyList_New(4);
  for (i = 0; i < 4; i++)
    PyList_SetItem(list, i, Py_BuildValue("i", cr2->wbRggb[i]));

  Py_INCREF(list);
  return list;
}

static PyObject * cr2getYuvWb(PyObject *self) {
  craw2_Object *cr2obj = (craw2_Object *)self;
  struct cr2file *cr2 = &(cr2obj->cr2);
  int i;

  PyObject *list = PyList_New(4);
  for (i = 0; i < 4; i++)
    PyList_SetItem(list, i, Py_BuildValue("i", cr2->srawRggb[i]));

  Py_INCREF(list);
  return list;
}


/*
* returns TIFF tag content as an array of values ('a')
*/
static PyObject * cr2_getTagContent(PyObject *self, PyObject *args) {
  struct tiff_tag tag;
  unsigned char *tagvalBuf = 0, *buf;
  unsigned short *tagvalBuf16;
  unsigned long *tagvalBuf32;
  char tagAccess;
  craw2_Object *cr2obj = (craw2_Object *)self;
  struct cr2file *cr2 = &(cr2obj->cr2);
  uint32_t i, typeLen;
  PyObject *value=NULL, *list=NULL;

  // only tag.val, tag.type and tag.len are needed  
  if (!PyArg_ParseTuple(args, "(ill)c", &(tag.type), &(tag.len), &(tag.val), &tagAccess))
    return NULL;
  //printf("%d %ld %ld %c\n", tag.type, tag.len, tag.val, tagAccess);
  if (tagAccess != 'b' && tagAccess != 'a')
    return NULL;
  if (cRaw2GetTagValue(&tag, &tagvalBuf, cr2->file)<0)
    return NULL;
  //printf("%p\n", tagvalBuf);
  buf = tagvalBuf; // because tagvalBuf will be incremented, so we will use "free(buf)" 
  tagvalBuf16 = (unsigned short *)tagvalBuf;
  tagvalBuf32 = (unsigned long *)tagvalBuf;

  if (tagAccess == 'a') {
    typeLen = getTagTypeLen(tag.type);
    list = PyList_New(0);
    for (i = 0; i<tag.len; i++) {
      //printf("%p %p %p\n", tagvalBuf, tagvalBuf16, tagvalBuf32);
      switch (tag.type) {
      case TIFFTAG_TYPE_UCHAR:
      case TIFFTAG_TYPE_STRING:
        value = Py_BuildValue("b", *tagvalBuf);
        tagvalBuf++;
        break;
      case TIFFTAG_TYPE_USHORT:
        value = Py_BuildValue("H", *tagvalBuf16);
        tagvalBuf16++;
        break;
      case TIFFTAG_TYPE_SHORT:
        value = Py_BuildValue("h", *tagvalBuf16);
        tagvalBuf16++;
        break;
      case TIFFTAG_TYPE_ULONG:
        value = Py_BuildValue("k", *tagvalBuf32);
        tagvalBuf32++;
        break;
      case TIFFTAG_TYPE_LONG:
        value = Py_BuildValue("l", *tagvalBuf32);
        tagvalBuf32++;
        break;
      case TIFFTAG_TYPE_URATIONAL:
        value = Py_BuildValue("(kk)", *tagvalBuf32, *(tagvalBuf32 + 1));
        tagvalBuf32 += 2;
        break;
      case TIFFTAG_TYPE_RATIONAL:
        value = Py_BuildValue("(ll)", *tagvalBuf32, *(tagvalBuf32 + 1));
        tagvalBuf32 += 2;
        break;
      default:
        fprintf(stderr, "cr2_getTagContent: unknown tag.type=%d\n", tag.type);
      }
      if (PyList_Append(list, value)<0) {
        fprintf(stderr, "cr2_getTagContent: PyList_Append\n");
        return Py_None;
      }
    } // for    
    free(buf);
    Py_INCREF(list);
    return list;
  } // tagAccess=='a'  
  free(buf);
  return Py_None;
}

/*
* returns TIFF tags as a Python dictionary
*/
static PyObject * cr2getTiffTags(PyObject *self) {
  craw2_Object *cr2obj = (craw2_Object *)self;
  struct cr2file *cr2 = &(cr2obj->cr2);
  PyObject *pydict = PyDict_New();
  PyObject *value, *key;
  char buf[20];
  struct tiff_tag *tag;

  tag = cr2->tagList;
  while (tag != 0) {
    /*printf("%9s 0x%-4x/%-5d type=%2d len=%6ld val=0x%-6lx/%-6ld (at 0x%lx)\n",
    tag->context, tag->id, tag->id, tag->type, tag->len, tag->val, tag->val, tag->offset);*/

    // { "context_id": ( id, type, len, val, offset ), ... }
    sprintf(buf, "0x%x_0x%x", tag->context, tag->id);
    value = Py_BuildValue("(i,i,i,k,k)", tag->id, tag->type, tag->len, tag->val, tag->offset);
    key = Py_BuildValue("s", buf);
    PyDict_SetItem(pydict, key, value);

    tag = tag->next;
  }

  Py_INCREF(pydict);
  return pydict;
}

static PyObject * cr2_saveTiff(PyObject *self, PyObject *args) {
  craw2_Object *cr2obj = (craw2_Object *)self;
  char *filename = 0;
  struct cr2file *cr2 = &(cr2obj->cr2);

  if (!PyArg_ParseTuple(args, "s", &filename))
    return NULL;

  exportRggbTiff(cr2, filename);

  return Py_None;
}

// forward definitions
static cr2image_Object* cr2createimg(PyObject *self, PyObject *args);
static cr2image_Object * cr2createimgFromYuv(PyObject *self, PyObject *args);


static PyMethodDef craw2_methods[] = {
  { "open", (PyCFunction)cr2open, METH_VARARGS, "open a CR2 file" },
  { "close", (PyCFunction)cr2close, METH_NOARGS, "close a CR2 file" },
  { "decode", (PyCFunction)cr2decode, METH_NOARGS, "decompress and unslice RAW data" },
  { "getRawData", (PyCFunction)cr2getRawData, METH_NOARGS, "returns RAW data" },
  { "findBorders", (PyCFunction)cr2FindImageBorders, METH_VARARGS, "find RGGB borders" },
  { "getRggbWb", (PyCFunction)cr2getRggbWb, METH_NOARGS, "get RGGB white balance ratios" },
  { "getYuvWb", (PyCFunction)cr2getYuvWb, METH_NOARGS, "get YUV white balance ratios" },
  { "createImage", (PyCFunction)cr2createimg, METH_VARARGS, "create image from RAW data" },
  { "saveTiff", (PyCFunction)cr2_saveTiff, METH_VARARGS, "save RGGB data as TIFF" },
  { "getTiffTags", (PyCFunction)cr2getTiffTags, METH_NOARGS, "returns list of TIFF tags" },
  { "getTagContent", (PyCFunction)cr2_getTagContent, METH_VARARGS, "get TIFF tag content" },
  { "yuvInterpolate", (PyCFunction)cr2yuvInterpolate, METH_NOARGS, "interpolate YUV data" },
  { "createImageFromYuv", (PyCFunction)cr2createimgFromYuv, METH_VARARGS, "YUV conversion to RGB, with WB applied" },
  { "getYuvMask", (PyCFunction)cr2getYuvMask, METH_VARARGS, "get YUV mask data" },
  { NULL }  /* Sentinel */
};

static PyMemberDef craw2_members[] = { // http://docs.python.org/2/c-api/structures.html?highlight=pymemberdef#PyMemberDef
  { "high", T_USHORT, offsetof(craw2_Object, cr2) + offsetof(struct cr2file, jpeg) + offsetof(struct lljpeg, high), READONLY, "jpeg_high" },
  { "wide", T_USHORT, offsetof(craw2_Object, cr2) + offsetof(struct cr2file, jpeg) + offsetof(struct lljpeg, wide), READONLY, "jpeg_wide" },
  { "ncomp", T_UBYTE, offsetof(craw2_Object, cr2) + offsetof(struct cr2file, jpeg) + offsetof(struct lljpeg, ncomp), READONLY, "jpeg_ncomp" },
  { "bits", T_UBYTE, offsetof(craw2_Object, cr2) + offsetof(struct cr2file, jpeg) + offsetof(struct lljpeg, bits), READONLY, "jpeg_bits" },
  { "hsf", T_UBYTE, offsetof(craw2_Object, cr2) + offsetof(struct cr2file, jpeg) + offsetof(struct lljpeg, hsf), READONLY, "jpeg_hsf" },
  { "vsf", T_UBYTE, offsetof(craw2_Object, cr2) + offsetof(struct cr2file, jpeg) + offsetof(struct lljpeg, vsf), READONLY, "jpeg_vsf" },
  { "type", T_INT, offsetof(craw2_Object, cr2) + offsetof(struct cr2file, type), READONLY, "cr2_type" },
  //{ "typeStr", T_STRING, offsetof(craw2_Object, typeStr), READONLY, "typeStr" },
  { "lborder", T_SHORT, offsetof(craw2_Object, cr2) + offsetof(struct cr2file, lBorder), READONLY, "lborder" },
  { "rborder", T_SHORT, offsetof(craw2_Object, cr2) + offsetof(struct cr2file, rBorder), READONLY, "rborder" },
  { "tborder", T_SHORT, offsetof(craw2_Object, cr2) + offsetof(struct cr2file, tBorder), READONLY, "tborder" },
  { "bborder", T_SHORT, offsetof(craw2_Object, cr2) + offsetof(struct cr2file, bBorder), READONLY, "bborder" },
  { "vshift", T_SHORT, offsetof(craw2_Object, cr2) + offsetof(struct cr2file, vShift), READONLY, "vshift" },
  { "height", T_SHORT, offsetof(craw2_Object, cr2) + offsetof(struct cr2file, height), READONLY, "height" },
  { "width", T_SHORT, offsetof(craw2_Object, cr2) + offsetof(struct cr2file, width), READONLY, "width" },
  { NULL }  /* Sentinel */
};

static PyTypeObject craw2_Type = {
  PyObject_HEAD_INIT(NULL)
  .tp_name = "craw2",
  .tp_doc = "craw2 object",
  .tp_basicsize = sizeof(craw2_Object),
  .tp_dealloc = (destructor)craw2_dealloc,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  //.tp_init = (initproc) Custom_init,
  .tp_as_buffer = &craw2_unsliced_buffer,
  .tp_methods = craw2_methods,
  .tp_members = craw2_members,
  .tp_new = craw2_new
};

/*
* cr2image functions
*/

// contructor
static PyObject * cr2image_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  cr2image_Object *self;
  puts("cr2image_new");

  self = (cr2image_Object *)type->tp_alloc(type, 0);
  if (self != NULL) {
  }
  return (PyObject *)self;
}

static void cr2img_dealloc(cr2image_Object * self)
{
  cr2image_Object *cr2img = self;
  puts("cr2img_dealloc");

  /*if (cr2obj->typeStr!=0)
  free(cr2obj->typeStr);*/
  cr2ImageFree(&(self->img));

  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject * cr2img_getPixels(PyObject *self) {
  cr2image_Object *cr2img = (cr2image_Object *)self;

  craw2_Object *cr2obj = cr2img->cr2obj;
  struct cr2file *cr2 = &(cr2obj->cr2);

  struct cr2image *img = (struct cr2image *)&(cr2img->img);
  PyObject *pixels = NULL;

  Py_buffer *pyBuf = (Py_buffer*)malloc(sizeof(Py_buffer)); // http://docs.python.org/2/c-api/buffer.html#the-new-style-py-buffer-struct
  pyBuf->obj = NULL;
  pyBuf->buf = img->pixels;
  //printf("%d x %d\n", img->width, img->height);
  if (img->isRGGB)
    pyBuf->len = img->width * img->height * 4 * sizeof(short);
  else
    pyBuf->len = img->width * img->height * 3 * sizeof(short);
  pyBuf->readonly = 0; // writable
  pyBuf->itemsize = sizeof(short);
  pyBuf->format = "H";
  pyBuf->ndim = 1;
  pyBuf->shape = NULL;
  pyBuf->strides = NULL;
  pyBuf->suboffsets = NULL;
  pyBuf->internal = NULL;

  pixels = PyMemoryView_FromBuffer(pyBuf);
  Py_INCREF(pixels);
  return pixels;
}

static PyObject * cr2img_interpolate(PyObject *self, PyObject *args) {
  cr2image_Object *cr2img = (cr2image_Object *)self;
  int method = 0;
  struct cr2image *img;
  craw2_Object *cr2obj = cr2img->cr2obj;
  struct cr2file *cr2 = &(cr2obj->cr2);

  if (!PyArg_ParseTuple(args, "i", &method))
    return NULL;
  //printf("method=%d\n", method);

  img = (struct cr2image *)&(cr2img->img);

  switch (method) {
    // other interpolation methods
  case INTERPOLATION_BILINEAR:
  default:
    interpolateRGB_bilinear(img);
  }

  return Py_None;
}

static PyObject * cr2img_saveRgbTiff(PyObject *self, PyObject *args) {
  cr2image_Object *cr2img = (cr2image_Object *)self;
  char *filename = 0;
  struct cr2image *img;

  craw2_Object *cr2obj = cr2img->cr2obj;
  struct cr2file *cr2 = &(cr2obj->cr2);

  img = (struct cr2image *)&(cr2img->img);

  if (!PyArg_ParseTuple(args, "s", &filename))
    return NULL;
  printf("filename=%s\n", filename);

  exportRgbTiff(img, filename, 0);

  return Py_None;
}

static PyMethodDef cr2image_methods[] = {
  { "getPixels", (PyCFunction)cr2img_getPixels, METH_NOARGS, "get pixel data" },
  { "interpolate", (PyCFunction)cr2img_interpolate, METH_VARARGS, "interpolate missing RGB data" },
  { "saveRgbTiff", (PyCFunction)cr2img_saveRgbTiff, METH_VARARGS, "save 16bits RGB data as TIFF" },
  { NULL }  /* Sentinel */
};

static PyMemberDef cr2image_members[] = { // http://docs.python.org/2/c-api/structures.html?highlight=pymemberdef#PyMemberDef
  { "height", T_USHORT, offsetof(cr2image_Object, img) + offsetof(struct cr2image, height), READONLY, "height" },
  { "width", T_USHORT, offsetof(cr2image_Object, img) + offsetof(struct cr2image, width), READONLY, "width" },
  { NULL }  /* Sentinel */
};

static PyTypeObject cr2image_Type = {
  PyObject_HEAD_INIT(NULL)
  .tp_name = "cr2img",
  .tp_doc = "cr2image object",
  .tp_basicsize = sizeof(cr2image_Object),
  .tp_dealloc = (destructor)cr2img_dealloc,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_methods = cr2image_methods,
  .tp_members = cr2image_members,
  .tp_new = cr2image_new
};


// to create an cr2image from craw2 object by calling createImg() 
static cr2image_Object * cr2createimg(PyObject *self, PyObject *args)
{
  cr2image_Object *cr2img;
  craw2_Object *cr2obj = (craw2_Object *)self;

  if (cr2obj->cr2.jpeg.bits == 15) {
    fprintf(stderr, "must be RGGB data\n");
    return NULL;
  }

  cr2img = PyObject_New(cr2image_Object, &cr2image_Type);
  if (cr2img == NULL)
    return NULL;

  cr2ImageNew(&(cr2img->img));
  cr2img->cr2obj = cr2obj; // keep link with parent craw2 object
  Py_INCREF(cr2obj);

  cr2ImageCreate(&(cr2obj->cr2), &(cr2img->img), 0);

  Py_INCREF(cr2img);
  return (cr2img);
}

/*
* to create an cr2image from yuv craw2 object by calling yuv2rgb()
* conversion to RGB and WB
*/
static cr2image_Object * cr2createimgFromYuv(PyObject *self, PyObject *args)
{
  cr2image_Object *cr2img;
  craw2_Object *cr2obj = (craw2_Object *)self;

  if (cr2obj->cr2.jpeg.bits != 15) {
    fprintf(stderr, "must be YUV data\n");
    return NULL;
  }

  cr2img = PyObject_New(cr2image_Object, &cr2image_Type);
  if (cr2img == NULL)
    return NULL;

  cr2ImageNew(&(cr2img->img));
  cr2img->img.isRGGB = 0; // not RGGB, is YUV
  cr2img->cr2obj = cr2obj; // keep link with parent craw2 object
  Py_INCREF(cr2obj);

  yuv2rgb(&(cr2obj->cr2), &(cr2img->img));

  Py_INCREF(cr2img);
  return (cr2img);
}

static PyMethodDef module_methods[] = {
	{ NULL }  /* Sentinel */
};

static struct PyModuleDef craw2module = {
	PyModuleDef_HEAD_INIT,
	"craw2",   /* name of module */
	NULL, /* module documentation, may be NULL */
	-1,       /* size of per-interpreter state of the module,
			  or -1 if the module keeps state in global variables. */
	module_methods
};

PyMODINIT_FUNC
PyInit_craw2(void)
{
  PyObject *m;

  setVerboseLevel(0);

  craw2_Type.tp_new = PyType_GenericNew;
  if (PyType_Ready(&craw2_Type) < 0)
	  return NULL;
  cr2image_Type.tp_new = PyType_GenericNew;
  if (PyType_Ready(&cr2image_Type) < 0)
	  return NULL;

  m = PyModule_Create(&craw2module);

  Py_INCREF(&craw2_Type);
  PyModule_AddObject(m, "craw2", (PyObject *)&craw2_Type);

  craw2Error = PyErr_NewException("craw2.error", NULL, NULL);
  Py_INCREF(craw2Error);
  PyModule_AddObject(m, "error", craw2Error);
  
  Py_INCREF(&cr2image_Type);
  PyModule_AddObject(m, "cr2img", (PyObject *)&cr2image_Type);
  
  if (PyModule_AddIntConstant(m, "INTERPOLATION_BILINEAR", INTERPOLATION_BILINEAR)<0)
    fprintf(stderr, "PyModule_AddIntConstant INTERPOLATION_BILINEAR\n");;

  return m;
}
