/*
 * Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "PyCUDA.h"

#include "cudaMappedMemory.h"
#include "cudaFont.h"


//-------------------------------------------------------------------------------
// PyCUDA_FreeMalloc
void PyCUDA_FreeMalloc( PyObject* capsule )
{
	printf(LOG_PY_UTILS "freeing cudaMalloc memory\n");

	void* ptr = PyCapsule_GetPointer(capsule, CUDA_MALLOC_MEMORY_CAPSULE);

	if( !ptr )
	{
		printf(LOG_PY_UTILS "PyCUDA_FreeMalloc() failed to get pointer from PyCapsule container\n");
		return;
	}

	if( CUDA_FAILED(cudaFree(ptr)) )
	{
		printf(LOG_PY_UTILS "failed to free cudaMalloc memory with cudaFree()\n");
		return;
	}
}


// PyCUDA_RegisterMemory
PyObject* PyCUDA_RegisterMemory( void* gpuPtr, bool freeOnDelete )
{
	if( !gpuPtr )
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "RegisterMemory() was provided NULL memory pointers");
		return NULL;
	}

	// create capsule object
	PyObject* capsule = PyCapsule_New(gpuPtr, CUDA_MALLOC_MEMORY_CAPSULE, freeOnDelete ? PyCUDA_FreeMalloc : NULL);

	if( !capsule )
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "RegisterMemory() failed to create PyCapsule container");
		
		if( freeOnDelete )
			CUDA(cudaFree(gpuPtr));

		return NULL;
	}

	return capsule;
}


// PyCUDA_Malloc
PyObject* PyCUDA_Malloc( PyObject* self, PyObject* args )
{
	int size = 0;

	if( !PyArg_ParseTuple(args, "i", &size) )
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "cudaMalloc() failed to parse size argument");
		return NULL;
	}
		
	if( size <= 0 )
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "cudaMalloc() requested size is negative or zero");
		return NULL;
	}

	// allocate memory
	void* gpuPtr = NULL;

	if( !cudaMalloc(&gpuPtr, size) )
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "cudaMalloc() failed");
		return NULL;
	}

	return PyCUDA_RegisterMemory(gpuPtr);
}


//-------------------------------------------------------------------------------
// PyCUDA_FreeMapped
void PyCUDA_FreeMapped( PyObject* capsule )
{
	printf(LOG_PY_UTILS "freeing CUDA mapped memory\n");

	void* ptr = PyCapsule_GetPointer(capsule, CUDA_MAPPED_MEMORY_CAPSULE);

	if( !ptr )
	{
		printf(LOG_PY_UTILS "PyCUDA_FreeMapped() failed to get pointer from PyCapsule container\n");
		return;
	}

	if( CUDA_FAILED(cudaFreeHost(ptr)) )
	{
		printf(LOG_PY_UTILS "failed to free CUDA mapped memory with cudaFreeHost()\n");
		return;
	}
}


// PyCUDA_RegisterMappedMemory
PyObject* PyCUDA_RegisterMappedMemory( void* cpuPtr, void* gpuPtr, bool freeOnDelete )
{
	if( !cpuPtr || !gpuPtr )
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "RegisterMappedMemory() was provided NULL memory pointers");
		return NULL;
	}

	if( cpuPtr != gpuPtr )
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "RegisterMappedMemory() pointers don't match");
		
		if( freeOnDelete )
			CUDA(cudaFreeHost(cpuPtr));

		return NULL;
	}

	// create capsule object
	PyObject* capsule = PyCapsule_New(cpuPtr, CUDA_MAPPED_MEMORY_CAPSULE, freeOnDelete ? PyCUDA_FreeMapped : NULL);

	if( !capsule )
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "RegisterMappedMemory() failed to create PyCapsule container");
		
		if( freeOnDelete )
			CUDA(cudaFreeHost(cpuPtr));

		return NULL;
	}

	return capsule;
}


// PyCUDA_AllocMapped
PyObject* PyCUDA_AllocMapped( PyObject* self, PyObject* args )
{
	int size = 0;

	if( !PyArg_ParseTuple(args, "i", &size) )
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "cudaAllocMapped() failed to parse size argument");
		return NULL;
	}
		
	if( size <= 0 )
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "cudaAllocMapped() requested size is negative or zero");
		return NULL;
	}

	// allocate memory
	void* cpuPtr = NULL;
	void* gpuPtr = NULL;

	if( !cudaAllocMapped(&cpuPtr, &gpuPtr, size) )
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "cudaAllocMapped() failed");
		return NULL;
	}

	return PyCUDA_RegisterMappedMemory(cpuPtr, gpuPtr);
}


//-------------------------------------------------------------------------------

static PyMethodDef pyCUDA_Functions[] = 
{
	{ "cudaMalloc", (PyCFunction)PyCUDA_Malloc, METH_VARARGS, "Allocated CUDA memory on the GPU with cudaMalloc()" },
	{ "cudaAllocMapped", (PyCFunction)PyCUDA_AllocMapped, METH_VARARGS, "Allocate CUDA ZeroCopy mapped memory" },
	{NULL}  /* Sentinel */
};

// Register functions
PyMethodDef* PyCUDA_RegisterFunctions()
{
	return pyCUDA_Functions;
}


//-------------------------------------------------------------------------------

// PyFont container
typedef struct {
	PyObject_HEAD
	cudaFont* font;

	// colors
	PyObject* black;
	PyObject* white;
	PyObject* gray;
	PyObject* brown;
	PyObject* tan;
	PyObject* red;
	PyObject* green;
	PyObject* blue;
	PyObject* cyan;
	PyObject* lime;
	PyObject* yellow;
	PyObject* orange;
	PyObject* purple;
	PyObject* magenta;

} PyFont_Object;


// New
static PyObject* PyFont_New( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
	printf(LOG_PY_UTILS "PyFont_New()\n");
	
	// allocate a new container
	PyFont_Object* self = (PyFont_Object*)type->tp_alloc(type, 0);
	
	if( !self )
	{
		PyErr_SetString(PyExc_MemoryError, LOG_PY_UTILS "cudaFont tp_alloc() failed to allocate a new object");
		printf(LOG_PY_UTILS "cudaFont tp_alloc() failed to allocate a new object\n");
		return NULL;
	}
	

	#define INIT_COLOR(color, r, g, b)		\
		self->color = Py_BuildValue("(ffff)", r, g, b, 255.0);	\
		if( !self->color ) {							\
			Py_DECREF(self);							\
			return NULL;								\
		}

	INIT_COLOR(black,   0.0, 0.0, 0.0);									
	INIT_COLOR(white,   255.0, 255.0, 255.0);
	INIT_COLOR(gray,	128.0, 128.0, 128.0);
	INIT_COLOR(brown,	165.0, 42.0, 42.0);
	INIT_COLOR(tan,	210.0, 180.0, 140.0);
	INIT_COLOR(red,     255.0, 255.0, 255.0);
	INIT_COLOR(green,   0.0, 200.0, 128.0);
	INIT_COLOR(blue,    0.0, 0.0, 255.0);
	INIT_COLOR(cyan,    0.0, 255.0, 255.0);
	INIT_COLOR(lime,    0.0, 255.0, 0.0);
	INIT_COLOR(yellow,  255.0, 255.0, 0.0);
	INIT_COLOR(orange,  255.0, 165.0, 0.0);
	INIT_COLOR(purple,  128.0, 0.0, 128.0);
	INIT_COLOR(magenta, 255.0, 0.0, 255.0);

	self->font = NULL;
	return (PyObject*)self;
}


// Init
static int PyFont_Init( PyFont_Object* self, PyObject *args, PyObject *kwds )
{
	printf(LOG_PY_UTILS "PyFont_Init()\n");
	
	// parse arguments
	const char* bitmap = "fontmapA.png";
	static char* kwlist[] = {"bitmap", NULL};

	if( !PyArg_ParseTupleAndKeywords(args, kwds, "|s", kwlist, &bitmap))
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "pyFont.__init()__ failed to parse args tuple");
		return -1;
	}
  
	// create the font
	cudaFont* font = cudaFont::Create(bitmap);

	if( !font )
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "failed to create cudaFont object");
		return -1;
	}

	self->font = font;
	return 0;
}


// Deallocate
static void PyFont_Dealloc( PyFont_Object* self )
{
	printf(LOG_PY_UTILS "PyFont_Dealloc()\n");

	// free the font
	if( self->font != NULL )
	{
		delete self->font;
		self->font = NULL;
	}
	
	// free the color objects
	Py_XDECREF(self->black);
	Py_XDECREF(self->white);
	Py_XDECREF(self->gray);
	Py_XDECREF(self->brown);
	Py_XDECREF(self->tan);
	Py_XDECREF(self->red);
	Py_XDECREF(self->green);
	Py_XDECREF(self->blue);
	Py_XDECREF(self->cyan);
	Py_XDECREF(self->lime);
	Py_XDECREF(self->yellow);
	Py_XDECREF(self->orange);
	Py_XDECREF(self->purple);
	Py_XDECREF(self->magenta);

	// free the container
	Py_TYPE(self)->tp_free((PyObject*)self);
}


// Overlay
static PyObject* PyFont_Overlay( PyFont_Object* self, PyObject* args, PyObject* kwds )
{
	if( !self || !self->font )
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "cudaFont invalid object instance");
		return NULL;
	}

	// parse arguments
	PyObject* input  = NULL;
	PyObject* output = NULL;
	PyObject* color  = NULL;

	const char* text = NULL;

	int width  = 0;
	int height = 0;

	int x = 0;
	int y = 0;

	static char* kwlist[] = {"input", "width", "height", "text", "x", "y", "color", "output", NULL};

	if( !PyArg_ParseTupleAndKeywords(args, kwds, "Oiis|iiOO", kwlist, &input, &width, &height, &text, &x, &y, &color, &output))
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "cudaFont.Overlay() failed to parse args tuple");
		return NULL;
	}

	if( !output )
		output = input;

	// parse color tuple
	float4 rgba = make_float4(0, 0, 0, 255);

	if( color != NULL )
	{
		if( !PyTuple_Check(color) )
		{
			PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "cudaFont.Overlay() color argument isn't a valid tuple");
			return NULL;
		}

		if( !PyArg_ParseTuple(color, "fff|f", &rgba.x, &rgba.y, &rgba.z, &rgba.w) )
		{
			PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "cudaFont.Overlay() failed to parse color tuple");
			return NULL;
		}
	}

	// verify dimensions
	if( width <= 0 || height <= 0 )
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "cudaFont.Overlay() image dimensions are invalid");
		return NULL;
	}

	// get pointer to input image data
	void* input_img = PyCUDA_GetPointer(input);

	if( !input_img )
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "cudaFont.Overlay() failed to get input image pointer from PyCapsule container");
		return NULL;
	}

	// get pointer to output image data
	void* output_img = PyCUDA_GetPointer(output);

	if( !output_img )
	{
		PyErr_SetString(PyExc_Exception, LOG_PY_UTILS "cudaFont.Overlay() failed to get output image pointer from PyCapsule container");
		return NULL;
	}

	//printf("cudaFont.Overlay(%p, %p, %i, %i, '%s', %i, %i, (%f, %f, %f, %f))\n", input_img, output_img, width, height, text, x, y, rgba.x, rgba.y, rgba.z, rgba.w);

	// render the font overlay
	self->font->RenderOverlay((float4*)input_img, (float4*)output_img, width, height, text, x, y, rgba);

	// return void
	Py_RETURN_NONE;
}


//-------------------------------------------------------------------------------
static PyTypeObject pyFont_Type = 
{
    PyVarObject_HEAD_INIT(NULL, 0)
};

static PyMethodDef pyFont_Methods[] = 
{
	{ "Overlay", (PyCFunction)PyFont_Overlay, METH_VARARGS|METH_KEYWORDS, "Render the font overlay for a given text string"},
	{NULL}  /* Sentinel */
};

static PyMemberDef pyFont_Members[] = 
{
	{ "Black",   T_OBJECT_EX, offsetof(PyFont_Object, black),   0, "Black color tuple"},
	{ "White",   T_OBJECT_EX, offsetof(PyFont_Object, white),   0, "White color tuple"},
	{ "Gray",    T_OBJECT_EX, offsetof(PyFont_Object, gray),    0, "Gray color tuple"},
	{ "Brown",   T_OBJECT_EX, offsetof(PyFont_Object, brown),   0, "Brown color tuple"},
	{ "Tan",     T_OBJECT_EX, offsetof(PyFont_Object, tan),     0, "Tan color tuple"},
	{ "Red",     T_OBJECT_EX, offsetof(PyFont_Object, red),     0, "Red color tuple"},
	{ "Green",   T_OBJECT_EX, offsetof(PyFont_Object, green),   0, "Green color tuple"},
	{ "Blue",    T_OBJECT_EX, offsetof(PyFont_Object, blue),    0, "Blue color tuple"},
	{ "Cyan",    T_OBJECT_EX, offsetof(PyFont_Object, cyan),    0, "Cyan color tuple"},
	{ "Lime",    T_OBJECT_EX, offsetof(PyFont_Object, lime),    0, "Lime color tuple"},
	{ "Yellow",  T_OBJECT_EX, offsetof(PyFont_Object, yellow),  0, "Yellow color tuple"},
	{ "Orange",  T_OBJECT_EX, offsetof(PyFont_Object, orange),  0, "Orange color tuple"},
	{ "Purple",  T_OBJECT_EX, offsetof(PyFont_Object, purple),  0, "Purple color tuple"},
	{ "Magenta", T_OBJECT_EX, offsetof(PyFont_Object, magenta), 0, "Magenta color tuple"},
	{NULL}  /* Sentinel */
};

// Register types
bool PyCUDA_RegisterTypes( PyObject* module )
{
	if( !module )
		return false;
	
	pyFont_Type.tp_name 	= PY_UTILS_MODULE_NAME ".cudaFont";
	pyFont_Type.tp_basicsize = sizeof(PyFont_Object);
	pyFont_Type.tp_flags 	= Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	pyFont_Type.tp_methods   = pyFont_Methods;
	pyFont_Type.tp_members	= pyFont_Members;
	pyFont_Type.tp_new 	     = PyFont_New;
	pyFont_Type.tp_init	     = (initproc)PyFont_Init;
	pyFont_Type.tp_dealloc	= (destructor)PyFont_Dealloc;
	pyFont_Type.tp_doc  	= "Bitmap font overlay rendering with CUDA";
	 
	if( PyType_Ready(&pyFont_Type) < 0 )
	{
		printf(LOG_PY_UTILS "cudaFont PyType_Ready() failed\n");
		return false;
	}
	
	Py_INCREF(&pyFont_Type);
    
	if( PyModule_AddObject(module, "cudaFont", (PyObject*)&pyFont_Type) < 0 )
	{
		printf(LOG_PY_UTILS "cudaFont PyModule_AddObject('cudaFont') failed\n");
		return false;
	}
	return true;
}

