#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include <Winstring.h>
/* Not available on Windows phone 8??? */
#include <Rometadataresolution.h>
#include <rometadata.h>
#include <RoMetadataApi.h>

static PyObject* IMetaDataImport2_New(IMetaDataImport2 *com);

static PyObject *
comerror(HRESULT hr)
{
    /* XXX IErrorInfo */
    return PyErr_SetExcFromWindowsErr(PyExc_WindowsError, hr);
}

static PyObject*
consume_hstring(HSTRING *s)
{
    const wchar_t *data;
    UINT32 len;
    PyObject *result;

    data = WindowsGetStringRawBuffer(*s, &len);
    result = PyUnicode_FromUnicode(data, len);
    WindowsDeleteString(*s);
    *s = NULL;
    return result;
}

struct winrt_wrapper{
    PyObject_HEAD
    void *_com;
};

#define IMetaDataImport2_ptr(o) ((IMetaDataImport2*)(o -> _com))

static PyObject*
IMetaDataImport2_EnumCustomAttributes(PyObject *_self, PyObject *args)
{
    winrt_wrapper *self = (winrt_wrapper*)_self;
    PyObject *oenum, *oType;
    long iMax;
    HCORENUM henum = NULL;
    mdToken tk, tkType;
    mdCustomAttribute *rCustomAttributes;
    ULONG cTypeDefs, i;
    HRESULT hr;
    PyObject *result;
    if (!PyArg_ParseTuple(args, "OkOl:EnumCustomAttributes", &oenum, &tk, &oType, &iMax))
        return NULL;
    if (oType == Py_None) {
        tkType = 0;
    }
    else {
        PyErr_SetString(PyExc_NotImplementedError, "tkType");
        return NULL;
    }
    rCustomAttributes = (mdTypeDef*)malloc(iMax*sizeof(mdCustomAttribute));
    /* XXX oenum: in/out */
		
    hr = IMetaDataImport2_ptr(self)->EnumCustomAttributes(&henum, tk, tkType, rCustomAttributes, iMax, &cTypeDefs);
    if (!SUCCEEDED(hr)) {
        free(rCustomAttributes);
        return comerror(hr);
    }
    result = PyList_New(cTypeDefs);
    if (!result) {
        free(rCustomAttributes);
        return NULL;
    }
    for (i = 0; i < cTypeDefs; i++) {
        PyObject *t = PyLong_FromUnsignedLong(rCustomAttributes[i]);
        if (!t) {
            free(rCustomAttributes);
            Py_DECREF(result);
            return NULL;
        }
        PyList_SET_ITEM(result, i, t);
    }
    return result;
}

static PyObject*
IMetaDataImport2_EnumMethods(PyObject *_self, PyObject *args)
{
    winrt_wrapper *self = (winrt_wrapper*)_self;
    PyObject *oenum;
    mdTypeDef cl;
    long iMax;
    HCORENUM henum = NULL;
    mdMethodDef *rMethods;
    ULONG cTokens, i;
    HRESULT hr;
    PyObject *result;
    if (!PyArg_ParseTuple(args, "Okl:EnumMethods", &oenum, &cl, &iMax))
        return NULL;
    rMethods = (mdMethodDef*)malloc(iMax*sizeof(mdMethodDef));
    /* XXX oenum: in/out */
    hr = IMetaDataImport2_ptr(self)->EnumMethods(&henum, cl, rMethods, iMax, &cTokens);
    if (!SUCCEEDED(hr)) {
        free(rMethods);
        return comerror(hr);
    }
    result = PyList_New(cTokens);
    if (!result) {
        free(rMethods);
        return NULL;
    }
    for (i = 0; i < cTokens; i++) {
        PyObject *t = PyLong_FromUnsignedLong(rMethods[i]);
        if (!t) {
            free(rMethods);
            Py_DECREF(result);
            return NULL;
        }
        PyList_SET_ITEM(result, i, t);
    }
    return result;
}

static PyObject*
IMetaDataImport2_EnumInterfaceImpls(PyObject *_self, PyObject *args)
{
    winrt_wrapper *self = (winrt_wrapper*)_self;
    PyObject *oenum;
    long iMax;
    HCORENUM henum = NULL;
    mdTypeDef td, *rTypeDefs;
    ULONG cTypeDefs, i;
    HRESULT hr;
    PyObject *result;
    if (!PyArg_ParseTuple(args, "Okl:EnumTypeDefs", &oenum, &td, &iMax))
        return NULL;
    rTypeDefs = (mdTypeDef*)malloc(iMax*sizeof(mdTypeDef));
    /* XXX oenum: in/out */
    hr = IMetaDataImport2_ptr(self)->EnumInterfaceImpls(&henum, td, rTypeDefs, iMax, &cTypeDefs);
    if (!SUCCEEDED(hr)) {
        free(rTypeDefs);
        return comerror(hr);
    }
    result = PyList_New(cTypeDefs);
    if (!result) {
        free(rTypeDefs);
        return NULL;
    }
    for (i = 0; i < cTypeDefs; i++) {
        PyObject *t = PyLong_FromUnsignedLong(rTypeDefs[i]);
        if (!t) {
            free(rTypeDefs);
            Py_DECREF(result);
            return NULL;
        }
        PyList_SET_ITEM(result, i, t);
    }
    return result;
}

static PyObject*
IMetaDataImport2_EnumTypeDefs(PyObject *_self, PyObject *args)
{
    winrt_wrapper *self = (winrt_wrapper*)_self;
    PyObject *oenum;
    long iMax;
    HCORENUM henum = NULL;
    mdTypeDef *rTypeDefs;
    ULONG cTypeDefs, i;
    HRESULT hr;
    PyObject *result;
    if (!PyArg_ParseTuple(args, "Ol:EnumTypeDefs", &oenum, &iMax))
        return NULL;
    rTypeDefs = (mdTypeDef*)malloc(iMax*sizeof(mdTypeDef));
    /* XXX oenum: in/out */
    hr = IMetaDataImport2_ptr(self)->EnumTypeDefs(&henum,rTypeDefs, iMax, &cTypeDefs);
    if (!SUCCEEDED(hr)) {
        free(rTypeDefs);
        return comerror(hr);
    }
    result = PyList_New(cTypeDefs);
    if (!result) {
        free(rTypeDefs);
        return NULL;
    }
    for (i = 0; i < cTypeDefs; i++) {
        PyObject *t = PyLong_FromUnsignedLong(rTypeDefs[i]);
        if (!t) {
            free(rTypeDefs);
            Py_DECREF(result);
            return NULL;
        }
        PyList_SET_ITEM(result, i, t);
    }
    return result;
}

static PyObject*
IMetaDataImport2_FindTypeDefByName(PyObject *_self, PyObject *args)
{
    winrt_wrapper *self = (winrt_wrapper*)_self;
    LPWSTR name;
    Py_ssize_t namelen;
    mdToken tkEnclosingClass;
    mdTypeDef td;
    HRESULT hr;
    if (!PyArg_ParseTuple(args, "u#k:FindTypeDefByName", &name, &namelen, &tkEnclosingClass))
        return NULL;
    hr = IMetaDataImport2_ptr(self)->FindTypeDefByName(name, tkEnclosingClass, &td);
    if (!SUCCEEDED(hr)) {
        return comerror(hr);
    }
    return PyLong_FromUnsignedLong(td);
}

static PyObject*
IMetaDataImport2_GetCustomAttributeProps(PyObject *_self, PyObject *args)
{
    winrt_wrapper *self = (winrt_wrapper*)_self;
    mdCustomAttribute cv;
    mdToken tkObj, tkType;
    const void *pBlob;
    ULONG cbSize;
    HRESULT hr;

    if (!PyArg_ParseTuple(args, "k:GetCustomAttributeProps", &cv))
        return NULL;
    /* Determine buffer size */
    hr = IMetaDataImport2_ptr(self)->GetCustomAttributeProps(cv, &tkObj, &tkType, &pBlob, &cbSize);
    if (!SUCCEEDED(hr)) {
        return comerror(hr);
    }
    return Py_BuildValue("kky#", tkObj, tkType, pBlob, cbSize);
}

static PyObject*
IMetaDataImport2_GetInterfaceImplProps(PyObject *_self, PyObject *args)
{
    winrt_wrapper *self = (winrt_wrapper*)_self;
	mdInterfaceImpl iiImpl;
	mdTypeDef Class;
	mdToken tkIface;
	HRESULT hr;

    if (!PyArg_ParseTuple(args, "k:GetTypeDefProps", &iiImpl))
        return NULL;
    /* Determine buffer size */
    hr = IMetaDataImport2_ptr(self)->GetInterfaceImplProps(iiImpl, &Class, &tkIface);
    if (!SUCCEEDED(hr)) {
        return comerror(hr);
    }
    return Py_BuildValue("kk", Class, tkIface);
}

static PyObject*
IMetaDataImport2_GetMemberRefProps(PyObject *_self, PyObject *args)
{
    winrt_wrapper *self = (winrt_wrapper*)_self;
    mdMemberRef mr;
    mdToken tk;
    PyObject *name;
    ULONG name_len;
    PCCOR_SIGNATURE pvSigBlob;
    ULONG bSig;
    HRESULT hr;

    if (!PyArg_ParseTuple(args, "k:GetMemberRefProps", &mr))
        return NULL;
    /* Determine buffer size */
    hr = IMetaDataImport2_ptr(self)->GetMemberRefProps(mr, NULL, NULL, 0, &name_len, 
        NULL, NULL);
    if (!SUCCEEDED(hr)) {
        return comerror(hr);
    }
    name = PyUnicode_FromUnicode(NULL, name_len-1); /* name_len includes terminating \0 */
    if (name == NULL)
        return NULL;
    hr = IMetaDataImport2_ptr(self)->GetMemberRefProps(mr, &tk, PyUnicode_AS_UNICODE(name), 
        name_len, &name_len, &pvSigBlob, &bSig);
    if (!SUCCEEDED(hr)) {
        Py_DECREF(name);
        return comerror(hr);
    }
    return Py_BuildValue("kNy#", tk, name, pvSigBlob, bSig);
}

static PyObject*
IMetaDataImport2_GetMethodProps(PyObject *_self, PyObject *args)
{
    winrt_wrapper *self = (winrt_wrapper*)_self;
    mdMethodDef md_method;
    mdTypeDef md_class;
    PyObject *name;
    ULONG name_len;
    DWORD attr;
    PCCOR_SIGNATURE sig;
    ULONG sig_size;
    ULONG rva;
    DWORD impl_flags;
    HRESULT hr;

    if (!PyArg_ParseTuple(args, "k:GetMethodProps", &md_method))
        return NULL;
    /* Determine buffer size */
    hr = IMetaDataImport2_ptr(self)->GetMethodProps(md_method, NULL, NULL, 0, &name_len, 
        NULL, NULL, NULL, NULL, NULL);
    if (!SUCCEEDED(hr)) {
        return comerror(hr);
    }
    name = PyUnicode_FromUnicode(NULL, name_len-1); /* name_len includes terminating \0 */
    if (name == NULL)
        return NULL;
    hr = IMetaDataImport2_ptr(self)->GetMethodProps(md_method, &md_class, PyUnicode_AS_UNICODE(name), 
        name_len, &name_len, &attr, &sig, &sig_size, &rva, &impl_flags);
    if (!SUCCEEDED(hr)) {
        Py_DECREF(name);
        return comerror(hr);
    }
    return Py_BuildValue("kNky#k", md_class, name, attr, sig, sig_size, impl_flags);
}

static PyObject*
IMetaDataImport2_GetTypeRefProps(PyObject *_self, PyObject *args)
{
    winrt_wrapper *self = (winrt_wrapper*)_self;
    mdTypeRef tr;
    mdToken scope;
    PyObject *name;
    ULONG namelen;
    HRESULT hr;

    if (!PyArg_ParseTuple(args, "k:GetTypeRefProps", &tr))
        return NULL;
    /* Determine buffer size */
    hr = IMetaDataImport2_ptr(self)->GetTypeRefProps(tr, NULL, NULL, 0, &namelen);
    if (!SUCCEEDED(hr)) {
        return comerror(hr);
    }
	/* Undocumented error code ??? */
	if (hr == S_FALSE) {
        PyErr_SetString(PyExc_ValueError, "not a typeref");
		return NULL;
    }
    name = PyUnicode_FromUnicode(NULL, namelen-1); /* name_len includes terminating \0 */
    if (name == NULL)
        return NULL;
    hr = IMetaDataImport2_ptr(self)->GetTypeRefProps(tr, &scope, PyUnicode_AS_UNICODE(name), namelen, &namelen);
    if (!SUCCEEDED(hr)) {
        Py_DECREF(name);
        return comerror(hr);
    }
    return Py_BuildValue("kN", scope, name);
}

static PyObject*
IMetaDataImport2_GetTypeDefProps(PyObject *_self, PyObject *args)
{
    winrt_wrapper *self = (winrt_wrapper*)_self;
    mdTypeDef td;
    PyObject *name;
    ULONG namelen;
    HRESULT hr;
    DWORD flags;
    mdToken base;

    if (!PyArg_ParseTuple(args, "k:GetTypeDefProps", &td))
        return NULL;
    /* Determine buffer size */
    hr = IMetaDataImport2_ptr(self)->GetTypeDefProps(td, NULL, 0, &namelen, &flags, &base);
    if (!SUCCEEDED(hr)) {
        return comerror(hr);
    }
    name = PyUnicode_FromUnicode(NULL, namelen-1); /* name_len includes terminating \0 */
    if (name == NULL)
        return NULL;
    hr = IMetaDataImport2_ptr(self)->GetTypeDefProps(td, PyUnicode_AS_UNICODE(name), namelen, &namelen, &flags, &base);
    if (!SUCCEEDED(hr)) {
        Py_DECREF(name);
        return comerror(hr);
    }
    return Py_BuildValue("Nkk", name, flags, base);
}

static PyObject*
IMetaDataImport2_GetTypeSpecFromToken(PyObject *_self, PyObject *args)
{
    winrt_wrapper *self = (winrt_wrapper*)_self;
    mdTypeSpec typespec;
    HRESULT hr;
	PCCOR_SIGNATURE pvSig;
	ULONG cbSig;

    if (!PyArg_ParseTuple(args, "k:GetTypeSpecFromToken", &typespec))
        return NULL;
    /* Determine buffer size */
    hr = IMetaDataImport2_ptr(self)->GetTypeSpecFromToken(typespec, &pvSig, &cbSig);
    if (FAILED(hr)) {
        return comerror(hr);
    }
	return Py_BuildValue("y#", pvSig, cbSig);
}

static PyObject*
IMetaDataImport2_ResolveTypeRef(PyObject *_self, PyObject *args)
{
    winrt_wrapper *self = (winrt_wrapper*)_self;
    mdTypeRef tr;
    IUnknown *plScope;
    mdTypeDef td;
    HRESULT hr;

    if (!PyArg_ParseTuple(args, "k:GetTypeSpecFromToken", &tr))
        return NULL;
    /* Determine buffer size */
    hr = IMetaDataImport2_ptr(self)->ResolveTypeRef(tr, IID_IMetaDataImport, &plScope, &td);
    if (FAILED(hr)) {
        return comerror(hr);
    }
    return Py_BuildValue("Nk", IMetaDataImport2_New((IMetaDataImport2*)plScope), td);
}

static PyMethodDef IMetaDataImport2_methods[] = {
    {"EnumCustomAttributes", IMetaDataImport2_EnumCustomAttributes, METH_VARARGS, 0},
    {"EnumMethods", IMetaDataImport2_EnumMethods, METH_VARARGS, 0},
    {"EnumTypeDefs", IMetaDataImport2_EnumTypeDefs, METH_VARARGS, 0},
    {"EnumInterfaceImpls", IMetaDataImport2_EnumInterfaceImpls, METH_VARARGS, 0},
    {"FindTypeDefByName", IMetaDataImport2_FindTypeDefByName, METH_VARARGS, 0},
    {"GetCustomAttributeProps", IMetaDataImport2_GetCustomAttributeProps, METH_VARARGS, 0},
    {"GetInterfaceImplProps", IMetaDataImport2_GetInterfaceImplProps, METH_VARARGS, 0},
    {"GetMemberRefProps", IMetaDataImport2_GetMemberRefProps, METH_VARARGS, 0},
    {"GetMethodProps", IMetaDataImport2_GetMethodProps, METH_VARARGS, 0},
    {"GetTypeRefProps", IMetaDataImport2_GetTypeRefProps, METH_VARARGS, 0},
    {"GetTypeDefProps", IMetaDataImport2_GetTypeDefProps, METH_VARARGS, 0},
    {"GetTypeSpecFromToken", IMetaDataImport2_GetTypeSpecFromToken, METH_VARARGS, 0},
    {"ResolveTypeRef", IMetaDataImport2_ResolveTypeRef, METH_VARARGS, 0},
    {0}
};

static void
IMetaDataImport2_del(PyObject *o)
{
    winrt_wrapper *self = (winrt_wrapper*)o;
    IMetaDataImport2_ptr(self)->Release();
    PyObject_Del(o);
}

static PyTypeObject IMetaDataImport2_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "IMetaDataImport2",
    sizeof(winrt_wrapper),
    0,
    IMetaDataImport2_del,                  /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                       /* tp_repr */
    0,                                          /* tp_as_number*/
    0,                         /* tp_as_sequence*/
    0,                          /* tp_as_mapping*/
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                           /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    0,                              /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,         /* tp_weaklistoffset */
    0,                    /* tp_iter */
    0,                                          /* tp_iternext */
    IMetaDataImport2_methods,                              /* tp_methods */
    0,                                          /* tp_members */
    0,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    0,                                  /* tp_new */
    PyObject_Del,                               /* tp_free */
};

static PyObject* 
IMetaDataImport2_New(IMetaDataImport2 *com)
{
    winrt_wrapper *res = PyObject_New(winrt_wrapper, &IMetaDataImport2_Type);
    res->_com = com;
    return (PyObject*)res;
}

static PyObject*
getmetadatafile(PyObject* unused, PyObject *args)
{
    Py_UNICODE *name;
    Py_ssize_t namelen;
    PyObject *dispenser;
    HRESULT hr;
    HSTRING hname, hpath;
    IMetaDataImport2 *mdi;
    mdTypeDef mdt;

    PyObject *opath = NULL;
    PyObject *omdi = NULL;

    if (!PyArg_ParseTuple(args, "u#|O:RoGetMetaDataFile", &name, &namelen, &dispenser))
        return NULL;
    hname = nullptr;
    hr = WindowsCreateString(name, namelen, &hname);
    if (!SUCCEEDED(hr)) {
        return comerror(hr);
    }
    /* XXX dispenser is unused */
    hpath = nullptr;
    mdi = nullptr;
    hr = RoGetMetaDataFile(hname, nullptr, &hpath, &mdi, &mdt);
    if (!SUCCEEDED(hr)) {
        comerror(hr);
        goto error;
    }
    hr = WindowsDeleteString(hname);
    hname = NULL;
    if (!SUCCEEDED(hr)) {
        comerror(hr);
        goto error;
    }
    opath = consume_hstring(&hpath);
    if (!opath)
        goto error;
    omdi = IMetaDataImport2_New(mdi);
    if (!omdi)
        goto error;
    return Py_BuildValue("NNk", opath, omdi, mdt);
error:
    if (hpath != nullptr) WindowsDeleteString(hpath);
    if (hname != nullptr) WindowsDeleteString(hname);
    Py_XDECREF(opath);
    Py_XDECREF(omdi);
    return NULL;
}

static PyObject*
resolvenamespace(PyObject *self, PyObject *args)
{
    Py_UNICODE *name;
    Py_UNICODE *mdDir = NULL;
    Py_ssize_t namelen, mdDirLen;
    HSTRING hname, hmdDir;
    HRESULT hr;
    DWORD cRetrievedSubNamespaces = 0;
    HSTRING *phstrRetrievedSubNamespaces = nullptr;
    DWORD cRetrievedMetaDataFilePaths = 0;
    HSTRING *phstrRetrievedMetaDataFiles = nullptr;
    DWORD i;

    PyObject *oPackageGraphDirs = NULL;
    PyObject *mdFilePaths = NULL;
    PyObject *subnamespaces = NULL;

    if (!PyArg_ParseTuple(args, "u#|u#O:RoResolveNamespace", &name, &namelen, &mdDir, &mdDirLen, &oPackageGraphDirs))
        return NULL;
    hname = nullptr;
    hr = WindowsCreateString(name, namelen, &hname);
    if (!SUCCEEDED(hr)) {
        return comerror(hr);
    }
    hmdDir = nullptr;
    if (mdDir) {
        hr = WindowsCreateString(mdDir, mdDirLen, &hmdDir);
        if (!SUCCEEDED(hr)) {
            WindowsDeleteString(hname);
            return comerror(hr);
        }
    }
    /* XXX packagedirs is unused */
    hr = RoResolveNamespace(hname, hmdDir, 0, nullptr,
        &cRetrievedMetaDataFilePaths, &phstrRetrievedMetaDataFiles,
        &cRetrievedSubNamespaces, &phstrRetrievedSubNamespaces);
    if (!SUCCEEDED(hr)) {
        comerror(hr);
        goto error;
    }
    hr = WindowsDeleteString(hname);
    hname = nullptr;
    if (!SUCCEEDED(hr)) {
        comerror(hr);
        goto error;
    }
    if (hmdDir) {
        hr = WindowsDeleteString(hmdDir);
        hmdDir = nullptr;
        if (!SUCCEEDED(hr)) {
            comerror(hr);
            goto error;
        }
    }

    mdFilePaths = PyList_New(cRetrievedMetaDataFilePaths);
    if (!mdFilePaths)
            goto error;
    for (i = 0; i < cRetrievedMetaDataFilePaths; i++) {
        PyObject *s = consume_hstring(phstrRetrievedMetaDataFiles+i);
        if (!s)
            goto error;
        PyList_SET_ITEM(mdFilePaths, i, s);
    }
    CoTaskMemFree(phstrRetrievedMetaDataFiles);
    phstrRetrievedMetaDataFiles = nullptr;

    subnamespaces = PyList_New(cRetrievedSubNamespaces);
    if (!subnamespaces)
            goto error;
    for (i = 0; i < cRetrievedSubNamespaces; i++) {
        PyObject *s = consume_hstring(phstrRetrievedSubNamespaces+i);
        if (!s)
            goto error;
        PyList_SET_ITEM(subnamespaces, i, s);
    }
    CoTaskMemFree(phstrRetrievedSubNamespaces);
    phstrRetrievedSubNamespaces = nullptr;

    return Py_BuildValue("NN", mdFilePaths, subnamespaces);
error:
    if (hmdDir != nullptr) WindowsDeleteString(hmdDir);
    if (hname != nullptr) WindowsDeleteString(hname);
    Py_XDECREF(mdFilePaths);
    Py_XDECREF(subnamespaces);
    if (phstrRetrievedMetaDataFiles)
        CoTaskMemFree(phstrRetrievedMetaDataFiles);
    if (phstrRetrievedSubNamespaces)
        CoTaskMemFree(phstrRetrievedSubNamespaces);
    return NULL;
}

#define IMetaDataDispenser_ptr(o) ((IMetaDataDispenser*)(((winrt_wrapper*)o) -> _com))
static PyObject*
IMetaDataDispenser_OpenScope(PyObject *_self, PyObject *args)
{
    winrt_wrapper *self = (winrt_wrapper*)_self;
    Py_UNICODE *path;
    Py_ssize_t pathlen;
    HRESULT hr;
    IMetaDataImport2 *mdi;

    if (!PyArg_ParseTuple(args, "u#:OpenScope", &path, &pathlen))
        return NULL;

    mdi = nullptr;
    hr = IMetaDataDispenser_ptr(_self)->OpenScope(path, ofRead, IID_IMetaDataImport2, (IUnknown**)&mdi);
    if (FAILED(hr))
        return comerror(hr);

    return IMetaDataImport2_New(mdi);
}

static PyMethodDef IMetaDataDispenser_methods[] = {
    {"OpenScope",          IMetaDataDispenser_OpenScope,      METH_VARARGS,
     0},
    
    {0}
};

static void
IMetaDataDispenser_del(PyObject *o)
{
    winrt_wrapper *self = (winrt_wrapper*)o;
    IMetaDataDispenser_ptr(self)->Release();
    PyObject_Del(o);
}

static PyTypeObject IMetaDataDispenser_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "IMetaDataDispenser",
    sizeof(winrt_wrapper),
    0,
    IMetaDataDispenser_del,                  /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                       /* tp_repr */
    0,                                          /* tp_as_number*/
    0,                         /* tp_as_sequence*/
    0,                          /* tp_as_mapping*/
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                           /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    0,                              /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,         /* tp_weaklistoffset */
    0,                    /* tp_iter */
    0,                                          /* tp_iternext */
    IMetaDataDispenser_methods,                              /* tp_methods */
    0,                                          /* tp_members */
    0,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    0,                                  /* tp_new */
    PyObject_Del,                               /* tp_free */
};

extern "C" static PyObject *
newdispenser(PyObject *self, PyObject *args)
{
    HRESULT hr;
    IMetaDataDispenser *pDispenser;
    hr = MetaDataGetDispenser(CLSID_CorMetaDataDispenser, IID_IMetaDataDispenser, 
                                (LPVOID*)&pDispenser);
    if (!SUCCEEDED(hr))
        return comerror(hr);

    winrt_wrapper *res = PyObject_New(winrt_wrapper, &IMetaDataDispenser_Type);
    if (res == NULL) {
        pDispenser->Release();
        return NULL;
    }
    res->_com = pDispenser;
    return (PyObject*)res;

    /*
    hr = ::CoCreateInstance(CLSID_CorMetaDataDispenser,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IMetaDataDispenser,
                           (void**) &pDispenser);
    */
}

static struct PyMethodDef winrt_methods[] = {
    { "RoGetMetaDataFile", getmetadatafile,
        METH_VARARGS},
    { "RoResolveNamespace", resolvenamespace,
        METH_VARARGS},
    { "newdispenser", newdispenser,
        METH_NOARGS},
    { NULL, NULL }
};

static struct PyModuleDef winrtmodule = {
    PyModuleDef_HEAD_INIT,
    "winrt",
    NULL,
    -1,
    winrt_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

extern "C"
PyMODINIT_FUNC
PyInit_winrt(void)
{
    PyObject *mod;

    mod = PyModule_Create(&winrtmodule);
    if (mod == NULL)
        return NULL;
    if (PyType_Ready(&IMetaDataImport2_Type) < 0)
        return NULL;
    if (PyType_Ready(&IMetaDataDispenser_Type) < 0)
        return NULL;
    return mod;
}