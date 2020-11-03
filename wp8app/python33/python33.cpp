#include "Python.h"
#include "python33.h"

using namespace python33;
using namespace Platform;
using namespace Windows::Storage::Streams;
//using namespace Windows::Security::Cryptography;


extern "C" void win32_urandom(unsigned char* buffer, Py_ssize_t size, int raise)
{
	/* XXX no cryptography on WP8???

	IBuffer^ data = CryptographicBuffer::GenerateRandom(size);
	Array<unsigned char>^ data2;
	CryptographicBuffer::CopyToByteArray(data, &data2);
	for(int i=0; i < size; i++)
		buffer[i] = data2[i];
	*/
	/* XXX leave data uninitialized for now */
}

static Interpreter^ singleton;

extern "C" static PyObject *
add_to_stdout(PyObject *self, PyObject *args)
{
    Py_UNICODE *data;
    if (!PyArg_ParseTuple(args, "u", &data))
        return NULL;
    singleton->Gui->AddOutAsync(ref new String(data));
    Py_RETURN_NONE;
}

extern "C" static PyObject *
add_to_stderr(PyObject *self, PyObject *args)
{
    Py_UNICODE *data;
    if (!PyArg_ParseTuple(args, "u", &data))
        return NULL;
    singleton->Gui->AddErrAsync(ref new String(data));
    Py_RETURN_NONE;
}

extern "C" static PyObject *
readline(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_IOError, "Getting input from console is not implemented yet");
    return NULL;
}

extern "C" static PyObject *
metroui_exit(PyObject *self, PyObject *args)
{
    //singleton->exit();
    Py_RETURN_NONE;
}

static struct PyMethodDef metroui_methods[] = {
    {"add_to_stdout", add_to_stdout,
     METH_VARARGS, NULL},
    {"add_to_stderr", add_to_stderr,
     METH_VARARGS, NULL},
    {"readline", readline, METH_NOARGS, NULL},
    /*{"exit", metroui_exit,
     METH_NOARGS, NULL},*/
  {NULL, NULL}
};

static struct PyModuleDef metroui = {
    PyModuleDef_HEAD_INIT,
    "metroui",
    NULL,
    -1,
    metroui_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

extern "C" static PyObject*
PyInit_metroui()
{
    return PyModule_Create(&metroui);
}
static int metroui_added = false;

static wchar_t progpath[1024];
Interpreter::Interpreter(GUI^ gui)
{
	singleton = this;
	this->gui = gui;
    /* add metroui to builtin modules */
	if (!metroui_added) {
        PyImport_AppendInittab("metroui", PyInit_metroui);
        metroui_added = true;
	}

    /* compute python path */
    Windows::ApplicationModel::Package^ package = Windows::ApplicationModel::Package::Current;
    Windows::Storage::StorageFolder^ installedLocation = package->InstalledLocation;
    wcscpy_s(progpath, installedLocation->Path->Data());
    /* XXX how to determine executable name? */
    wcscat_s(progpath, L"\\python33app.exe");
    Py_SetProgramName(progpath);
    Py_Initialize();
    PyEval_InitThreads();
    
    /* boot interactive shell */
    metrosetup = PyImport_ImportModule("metrosetup");
    if (metrosetup == NULL) {
        PyErr_Print();
    }

    PyEval_ReleaseThread(PyThreadState_Get());
}

void Interpreter::ShutDown()
{
	PyGILState_STATE s = PyGILState_Ensure();
	Py_Finalize();
}

python33::Object^ Interpreter::TryCompile(String ^command)
{
	python33::Object^ result;
	PyGILState_STATE s = PyGILState_Ensure();
	PyObject *code = PyObject_CallMethod(metrosetup, "compile", "u", command->Data());
	if (code == NULL) {
		PyErr_Print();
		result = nullptr;
	}
	else
		result = ref new python33::Object(code);
	PyGILState_Release(s);
	return result;
}

void Interpreter::RunCode(python33::Object^ command)
{
	PyGILState_STATE s = PyGILState_Ensure();
	PyObject *result = PyObject_CallMethod(metrosetup, "eval", "O", command->obj);
	if (result == NULL) {
		PyErr_Print();
	}
	else
		Py_DECREF(result);
	PyGILState_Release(s);
}

void Interpreter::RunScript(const Platform::Array<uint8>^ script)
{
	if (script[script->Length-1] != '\0')
	{
		throw ref new Platform::InvalidArgumentException("code not null-terminated");
	}
	PyGILState_STATE s = PyGILState_Ensure();
	PyRun_SimpleString((const char*)script->Data);
	PyGILState_Release(s);
}
