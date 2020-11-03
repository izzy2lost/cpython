import winrt

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
    ELEMENT_TYPE_BOOLEAN: 'bool',
    ELEMENT_TYPE_CHAR: 'wchar_t',
    ELEMENT_TYPE_I1: 'char',
    ELEMENT_TYPE_U1: 'unsigned char',
    ELEMENT_TYPE_I2: 'short',
    ELEMENT_TYPE_U2: 'unsigned short',
    ELEMENT_TYPE_I4: 'int',
    ELEMENT_TYPE_U4: 'unsigned int',
    ELEMENT_TYPE_I8: 'int64_t',
    ELEMENT_TYPE_U8: 'uint65_t',
    ELEMENT_TYPE_R4: 'float',
    ELEMENT_TYPE_R8: 'double',
    ELEMENT_TYPE_STRING: 'String',
    ELEMENT_TYPE_I: 'int',
}


def decompress_integer(s):
    b0 = s[0]
    if (b0 & 0x80) == 0:
        return b0, s[1:]
    if (b0 & 0x40) == 0:
        return ((b0 & 0x3f) << 8) + s[1], s[2:]
    return (((b0 & 0x3f) << 24) + (s[1]<<16) +
            (s[2] << 8) + s[3]), s[4:]

def decode_type(s):
    b0 = s[0]
    s = s[1:]
    if b0 in tnames:
        print(tnames[b0], end=' ')
        return s
    if b0 in (ELEMENT_TYPE_CLASS, ELEMENT_TYPE_VALUETYPE):
        i, s = decompress_integer(s)
        table = i & 0x3
        value = i >> 2
        token = (table<<24) + value
        scope, name = mdi.GetTypeRefProps(token)
        print("[%x]%s" % (scope, name), end=' ')
        return s
    if b0 == ELEMENT_TYPE_GENERICINST:
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
        print('Object', end=' ')
        return s
    if b0 == ELEMENT_TYPE_SZARRAY:
        b0 = s[0]
        if b0 in (ELEMENT_TYPE_CMOD_REQD, ELEMENT_TYPE_CMOD_OPT):
            s = parse_custom_mod(s)
        decode_type(s)
        print('[]', end=' ')
        return s
    if b0 == ELEMENT_TYPE_VAR:
        param, s = decompress_integer(s)
        print('T%d' % param, end=' ')
        return s
    assert 0, hex(b0)

def parse_param(s):
    b0 = s[0]
    if b0 in (ELEMENT_TYPE_CMOD_REQD, ELEMENT_TYPE_CMOD_OPT):
        s = parse_custom_mod(s)
        return parse_param(s)
    if b0 == ELEMENT_TYPE_BYREF:
        print('BYREF', end=' ')
        return decode_type(s[1:])
    elif b0 == ELEMENT_TYPE_TYPEDBYREF:
        print('TYPEDBYREF', end=' ')
    elif b0 == ELEMENT_TYPE_VOID:
        print('void', end=' ')
    else:
        return decode_type(s)

def parse_sig(s, name):
    # 22.2.3 StandAloneMethodSig
    s0 = s[0]
    s = s[1:]
    if s0 & 0x20:
        print("HASTHIS", end='')
    if s0 & 0x40:
        print("EXPLICITTHIS", end=' ')
    callconv = ('DEFAULT', 'C', 'STDCALL', 'THISCALL', 'FASTCALL', 'VARARG')
    print(callconv[s0 & 7], end=' ')
    nparams, s = decompress_integer(s)
    s = decode_type(s)
    print(name, '(', sep='', end=' ')
    for i in range(nparams):
        s = parse_param(s)
        if i < nparams-1:
            print(',', end=' ')
    print(')')


#a,mdi,c = winrt.RoGetMetaDataFile('Windows.Media.MediaControl')

#print(a)
#mds = mdi.EnumMethods(None, c, 100)
#for md in mds:
#    mt, name, flags, sig, flags = mdi.GetMethodProps(md)
#    parse_sig(sig, name)

def namespaces(n, files):
    print(n)
    mdfiles, subns = winrt.RoResolveNamespace(n)
    files.extend(mdfiles)
    for sub in subns:
        namespaces(n+"."+sub, files)


files = []
namespaces('Windows', files)

dispenser = winrt.newdispenser()
print(dispenser)

for f in files:
    print(f)
    mdi = dispenser.OpenScope(f)
    for typedef in mdi.EnumTypeDefs(None, 1000):
        tname, flags, base = mdi.GetTypeDefProps(typedef)
        for md in mdi.EnumMethods(None, typedef, 100):
            mt, name, flags, sig, flags = mdi.GetMethodProps(md)
            parse_sig(sig, "%s.%s" % (tname, name))

