#include "Python.h"

typedef struct RTObject {
    PyObject_HEAD
    void *_com;
} RTObject;

#define WINAPI_FAMILY WINAPI_FAMILY_APP
