import winrt, struct, binascii

ELEMENT_TYPE_VOID = 1
ELEMENT_TYPE_BOOLEAN = 2
ELEMENT_TYPE_CHAR = 3
ELEMENT_TYPE_I1 = 4
ELEMENT_TYPE_U1 = 5
ELEMENT_TYPE_I2 = 6
ELEMENT_TYPE_U2 = 7
ELEMENT_TYPE_I4 = 8
ELEMENT_TYPE_U4 = 9
ELEMENT_TYPE_I8 = 0xa
ELEMENT_TYPE_U8 = 0xb
ELEMENT_TYPE_R4 = 0xc
ELEMENT_TYPE_R8 = 0xd
ELEMENT_TYPE_STRING = 0xe
ELEMENT_TYPE_PTR = 0xf
ELEMENT_TYPE_BYREF = 0x10
ELEMENT_TYPE_VALUETYPE = 0x11
ELEMENT_TYPE_CLASS = 0x12
ELEMENT_TYPE_VAR = 0x13
ELEMENT_TYPE_GENERICINST = 0x15
ELEMENT_TYPE_TYPEDBYREF = 0x16
ELEMENT_TYPE_I = 0x18
ELEMENT_TYPE_OBJECT = 0x1c
ELEMENT_TYPE_SZARRAY = 0x1d
ELEMENT_TYPE_CMOD_REQD = 0x1f
ELEMENT_TYPE_CMOD_OPT = 0x20

tnames = {
    ELEMENT_TYPE_VOID: 'void',
    ELEMENT_TYPE_BOOLEAN: 'boolean',
    ELEMENT_TYPE_CHAR: 'wchar_t',
    ELEMENT_TYPE_I1: 'char',
    ELEMENT_TYPE_U1: 'unsigned char',
    ELEMENT_TYPE_I2: 'short',
    ELEMENT_TYPE_U2: 'unsigned short',
    ELEMENT_TYPE_I4: 'int',
    ELEMENT_TYPE_U4: 'unsigned int',
    ELEMENT_TYPE_I8: 'INT64',
    ELEMENT_TYPE_U8: 'UINT64',
    ELEMENT_TYPE_R4: 'float',
    ELEMENT_TYPE_R8: 'double',
    ELEMENT_TYPE_STRING: 'HSTRING',
    ELEMENT_TYPE_I: 'int',
}


mdtModule                       = 0x00000000
mdtTypeRef                      = 0x01000000
mdtTypeDef                      = 0x02000000
mdtFieldDef                     = 0x04000000
mdtMethodDef                    = 0x06000000
mdtParamDef                     = 0x08000000
mdtInterfaceImpl                = 0x09000000
mdtMemberRef                    = 0x0a000000
mdtCustomAttribute              = 0x0c000000
mdtPermission                   = 0x0e000000
mdtSignature                    = 0x11000000
mdtEvent                        = 0x14000000
mdtProperty                     = 0x17000000
mdtModuleRef                    = 0x1a000000
mdtTypeSpec                     = 0x1b000000
mdtAssembly                     = 0x20000000
mdtAssemblyRef                  = 0x23000000
mdtFile                         = 0x26000000
mdtExportedType                 = 0x27000000
mdtManifestResource             = 0x28000000
mdtGenericParam                 = 0x2a000000
mdtMethodSpec                   = 0x2b000000
mdtGenericParamConstraint       = 0x2c000000
mdtString                       = 0x70000000
mdtName                         = 0x71000000
mdtBaseType                     = 0x72000000

def token_type(t):
    return t & 0xff000000

def decompress_integer(s):
    b0 = s[0]
    if (b0 & 0x80) == 0:
        return b0, s[1:]
    if (b0 & 0x40) == 0:
        return ((b0 & 0x3f) << 8) + s[1], s[2:]
    return (((b0 & 0x3f) << 24) + (s[1]<<16) +
            (s[2] << 8) + s[3]), s[4:]

def decompress_packed_string(s):
    # http://www.codeproject.com/Articles/42655/NET-file-format-Signatures-under-the-hood-Part-2
    if s[0] == 0:
        return None, s[1:]
    if s[0] == 0xff:
        return "", s[1:]
    len, s = decompress_integer(s)
    res = s[:len].decode('utf-8')
    s = s[len:]
    return res, s

struct_map = {
    ELEMENT_TYPE_U1:'B',
    ELEMENT_TYPE_U2:'<H',
    ELEMENT_TYPE_U4:'<I',
    ELEMENT_TYPE_U8:'<Q',
}

def decode_value(value, typecode):
    code = struct_map[typecode]
    s = struct.calcsize(code)
    v = value[:s]
    value = value[s:]
    return struct.unpack(code, v)[0], value

def decode_custom_attribute(sig, value):
    # Prolog
    v, value = decode_value(value, ELEMENT_TYPE_U2)
    assert v == 1
    # StandAloneMethodSig
    s0 = sig[0]
    sig = sig[1:]
    nparams, sig = decompress_integer(sig)
    rtype, sig = decode_type(sig)
    assert rtype.is_void()
    result = []
    for i in range(nparams):
        ptype, sig = decode_type(sig)
        if ptype.is_basic():
            if ptype.kind == ELEMENT_TYPE_STRING:
                v, value = decompress_packed_string(value)
            else:
                v, value = decode_value(value, ptype.kind)
            result.append(v)
        elif ptype.name == 'System.Type':
            tname, value = decompress_packed_string(value)
            result.append(tname)
        elif ptype.is_enum():
            v, value = decode_value(value, ELEMENT_TYPE_U4)
            result.append(v)
        else:
            assert False, (ptype.name, value)
    numnamed, value = decode_value(value, ELEMENT_TYPE_U2)
    if numnamed:
        result.append(("named", numnamed, value))
    return result

class RTType:
    def is_void(self):
        return False

    def is_array(self):
        return False

    def is_basic(self):
        return False

class BasicType(RTType):
    def __init__(self, kind):
        self.kind = kind

    def decl_var(self, var):
        return tnames[self.kind] + " " + var + ";"

    def decl_ptr(self, var):
        return tnames[self.kind] + " *" + var + ";"

    def is_void(self):
        return self.kind == ELEMENT_TYPE_VOID

    def is_basic(self):
        return True

builtin_types = {
    'IUnknown':'IUnknown',
    'Windows.Foundation.EventRegistrationToken':'EventRegistrationToken',
    'Windows.Foundation.AsyncStatus':'AsyncStatus',
    'Windows.Foundation.IAsyncInfo':'IAsyncInfo',
    'Windows.Foundation.HResult':'HRESULT',
    'Windows.Foundation.Uri':'__x_ABI_CWindows_CFoundation_CIUriRuntimeClass',
    'System.Guid':'GUID',
    }

unsupported_types = {
    # no C type
    'Windows.Foundation.WwwFormUrlDecoder',
    'Windows.Graphics.Printing.PrintTaskOptions',
    'Windows.Media.MediaProperties.MediaPropertySet',
    'Windows.Networking.Sockets.IWebSocket',
    'Windows.Networking.Sockets.MessageWebSocketInformation',
    'Windows.Networking.Sockets.StreamWebSocketInformation',
    'Windows.Networking.Connectivity.IPInformation',
    'Windows.Security.Authentication.OnlineId.SignOutUserOperation',
    'Windows.Security.Cryptography.Core.CryptographicHash',
    'Windows.Storage.Streams.DataReaderLoadOperation',
    'Windows.Storage.Streams.DataWriterStoreOperation',
    'Windows.Storage.AccessCache.AccessListEntryView',
    'Windows.Storage.FileProperties.StorageItemThumbnail',
    'Windows.UI.ApplicationSettings.SettingsCommand',
    'Windows.UI.Xaml.Media.Animation.TimelineCollection',
    'Windows.UI.Xaml.Media.Animation.DoubleKeyFrameCollection',
    }

class NamedType(RTType):
    def __init__(self, name, token, isclass):
        if name in unsupported_types:
            raise NotImplementedError
        self.name = name
        self.token = token
        self.isclass = isclass
        self.ptr = ''
        if self.isclass:
            self.ptr = '*'
        try:
            self.tname = builtin_types[name]
        except KeyError:
            comp = name.split('.')
            # XXX tell interfaces apart
            if isclass and not (comp[-1][0] == 'I' and comp[-1][1].isupper()):
                comp[-1] = 'I'+comp[-1]
            self.tname = '::'.join(comp)

    def decl_var(self, var):
        return "%s %s%s;" % (self.tname, self.ptr, var)

    def decl_ptr(self, var):
        return "%s %s*%s;" % (self.tname, self.ptr, var)

    def is_enum(self):
        mdi, td = type_by_name[self.name]
        tname, flags, base = mdi.GetTypeDefProps(td)
        scope, basename = mdi.GetTypeRefProps(base)
        return basename == 'System.Enum'

class SZArray(RTType):
    def __init__(self, elemtype):
        self.elemtype = elemtype

    def decl_var(self, var):
        return self.elemtype.decl_ptr(var)

    def is_array(self):
        return True

def decode_type(s):
    b0 = s[0]
    s = s[1:]
    if b0 in tnames:
        return BasicType(b0), s
    if b0 in (ELEMENT_TYPE_CLASS, ELEMENT_TYPE_VALUETYPE):
        i, s = decompress_integer(s)
        table = i & 0x3
        value = i >> 2
        token = (table<<24) + value
        scope, name = mdi.GetTypeRefProps(token)
        return NamedType(name, token, b0 == ELEMENT_TYPE_CLASS), s
    if b0 == ELEMENT_TYPE_GENERICINST:
        raise NotImplementedError
        s = decode_type(s)
        argc, s = decompress_integer(s)
        print('<', end='')
        for i in range(argc):
            s = decode_type(s)
            if i < argc-1:
                print(',', end=' ')
        print('>', end=' ')
        return s
    if b0 == ELEMENT_TYPE_OBJECT:
        return NamedType("IInspectable", True), s
    if b0 == ELEMENT_TYPE_SZARRAY:
        b0 = s[0]
        if b0 in (ELEMENT_TYPE_CMOD_REQD, ELEMENT_TYPE_CMOD_OPT):
            c, s = parse_custom_mod(s)
        t, s = decode_type(s)
        # XXX consider c
        return SZArray(t), s
    if b0 == ELEMENT_TYPE_VAR:
        raise NotImplementedError
        param, s = decompress_integer(s)
        print('T%d' % param, end=' ')
        return s
    raise NotImplementedError(hex(b0))

def parse_param(s):
    b0 = s[0]
    if b0 in (ELEMENT_TYPE_CMOD_REQD, ELEMENT_TYPE_CMOD_OPT):
        raise NotImplementedError
        s = parse_custom_mod(s)
        return parse_param(s)
    if b0 == ELEMENT_TYPE_BYREF:
        raise NotImplementedError
        print('BYREF', end=' ')
        return decode_type(s[1:])
    elif b0 == ELEMENT_TYPE_TYPEDBYREF:
        raise NotImplementedError
        print('TYPEDBYREF', end=' ')
    elif b0 == ELEMENT_TYPE_VOID:
        raise NotImplementedError
        print('void', end=' ')
    else:
        return decode_type(s)

def parse_sig(this, s, name):
    # 22.2.3 StandAloneMethodSig
    s0 = s[0]
    s = s[1:]
    #if s0 & 0x20:
    #    print("HASTHIS", end='')
    #if s0 & 0x40:
    #    print("EXPLICITTHIS", end=' ')
    #callconv = ('DEFAULT', 'C', 'STDCALL', 'THISCALL', 'FASTCALL', 'VARARG')
    #print(callconv[s0 & 7], end=' ')
    nparams, s = decompress_integer(s)
    try:
        rtype, s = decode_type(s)
        if rtype.is_void():
            rtype = None
        params = []
        for i in range(nparams):
            t, s = parse_param(s)
            params.append(t)
    except NotImplementedError:
        raise
        params = None
    outfile.write('static PyObject*\n%s_%s(PyObject *_self, PyObject *args)\n{\n' %
                  (cname, name))
    if params is None:
        outfile.write('  PyErr_SetString(PyExc_NotImplementedError, "signature is unsupported");\n')
        outfile.write('  return NULL;\n')
    else:
        outfile.write('  RTObject* self = (RTObject*)_self;\n')
        outfile.write('  %s *_this = (%s*)self->_com;\n' % (this.tname, this.tname))
        if rtype:
            if rtype.is_array():
                outfile.write('  UINT32 result_size;\n')
            outfile.write('  %s\n' % rtype.decl_var('result'))
        outfile.write('  HRESULT hres;\n')
        for i, p in enumerate(params):
            if p.is_array():
                outfile.write("  UINT32 param%d_size;\n" % i)
            outfile.write("  %s\n" % p.decl_var("param%d" % i))
        outfile.write("  hres = _this->%s(" % name)
        parm_strings = []
        for i, p in enumerate(params):
            if p.is_array():
                parm_strings.append("param%d_size" % i)
            parm_strings.append("param%d" % i)
        if rtype:
            if rtype.is_array():
                parm_strings.append('&result_size')
            parm_strings.append('&result')
        outfile.write(", ".join(parm_strings))
        outfile.write(');\n')
    outfile.write('}\n\n')

# No header file for these namespaces
noheader = {
    "Windows.Data",
    "Windows.Devices",
    "Windows.Graphics",
    "Windows.Management",
    "Windows.Security"
    }
    
included = set()
def include(h):
    if h in included:
        return
    outfile.write('#include <%s.h>\n' % h)
    included.add(h)

def namespaces(n, files):
    if n not in noheader:
        include(n)
    mdfiles, subns = winrt.RoResolveNamespace(n)
    files.extend(mdfiles)
    for sub in subns:
        namespaces(n+"."+sub, files)

outfile = open('rtwrapper.c', 'w')
outfile.write('#include "rtsupport.c"\n')

files = []
namespaces('Windows', files)
classes = []

dispenser = winrt.newdispenser()

def print_type():
    outfile.write("static PyMethodDef %s_methods[] = {\n" % cname)
    for m in methods:
        outfile.write('  {"%s", %s_%s, METH_VARARGS},\n' % (m, cname, m))
    outfile.write("  {NULL}\n};\n\n")
    
    outfile.write("static PyType_Slot %s_slots[] = {\n" % cname)
    outfile.write("  {Py_tp_methods, %s_methods},\n" % cname)
    outfile.write("  {0, 0}\n};\n\n")

    outfile.write("static PyType_Spec %s_spec = {\n" % cname)
    outfile.write('  "%s",\n' % cname.split('_')[-1])
    outfile.write('  sizeof(RTObject),\n')
    outfile.write('  0,\n')
    outfile.write('  Py_TPFLAGS_DEFAULT,\n')
    outfile.write('  %s_slots,\n' % cname)
    outfile.write('};\n\n')

def gen_rtclass():
    outfile.write('/* rtclass %s */\n' % tname)
    for intf in implements:
        klass, intf = mdi.GetInterfaceImplProps(intf)
        assert klass == typedef
        if token_type(intf) == mdtTypeRef:
            outfile.write('/* implements %r */\n' % (mdi.GetTypeRefProps(intf),))
        elif token_type(intf) == mdtTypeSpec:
            outfile.write('/* implements %r */\n' % mdi.GetTypeSpecFromToken(intf))

def get_attributes(typedef):
    attrs = []
    for cv in mdi.EnumCustomAttributes(None, typedef, None, 1000):
        tkObj, tkType, pBlob = mdi.GetCustomAttributeProps(cv)
        assert tkObj == typedef
        atype, aname, asig = mdi.GetMemberRefProps(tkType)
        assert aname == '.ctor'
        ascope, aname = mdi.GetTypeRefProps(atype)
        params = decode_custom_attribute(asig, pBlob)
        attrs.append((aname, params))
    return attrs

# skip for now
skipped_types = {
    'Windows.Devices.Enumeration.DeviceThumbnail',
    'Windows.ApplicationModel.Background.IBackgroundTaskBuilder',
    'Windows.ApplicationModel.Background.IPushNotificationTriggerFactory',
    'Windows.Devices.Sms.ISmsDevice',
    'Windows.Management.Deployment.IPackageManager',
    'Windows.UI.Xaml.Media.Animation.IStoryboard',
    # no this type
    'Windows.ApplicationModel.Background.BackgroundExecutionManager',
    # 8.1 only
    'Windows.ApplicationModel.Background.IAlarmApplicationManagerStatics',
    'Windows.ApplicationModel.Background.IBackgroundWorkCostStatics',
    }
skipped_namespaces = {
    'Windows.Foundation.Metadata', # no header file
    'Windows.Graphics.Printing.OptionDetails', # inconsistent type names
    'Windows.UI.Xaml.Documents', # missing types
    'Windows.UI.Xaml.Controls', # vector types
    'Windows.UI.Xaml.Controls.Primitives', # vector types
    'Windows.UI.Xaml', # vector types
    'Windows.UI.Xaml.Media', # vector types
    #'Windows.ApplicationModel.Appointments', # 8.1 only
    #'Windows.ApplicationModel.Appointments.AppointmentsProvider', # 8.1 only
    #'Windows.ApplicationModel.Search.Core', # 8.1 only
    #'Windows.ApplicationModel.Calls', # 8.1 only
    }

# First find all types, so that we don't need to follow assembly references
mdi_by_file = {}
type_by_name = {}
for f in files:
    mdi = mdi_by_file[f] = dispenser.OpenScope(f)
    for typedef in mdi.EnumTypeDefs(None, 1000):
        tname, flags, base = mdi.GetTypeDefProps(typedef)
        assert tname not in type_by_name
        type_by_name[tname] = (mdi, typedef)

for f in files:
    outfile.write("/********* %s ************/\n" % f)
    mdi = mdi_by_file[f]
    for typedef in mdi.EnumTypeDefs(None, 1000):
        attrs = [param[0] for name, param in get_attributes(typedef)
                 if name == 'Windows.Foundation.Metadata.VersionAttribute']
        if attrs and attrs[0] > 6020000:
            # Skip Windows 8.1 and newer
            continue
        tname, flags, base = mdi.GetTypeDefProps(typedef)
        namespace = tname.rsplit('.', 1)[0]
        if namespace in skipped_namespaces:
            continue
        include(namespace)
    outfile.write('using namespace ABI;\n')
    for typedef in mdi.EnumTypeDefs(None, 1000):
        tname, flags, base = mdi.GetTypeDefProps(typedef)
        namespace = tname.rsplit('.', 1)[0]
        if namespace in skipped_namespaces:
            continue
        if tname in skipped_types:
            continue
        cname = tname.replace('.', '_')
        if not (flags & 0x20):
            implements =  mdi.EnumInterfaceImpls(None, typedef, 1000)
            if implements:
               gen_rtclass()
               continue
            # skip non-interface types for now
            continue
        if '`' in tname:
            # XXX generics
            continue
        try:
            this = NamedType(tname, 0, True)
        except NotImplementedError:
            continue
        outfile.write("/******* %s %x ********/\n" % (tname, typedef))
        attrs = get_attributes(typedef)
        for aname, aparam in attrs:
            if aname == 'Windows.Foundation.Metadata.VersionAttribute':
                version = aparam[0]
                outfile.write('/* Version %x */\n' % version)
            else:
                outfile.write('/* %s, %r */\n' % (aname, aparam))
        methods = set()
        overloaded = set()
        # skip over overloaded methods
        for md in mdi.EnumMethods(None, typedef, 100):
            mt, name, flags, sig, flags = mdi.GetMethodProps(md)
            if name in methods:
                overloaded.add(name)
            methods.add(name)
        classes.append(cname)
        methods = set()
        for md in mdi.EnumMethods(None, typedef, 100):
            mt, name, flags, sig, flags = mdi.GetMethodProps(md)
            if name in overloaded:
                # XXX resolve overloading somehow
                continue
            if name == '.ctor':
                # XXX ctor methods
                continue
            parse_sig(this, sig, name)
            methods.add(name)
        print_type()

outfile.write('''
static struct PyModuleDef rtwrappers = {
    PyModuleDef_HEAD_INIT,
    "_rtwrappers",
    NULL,
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_rtwrappers(void)
{
  PyObject *c = NULL;
  PyObject *m = PyModule_Create(&rtwrappers);
  if (m == NULL)
    return NULL;  
''')
for c in classes:
    outfile.write('  c = PyType_FromSpec(&%s_spec);\n' % c)
    outfile.write('  if (c == NULL) goto error;\n')
    outfile.write('  if (PyModule_AddObject(m, "%s", c) < 0)goto error;\n' % c)
outfile.write('''
 error:
  Py_DECREF(m);
  Py_XDECREF(c);
  return NULL;
}
''')

outfile.close()
