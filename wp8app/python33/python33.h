#pragma once

using namespace Platform;

namespace python33
{
	public ref class Object sealed{
	private:
		PyObject *obj;
		friend ref class Interpreter;
		Object(PyObject* o):obj(o){}
		~Object(){ Py_DECREF(obj);}
	public:
		property bool isNone{
			bool get(){return obj == Py_None;}
		}
	};

	public interface class GUI {
		void AddOutAsync(String^ text);
		void AddErrAsync(String^ text);
	};

    public ref class Interpreter sealed
    {
		GUI^ gui;
		PyObject *metrosetup;
    public:
        Interpreter(GUI^ callback);
		void ShutDown();
		property GUI^ Gui {
			GUI^ get() { return gui; }
		}
		python33::Object^ TryCompile(String^ code);
		void RunCode(python33::Object^ code);
		void RunScript(const Platform::Array<uint8>^ code);
    };
}