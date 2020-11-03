#include "python34.h"
#include "Python.h"
#include <ppltasks.h>
#include <inspectable.h>
#include <wrl.h>
#include <robuffer.h>
#include <windows.storage.streams.h>


using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Security::Cryptography;
using namespace Microsoft::WRL;
using namespace Platform;
using namespace Concurrency;

namespace python34 {
class PyBytesBuffer:
	public RuntimeClass<RuntimeClassFlags<RuntimeClassType::WinRtClassicComMix>,
						ABI::Windows::Storage::Streams::IBuffer,
						IBufferByteAccess>
{
public:
	PyObject *data;
	int length;
	virtual ~PyBytesBuffer()
    {
		Py_DECREF(data);
    }

	STDMETHODIMP RuntimeClassInitialize(PyObject *data)
	{
		Py_INCREF(data);
		this->data = data;
		length = 0;
		return S_OK;
	}

    STDMETHODIMP Buffer(byte **value)
    {
        *value = (byte*)PyBytes_AsString(data);
        return S_OK;
    }

    STDMETHODIMP get_Capacity(UINT32 *value)
    {
        *value = PyBytes_Size(data);
        return S_OK;
    }

    STDMETHODIMP get_Length(UINT32 *value)
    {
        *value = length;
        return S_OK;
    }

    STDMETHODIMP put_Length(UINT32 value)
    {
        length = value;
        return S_OK;
    }
};
}
using namespace python34;

extern "C" {

void win32_urandom(unsigned char* buffer, Py_ssize_t size, int raise)
{
    IBuffer^ data = CryptographicBuffer::GenerateRandom(size);
    Array<unsigned char>^ data2;
    CryptographicBuffer::CopyToByteArray(data, &data2);
    for(int i=0; i < size; i++)
        buffer[i] = data2[i];
}

/* Temporary wrapper for local app data store. Will be replaced with generic wrapping later.
   Only modes "r" and "w" are supported. */
typedef struct {
    PyObject_HEAD
	IInputStream^ item;
} iinputstream;


static PyObject* 
iinputstream_read(PyObject *self, PyObject* args)
{
	int size;
	if (!PyArg_ParseTuple(args, "i", &size))
		return NULL;
	
	PyObject *result = PyBytes_FromStringAndSize(NULL, size);
	if (!result)
		return NULL;
	ComPtr<PyBytesBuffer> native;
	Microsoft::WRL::Details::MakeAndInitialize<PyBytesBuffer>(&native, result);
	auto iinspectable = (IInspectable *)reinterpret_cast<IInspectable *>(native.Get());
	Streams::IBuffer ^buffer = reinterpret_cast<Streams::IBuffer ^>(iinspectable);
	create_task(((iinputstream*)self)->item->ReadAsync(buffer, size, InputStreamOptions::None)).wait();
	_PyBytes_Resize(&result, buffer->Length);
	return result;
}

static PyMethodDef iinputstream_methods[] = {
	{"read", iinputstream_read},
	{NULL}
};

static PyTypeObject istoragefile_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "istoragefile",             /* tp_name */
    sizeof(iinputstream),      /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "WinRT file",           /* tp_doc */
	0,                      /* tp_traverse */
	0,						/* tp_clear */
    0,                      /* tp_richcompare */
    0,                      /* tp_weaklistoffset */
    0,                      /* tp_iter */
    0,                      /* tp_iternext */
    0,                      /* tp_methods */
	0,	                    /* tp_members */
    0,                      /* tp_getset */
};

static PyObject*
new_istorageitem(IRandomAccessStream^ item)
{
	istoragefile_Type.tp_new = PyType_GenericNew;
	if (!PyType_Ready(&istoragefile_Type))
		return NULL;
	PyObject *result = istoragefile_Type.tp_alloc(&istoragefile_Type, 0);
	if (!result)
		return NULL;
	((iinputstream*)result)->item = item;
	return result;
}

static PyObject*
local_open(PyObject *self, PyObject* args)
{
	wchar_t *name, *mode;
	if (!PyArg_ParseTuple(args, "SS", &name, &mode))
		return NULL;
	StorageFolder^ localFolder = ApplicationData::Current->LocalFolder;
	if (mode[0] == L'r') {
		String ^sname = ref new String(name);
		StorageFile^ item = create_task(localFolder->GetFileAsync(sname)).get();
		if (item == nullptr) {
			PyErr_SetString(PyExc_OSError, "File not found");
			return NULL;
		}
		auto result = create_task(item->OpenReadAsync()).get();
		return new_istorageitem(result);
	}
}

} // extern "C"
