/* Socket module */

/*

This module provides an interface to Berkeley socket IPC.

Limitations:

- Only AF_INET, AF_INET6 and AF_UNIX address families are supported in a
  portable manner, though AF_PACKET, AF_NETLINK and AF_TIPC are supported
  under Linux.
- No read/write operations (use sendall/recv or makefile instead).
- Additional restrictions apply on some non-Unix platforms (compensated
  for by socket.py).

Module interface:

- socket.error: exception raised for socket specific errors
- socket.gaierror: exception raised for getaddrinfo/getnameinfo errors,
    a subclass of socket.error
- socket.herror: exception raised for gethostby* errors,
    a subclass of socket.error
- socket.gethostbyname(hostname) --> host IP address (string: 'dd.dd.dd.dd')
- socket.gethostbyaddr(IP address) --> (hostname, [alias, ...], [IP addr, ...])
- socket.gethostname() --> host name (string: 'spam' or 'spam.domain.com')
- socket.getprotobyname(protocolname) --> protocol number
- socket.getservbyname(servicename[, protocolname]) --> port number
- socket.getservbyport(portnumber[, protocolname]) --> service name
- socket.socket([family[, type [, proto, fileno]]]) --> new socket object
    (fileno specifies a pre-existing socket file descriptor)
- socket.socketpair([family[, type [, proto]]]) --> (socket, socket)
- socket.ntohs(16 bit value) --> new int object
- socket.ntohl(32 bit value) --> new int object
- socket.htons(16 bit value) --> new int object
- socket.htonl(32 bit value) --> new int object
- socket.getaddrinfo(host, port [, family, socktype, proto, flags])
    --> List of (family, socktype, proto, canonname, sockaddr)
- socket.getnameinfo(sockaddr, flags) --> (host, port)
- socket.AF_INET, socket.SOCK_STREAM, etc.: constants from <socket.h>
- socket.has_ipv6: boolean value indicating if IPv6 is supported
- socket.inet_aton(IP address) -> 32-bit packed IP representation
- socket.inet_ntoa(packed IP) -> IP address string
- socket.getdefaulttimeout() -> None | float
- socket.setdefaulttimeout(None | float)
- socket.if_nameindex() -> list of tuples (if_index, if_name)
- socket.if_nametoindex(name) -> corresponding interface index
- socket.if_indextoname(index) -> corresponding interface name
- an Internet socket address is a pair (hostname, port)
  where hostname can be anything recognized by gethostbyname()
  (including the dd.dd.dd.dd notation) and port is in host byte order
- where a hostname is returned, the dd.dd.dd.dd notation is used
- a UNIX domain socket address is a string specifying the pathname
- an AF_PACKET socket address is a tuple containing a string
  specifying the ethernet interface and an integer specifying
  the Ethernet protocol number to be received. For example:
  ("eth0",0x1234).  Optional 3rd,4th,5th elements in the tuple
  specify packet-type and ha-type/addr.
- an AF_TIPC socket address is expressed as
 (addr_type, v1, v2, v3 [, scope]); where addr_type can be one of:
    TIPC_ADDR_NAMESEQ, TIPC_ADDR_NAME, and TIPC_ADDR_ID;
  and scope can be one of:
    TIPC_ZONE_SCOPE, TIPC_CLUSTER_SCOPE, and TIPC_NODE_SCOPE.
  The meaning of v1, v2 and v3 depends on the value of addr_type:
    if addr_type is TIPC_ADDR_NAME:
        v1 is the server type
        v2 is the port identifier
        v3 is ignored
    if addr_type is TIPC_ADDR_NAMESEQ:
        v1 is the server type
        v2 is the lower port number
        v3 is the upper port number
    if addr_type is TIPC_ADDR_ID:
        v1 is the node
        v2 is the ref
        v3 is ignored


Local naming conventions:

- names starting with sock_ are socket object methods
- names starting with socket_ are module-level functions
- names starting with PySocket are exported through socketmodule.h

*/

#include <ppltasks.h>
#include "Python.h"
#include "structmember.h"

using namespace Windows::Foundation;
using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace Platform;
using namespace Concurrency;
/* Constants from winsock */
#define AF_INET 2
#define AF_INET6 23
#define SOCK_STREAM 1
#define SOCK_DGRAM 2

struct sock_addr_t {
    HostName ^host;
    String ^service;
};

/* Global variable holding the exception type for errors detected
   by this module (but not argument type or memory errors, etc.). */
static PyObject *socket_herror;
static PyObject *socket_gaierror;
static PyObject *socket_timeout;

namespace{
    extern PyTypeObject sock_type;
}

#undef MAX
#define MAX(x, y) ((x) < (y) ? (y) : (x))

/* The object holding a socket.  It holds some extra information,
   like the address family, which is used to decode socket address
   arguments properly. */

typedef struct {
    PyObject_HEAD
    Platform::Object^ sock_fd;           /* Socket file descriptor */
    DataWriter ^writer;
    DataReader ^reader;
    int sock_family;            /* Address family, e.g., AF_INET */
    int sock_type;              /* Socket type, e.g., SOCK_STREAM */
    int sock_proto;             /* Protocol type, usually 0 */
    PyObject *(*errorhandler)(void); /* Error handler; checks
                                        errno, returns NULL and
                                        sets a Python exception */
    double sock_timeout;                 /* Operation timeout in seconds;
                                        0.0 means non-blocking */
} PySocketSockObject;

/* Socket object documentation */
PyDoc_STRVAR(sock_doc,
"socket([family[, type[, proto]]]) -> socket object\n\
\n\
Open a socket of the given type.  The family argument specifies the\n\
address family; it defaults to AF_INET.  The type argument specifies\n\
whether this is a stream (SOCK_STREAM, this is the default)\n\
or datagram (SOCK_DGRAM) socket.  The protocol argument defaults to 0,\n\
specifying the default protocol.  Keyword arguments are accepted.\n\
\n\
A socket object represents one endpoint of a network connection.\n\
\n\
Methods of socket objects (keyword arguments not allowed):\n\
\n\
_accept() -- accept connection, returning new socket fd and client address\n\
bind(addr) -- bind the socket to a local address\n\
close() -- close the socket\n\
connect(addr) -- connect the socket to a remote address\n\
connect_ex(addr) -- connect, return an error code instead of an exception\n\
_dup() -- return a new socket fd duplicated from fileno()\n\
fileno() -- return underlying file descriptor\n\
getpeername() -- return remote address [*]\n\
getsockname() -- return local address\n\
getsockopt(level, optname[, buflen]) -- get socket options\n\
gettimeout() -- return timeout or None\n\
listen(n) -- start listening for incoming connections\n\
recv(buflen[, flags]) -- receive data\n\
recv_into(buffer[, nbytes[, flags]]) -- receive data (into a buffer)\n\
recvfrom(buflen[, flags]) -- receive data and sender\'s address\n\
recvfrom_into(buffer[, nbytes, [, flags])\n\
  -- receive data and sender\'s address (into a buffer)\n\
sendall(data[, flags]) -- send all data\n\
send(data[, flags]) -- send data, may not send all of it\n\
sendto(data[, flags], addr) -- send data to a given address\n\
setblocking(0 | 1) -- set or clear the blocking I/O flag\n\
setsockopt(level, optname, value) -- set socket options\n\
settimeout(None | float) -- set or clear the timeout\n\
shutdown(how) -- shut down traffic in one or both directions\n\
if_nameindex() -- return all network interface indices and names\n\
if_nametoindex(name) -- return the corresponding interface index\n\
if_indextoname(index) -- return the corresponding interface name\n\
\n\
 [*] not available on all platforms!");

static PyObject*
select_error(void)
{
    PyErr_SetString(PyExc_OSError, "unable to select on socket");
    return NULL;
}

#ifdef MS_WINDOWS
#ifndef WSAEAGAIN
#define WSAEAGAIN WSAEWOULDBLOCK
#endif
#define CHECK_ERRNO(expected) \
    (WSAGetLastError() == WSA ## expected)
#else
#define CHECK_ERRNO(expected) \
    (errno == expected)
#endif

/* Convenience function to raise an error according to errno
   and return a NULL pointer from a function. */

static PyObject *
set_error(void)
{
    /* XXX */
    return NULL;
}


static PyObject *
set_herror(int h_error)
{
    /* XXX */
    return NULL;
}


static PyObject *
set_gaierror(int error)
{
    /* XXX */
    return NULL;
}

/* Function to perform the setting of socket blocking mode
   internally. block = (1 | 0). */
static int
internal_setblocking(PySocketSockObject *s, int block)
{
    /* XXX later */
    return 1;
}

/* Do a select()/poll() on the socket, if necessary (sock_timeout > 0).
   The argument writing indicates the direction.
   This does not raise an exception; we'll let our caller do that
   after they've reacquired the interpreter lock.
   Returns 1 on timeout, -1 on error, 0 otherwise. */
static int
internal_select_ex(PySocketSockObject *s, int writing, double interval)
{
    /* Nothing to do unless we're in timeout mode (not non-blocking) */
    if (s->sock_timeout <= 0.0)
        return 0;

    /* Guard against closed socket */
    if (s->sock_fd == nullptr)
        return 0;

    /* Handling this condition here simplifies the select loops */
    if (interval < 0.0)
        return 1;

    /* XXX not supported in winrt? */
    return 1;
}

static int
internal_select(PySocketSockObject *s, int writing)
{
    return internal_select_ex(s, writing, s->sock_timeout);
}

/*
   Two macros for automatic retry of select() in case of false positives
   (for example, select() could indicate a socket is ready for reading
    but the data then discarded by the OS because of a wrong checksum).
   Here is an example of use:

    BEGIN_SELECT_LOOP(s)
    Py_BEGIN_ALLOW_THREADS
    timeout = internal_select_ex(s, 0, interval);
    if (!timeout)
        outlen = recv(s->sock_fd, cbuf, len, flags);
    Py_END_ALLOW_THREADS
    if (timeout == 1) {
        PyErr_SetString(socket_timeout, "timed out");
        return -1;
    }
    END_SELECT_LOOP(s)
*/

#define BEGIN_SELECT_LOOP(s) \
    { \
        _PyTime_timeval now, deadline = {0, 0}; \
        double interval = s->sock_timeout; \
        int has_timeout = s->sock_timeout > 0.0; \
        if (has_timeout) { \
            _PyTime_gettimeofday(&now); \
            deadline = now; \
            _PyTime_ADD_SECONDS(deadline, s->sock_timeout); \
        } \
        while (1) { \
            errno = 0; \

#define END_SELECT_LOOP(s) \
            if (!has_timeout || \
                (!CHECK_ERRNO(EWOULDBLOCK) && !CHECK_ERRNO(EAGAIN))) \
                break; \
            _PyTime_gettimeofday(&now); \
            interval = _PyTime_INTERVAL(now, deadline); \
        } \
    } \

/* Initialize a new socket object. */

static double defaulttimeout = -1.0; /* Default timeout for new sockets */

static void
init_sockobject(PySocketSockObject *s,
                Platform::Object^ fd, int family, int type, int proto)
{
    s->sock_fd = fd;
    s->sock_family = family;
    s->sock_type = type;
    s->sock_proto = proto;

    s->errorhandler = &set_error;
#ifdef SOCK_NONBLOCK
    if (type & SOCK_NONBLOCK)
        s->sock_timeout = 0.0;
    else
#endif
    {
        s->sock_timeout = defaulttimeout;
        if (defaulttimeout >= 0.0)
            internal_setblocking(s, 0);
    }

}


/* Create a new socket object.
   This just creates the object and initializes it.
   If the creation fails, return NULL and set an exception (implicit
   in NEWOBJ()). */

static PySocketSockObject *
new_sockobject(Platform::Object^ fd, int family, int type, int proto)
{
    PySocketSockObject *s;
    s = (PySocketSockObject *)
        PyType_GenericNew(&sock_type, NULL, NULL);
    if (s != NULL)
        init_sockobject(s, fd, family, type, proto);
    return s;
}


/* Lock to allow python interpreter to continue, but only allow one
   thread to be in gethostbyname or getaddrinfo */
#if defined(USE_GETHOSTBYNAME_LOCK) || defined(USE_GETADDRINFO_LOCK)
PyThread_type_lock netdb_lock;
#endif


/* Convert a string specifying a host name or one of a few symbolic
   names to a numeric IP address.  This usually calls gethostbyname()
   to do the work; the names "" and "<broadcast>" are special.
   Return the length (IPv4 should be 4 bytes), or negative if
   an error occurred; then an exception is raised. */

static int
setipaddr(char *name, struct sockaddr *addr_ret, size_t addr_ret_size, int af)
{
    /* XXX later */
    PyErr_SetString(PyExc_OSError, "unknown address family");
    return -1;
}


/* Create a string object representing an IP address.
   This is always a string of the form 'dd.dd.dd.dd' (with variable
   size numbers). */

static PyObject *
makeipaddr(struct sockaddr *addr, int addrlen)
{
    /* XXX later */
    PyErr_SetString(PyExc_NotImplementedError, "makeipaddr");
    return NULL;
}


/* Create an object representing the given socket address,
   suitable for passing it back to bind(), connect() etc.
   The family field of the sockaddr structure is inspected
   to determine what kind of address it really is. */

static PyObject *
makesockaddr(Platform::Object^ sockfd, struct sockaddr *addr, size_t addrlen, int proto)
{
    PyErr_SetString(PyExc_NotImplementedError, "makesockaddr");
    return NULL;
}


/* Parse a socket address argument according to the socket object's
   address family.  Return 1 if the address was in the proper format,
   0 of not.  The address is returned through addr_ret, its length
   through len_ret. */

static int
getsockaddrarg(PySocketSockObject *s, PyObject *args,
               struct sock_addr_t *addr_ret)
{
    Py_UNICODE *host;
    int port;
    PyObject *strport;
    if (!PyTuple_Check(args)) {
        PyErr_Format(
                PyExc_TypeError,
                "getsockaddrarg: "
                "address must be tuple, not %.500s",
                Py_TYPE(args)->tp_name);
        return 0;
    }
    if (!PyArg_ParseTuple(args, "ui", &host, &port))
        return 0;
    strport = PyUnicode_FromFormat("%i", port);
    if (!strport)
        return 1;

    addr_ret->host = ref new HostName(ref new String(host));
    addr_ret->service = ref new String(PyUnicode_AsUnicode(strport));
    Py_DECREF(strport);
    return 1;
}


/* s._accept() -> (fd, address) */

static PyObject *
sock_accept(PySocketSockObject *s)
{
    StreamSocketListener ^fd;

    try {
        fd = safe_cast<StreamSocketListener^>(s->sock_fd);
    } catch(...) {
        return select_error();
    }

    /* XXX ConnectionReceived */
    PyErr_SetString(PyExc_NotImplementedError, "accept");
    return NULL;
}

PyDoc_STRVAR(accept_doc,
"_accept() -> (socket, address info)\n\
\n\
Wait for an incoming connection.  Return a new socket file descriptor\n\
representing the connection, and the address of the client.\n\
For IP sockets, the address info is a pair (hostaddr, port).");

/* s.setblocking(flag) method.  Argument:
   False -- non-blocking mode; same as settimeout(0)
   True -- blocking mode; same as settimeout(None)
*/

static PyObject *
sock_setblocking(PySocketSockObject *s, PyObject *arg)
{
    int block;

    block = PyLong_AsLong(arg);
    if (block == -1 && PyErr_Occurred())
        return NULL;

    s->sock_timeout = block ? -1.0 : 0.0;
    internal_setblocking(s, block);

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(setblocking_doc,
"setblocking(flag)\n\
\n\
Set the socket to blocking (flag is true) or non-blocking (false).\n\
setblocking(True) is equivalent to settimeout(None);\n\
setblocking(False) is equivalent to settimeout(0.0).");

/* s.settimeout(timeout) method.  Argument:
   None -- no timeout, blocking mode; same as setblocking(True)
   0.0  -- non-blocking mode; same as setblocking(False)
   > 0  -- timeout mode; operations time out after timeout seconds
   < 0  -- illegal; raises an exception
*/
static PyObject *
sock_settimeout(PySocketSockObject *s, PyObject *arg)
{
    double timeout;

    if (arg == Py_None)
        timeout = -1.0;
    else {
        timeout = PyFloat_AsDouble(arg);
        if (timeout < 0.0) {
            if (!PyErr_Occurred())
                PyErr_SetString(PyExc_ValueError,
                                "Timeout value out of range");
            return NULL;
        }
    }

    s->sock_timeout = timeout;
    internal_setblocking(s, timeout < 0.0);

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(settimeout_doc,
"settimeout(timeout)\n\
\n\
Set a timeout on socket operations.  'timeout' can be a float,\n\
giving in seconds, or None.  Setting a timeout of None disables\n\
the timeout feature and is equivalent to setblocking(1).\n\
Setting a timeout of zero is the same as setblocking(0).");

/* s.gettimeout() method.
   Returns the timeout associated with a socket. */
static PyObject *
sock_gettimeout(PySocketSockObject *s)
{
    if (s->sock_timeout < 0.0) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    else
        return PyFloat_FromDouble(s->sock_timeout);
}

PyDoc_STRVAR(gettimeout_doc,
"gettimeout() -> timeout\n\
\n\
Returns the timeout in seconds (float) associated with socket \n\
operations. A timeout of None indicates that timeouts on socket \n\
operations are disabled.");

/* s.setsockopt() method.
   With an integer third argument, sets an integer option.
   With a string third argument, sets an option from a buffer;
   use optional built-in module 'struct' to encode the string. */

static PyObject *
sock_setsockopt(PySocketSockObject *s, PyObject *args)
{
    int level;
    int optname;
    int res;
    char *buf;
    int buflen;
    int flag;

    if (PyArg_ParseTuple(args, "iii:setsockopt",
                         &level, &optname, &flag)) {
        buf = (char *) &flag;
        buflen = sizeof flag;
    }
    else {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args, "iiy#:setsockopt",
                              &level, &optname, &buf, &buflen))
            return NULL;
    }
    /* XXX implement */
    res = -1;
    if (res < 0)
        return s->errorhandler();
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(setsockopt_doc,
"setsockopt(level, option, value)\n\
\n\
Set a socket option.  See the Unix manual for level and option.\n\
The value argument can either be an integer or a string.");


#if 0
/* s.getsockopt() method.
   With two arguments, retrieves an integer option.
   With a third integer argument, retrieves a string buffer of that size;
   use optional built-in module 'struct' to decode the string. */

static PyObject *
sock_getsockopt(PySocketSockObject *s, PyObject *args)
{
    int level;
    int optname;
    int res;
    PyObject *buf;
    socklen_t buflen = 0;

    if (!PyArg_ParseTuple(args, "ii|i:getsockopt",
                          &level, &optname, &buflen))
        return NULL;

    if (buflen == 0) {
        int flag = 0;
        socklen_t flagsize = sizeof flag;
        res = getsockopt(s->sock_fd, level, optname,
                         (void *)&flag, &flagsize);
        if (res < 0)
            return s->errorhandler();
        return PyLong_FromLong(flag);
    }
    if (buflen <= 0 || buflen > 1024) {
        PyErr_SetString(PyExc_OSError,
                        "getsockopt buflen out of range");
        return NULL;
    }
    buf = PyBytes_FromStringAndSize((char *)NULL, buflen);
    if (buf == NULL)
        return NULL;
    /* XXX implement */
    res = -1;
    if (res < 0) {
        Py_DECREF(buf);
        return s->errorhandler();
    }
    _PyBytes_Resize(&buf, buflen);
    return buf;
}
#endif

PyDoc_STRVAR(getsockopt_doc,
"getsockopt(level, option[, buffersize]) -> value\n\
\n\
Get a socket option.  See the Unix manual for level and option.\n\
If a nonzero buffersize argument is given, the return value is a\n\
string of that length; otherwise it is an integer.");


/* s.bind(sockaddr) method */

static PyObject *
sock_bind(PySocketSockObject *s, PyObject *addro)
{
    PyErr_SetString(PyExc_NotADirectoryError, "bind");
    return NULL;
#if 0
    sock_addr_t addrbuf;
    int addrlen;
    int res;

    if (!getsockaddrarg(s, addro, SAS2SA(&addrbuf), &addrlen))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = bind(s->sock_fd, SAS2SA(&addrbuf), addrlen);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return s->errorhandler();
    Py_INCREF(Py_None);
    return Py_None;
#endif
}

PyDoc_STRVAR(bind_doc,
"bind(address)\n\
\n\
Bind the socket to a local address.  For IP sockets, the address is a\n\
pair (host, port); the host must refer to the local host. For raw packet\n\
sockets the address is a tuple (ifname, proto [,pkttype [,hatype]])");


/* s.close() method.
   Set the file descriptor to -1 so operations tried subsequently
   will surely fail. */

static PyObject *
sock_close(PySocketSockObject *s)
{
    Platform::IDisposable^ fd;

    if ((fd = (Platform::IDisposable^)s->sock_fd) != nullptr) {
        //XXX compiler rejects the call to dispose
        //fd->Dispose();
        s->sock_fd = nullptr;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(close_doc,
"close()\n\
\n\
Close the socket.  It cannot be used after this call.");

static PyObject *
sock_detach(PySocketSockObject *s)
{
    PyErr_SetString(PyExc_NotImplementedError, "detach");
    return NULL;
#if 0
    Platform::Object^ fd = s->sock_fd;
    s->sock_fd = -1;
    return PyLong_FromSocket_t(fd);
#endif
}

PyDoc_STRVAR(detach_doc,
"detach()\n\
\n\
Close the socket object without closing the underlying file descriptor.\
The object cannot be used after this call, but the file descriptor\
can be reused for other purposes.  The file descriptor is returned.");

/* s.connect(sockaddr) method */

static PyObject *
sock_connect(PySocketSockObject *s, PyObject *addro)
{
    sock_addr_t addrbuf;
    int res = 0;
    int timeout = 0;
    HostName ^host;
    String ^error = nullptr;

    StreamSocket ^sock = (StreamSocket^)(s->sock_fd);

    if (!getsockaddrarg(s, addro, &addrbuf))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    auto t = create_task(sock->ConnectAsync(addrbuf.host, addrbuf.service, SocketProtectionLevel::PlainSocket));
	try{
		t.get();
	}catch(Exception ^e){
		error = e->ToString();
	}
    Py_END_ALLOW_THREADS

    if (timeout == 1) {
        //PyErr_SetString(socket_timeout, "timed out");
        return NULL;
    }
    if (error != nullptr) {
        PyObject *msg = PyUnicode_FromUnicode(error->Data(), error->Length());
        /* XXX error check */
        PyErr_SetObject(PyExc_OSError, msg);
        Py_DECREF(msg);
        return NULL;
    }
    else {
        s->writer = ref new DataWriter(sock->OutputStream);
        s->reader = ref new DataReader(sock->InputStream);
    }
        
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(connect_doc,
"connect(address)\n\
\n\
Connect the socket to a remote address.  For IP sockets, the address\n\
is a pair (host, port).");


/* s.connect_ex(sockaddr) method */

static PyObject *
sock_connect_ex(PySocketSockObject *s, PyObject *addro)
{
    PyErr_SetString(PyExc_NotImplementedError, "connect_ex");
    return NULL;
#if 0
    sock_addr_t addrbuf;
    int addrlen;
    int res;
    int timeout;

    if (!getsockaddrarg(s, addro, SAS2SA(&addrbuf), &addrlen))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    res = internal_connect(s, SAS2SA(&addrbuf), addrlen, &timeout);
    Py_END_ALLOW_THREADS

    /* Signals are not errors (though they may raise exceptions).  Adapted
       from PyErr_SetFromErrnoWithFilenameObject(). */
#ifdef EINTR
    if (res == EINTR && PyErr_CheckSignals())
        return NULL;
#endif

    return PyLong_FromLong((long) res);
#endif
}

PyDoc_STRVAR(connect_ex_doc,
"connect_ex(address) -> errno\n\
\n\
This is like connect(address), but returns an error code (the errno value)\n\
instead of raising an exception when an error occurs.");


/* s.fileno() method */

static PyObject *
sock_fileno(PySocketSockObject *s)
{
    PyErr_SetString(PyExc_NotImplementedError, "fileno");
    return NULL;
    // return PyLong_FromSocket_t(s->sock_fd);
}

PyDoc_STRVAR(fileno_doc,
"fileno() -> integer\n\
\n\
Return the integer file descriptor of the socket.");


/* s.getsockname() method */

static PyObject *
sock_getsockname(PySocketSockObject *s)
{
    PyErr_SetString(PyExc_NotImplementedError, "getsockname");
    return NULL;
#if 0
    sock_addr_t addrbuf;
    int res;
    socklen_t addrlen;

    if (!getsockaddrlen(s, &addrlen))
        return NULL;
    memset(&addrbuf, 0, addrlen);
    Py_BEGIN_ALLOW_THREADS
    res = getsockname(s->sock_fd, SAS2SA(&addrbuf), &addrlen);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return s->errorhandler();
    return makesockaddr(s->sock_fd, SAS2SA(&addrbuf), addrlen,
                        s->sock_proto);
#endif
}

PyDoc_STRVAR(getsockname_doc,
"getsockname() -> address info\n\
\n\
Return the address of the local endpoint.  For IP sockets, the address\n\
info is a pair (hostaddr, port).");


/* s.getpeername() method */

static PyObject *
sock_getpeername(PySocketSockObject *s)
{
    PyErr_SetString(PyExc_NotImplementedError, "getpeername");
    return NULL;
#if 0
    sock_addr_t addrbuf;
    int res;
    socklen_t addrlen;

    if (!getsockaddrlen(s, &addrlen))
        return NULL;
    memset(&addrbuf, 0, addrlen);
    Py_BEGIN_ALLOW_THREADS
    res = getpeername(s->sock_fd, SAS2SA(&addrbuf), &addrlen);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return s->errorhandler();
    return makesockaddr(s->sock_fd, SAS2SA(&addrbuf), addrlen,
                        s->sock_proto);
#endif
}

PyDoc_STRVAR(getpeername_doc,
"getpeername() -> address info\n\
\n\
Return the address of the remote endpoint.  For IP sockets, the address\n\
info is a pair (hostaddr, port).");



/* s.listen(n) method */

static PyObject *
sock_listen(PySocketSockObject *s, PyObject *arg)
{
    PyErr_SetString(PyExc_NotImplementedError, "listen");
    return NULL;
#if 0
    int backlog;
    int res;

    backlog = PyLong_AsLong(arg);
    if (backlog == -1 && PyErr_Occurred())
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    /* To avoid problems on systems that don't allow a negative backlog
     * (which doesn't make sense anyway) we force a minimum value of 0. */
    if (backlog < 0)
        backlog = 0;
    res = listen(s->sock_fd, backlog);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return s->errorhandler();
    Py_INCREF(Py_None);
    return Py_None;
#endif
}

PyDoc_STRVAR(listen_doc,
"listen(backlog)\n\
\n\
Enable a server to accept connections.  The backlog argument must be at\n\
least 0 (if it is lower, it is set to 0); it specifies the number of\n\
unaccepted connections that the system will allow before refusing new\n\
connections.");


/* s.recv(nbytes [,flags]) method */

static PyObject *
sock_recv(PySocketSockObject *s, PyObject *args)
{
    Py_ssize_t recvlen;
    int flags = 0;

    if (!PyArg_ParseTuple(args, "n|i:recv", &recvlen, &flags))
        return NULL;

    if (recvlen < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "negative buffersize in recv");
        return NULL;
    }

	size_t bytesread;
	auto t = create_task(s->reader->LoadAsync(recvlen));
	try {
		bytesread = t.get();
	} catch(Exception ^e) {
		String ^error = e->ToString();
		PyObject *msg = PyUnicode_FromUnicode(error->Data(), error->Length());
		if (!msg)
			return NULL;
		PyErr_SetObject(PyExc_OSError, msg);
		Py_DECREF(msg);
		return NULL;
	}

	auto buf = ref new Array<unsigned char>(bytesread);
	s->reader->ReadBytes(buf);
    return PyBytes_FromStringAndSize((char*)buf->Data, buf->Length);
}

PyDoc_STRVAR(recv_doc,
"recv(buffersize[, flags]) -> data\n\
\n\
Receive up to buffersize bytes from the socket.  For the optional flags\n\
argument, see the Unix manual.  When no data is available, block until\n\
at least one byte is available or until the remote end is closed.  When\n\
the remote end is closed and all data is read, return the empty string.");


/* s.recv_into(buffer, [nbytes [,flags]]) method */

static PyObject*
sock_recv_into(PySocketSockObject *s, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"buffer", "nbytes", "flags", 0};

    int flags = 0;
    Py_buffer pbuf;
    Py_ssize_t readlen, recvlen = 0;

    /* Get the buffer's memory */
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "w*|ni:recv_into", kwlist,
                                     &pbuf, &recvlen, &flags))

    if (recvlen < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "negative buffersize in recv");
        return NULL;
    }

    if (recvlen == 0) {
        /* If nbytes was not specified, use the buffer's length */
        recvlen = pbuf.len;
    }

    /* Check if the buffer is large enough */
    if (pbuf.len < recvlen) {
        PyBuffer_Release(&pbuf);
        PyErr_SetString(PyExc_ValueError,
                        "buffer too small for requested bytes");
        return NULL;
    }

    size_t bytesread;
    auto t = create_task(s->reader->LoadAsync(recvlen));
    try {
        bytesread = t.get();
    } catch(Exception ^e) {
        String ^error = e->ToString();
        PyObject *msg = PyUnicode_FromUnicode(error->Data(), error->Length());
        if (!msg)
            return NULL;
        PyErr_SetObject(PyExc_OSError, msg);
        Py_DECREF(msg);
        return NULL;
    }

    auto buf = ref new Array<unsigned char>(bytesread);
    s->reader->ReadBytes(buf);
    memcpy(pbuf.buf, buf->Data, bytesread);
    PyBuffer_Release(&pbuf);
    return PyLong_FromSize_t(bytesread);
}

PyDoc_STRVAR(recv_into_doc,
"recv_into(buffer, [nbytes[, flags]]) -> nbytes_read\n\
\n\
A version of recv() that stores its data into a buffer rather than creating \n\
a new string.  Receive up to buffersize bytes from the socket.  If buffersize \n\
is not specified (or 0), receive up to the size available in the given buffer.\n\
\n\
See recv() for documentation about the flags.");


/* s.recvfrom(nbytes [,flags]) method */

static PyObject *
sock_recvfrom(PySocketSockObject *s, PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "recvfrom");
    return NULL;
#if 0
    PyObject *buf = NULL;
    PyObject *addr = NULL;
    PyObject *ret = NULL;
    int flags = 0;
    Py_ssize_t recvlen, outlen;

    if (!PyArg_ParseTuple(args, "n|i:recvfrom", &recvlen, &flags))
        return NULL;

    if (recvlen < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "negative buffersize in recvfrom");
        return NULL;
    }

    buf = PyBytes_FromStringAndSize((char *) 0, recvlen);
    if (buf == NULL)
        return NULL;

    outlen = sock_recvfrom_guts(s, PyBytes_AS_STRING(buf),
                                recvlen, flags, &addr);
    if (outlen < 0) {
        goto finally;
    }

    if (outlen != recvlen) {
        /* We did not read as many bytes as we anticipated, resize the
           string if possible and be successful. */
        if (_PyBytes_Resize(&buf, outlen) < 0)
            /* Oopsy, not so successful after all. */
            goto finally;
    }

    ret = PyTuple_Pack(2, buf, addr);

finally:
    Py_XDECREF(buf);
    Py_XDECREF(addr);
    return ret;
#endif
}

PyDoc_STRVAR(recvfrom_doc,
"recvfrom(buffersize[, flags]) -> (data, address info)\n\
\n\
Like recv(buffersize, flags) but also return the sender's address info.");


/* s.recvfrom_into(buffer[, nbytes [,flags]]) method */

static PyObject *
sock_recvfrom_into(PySocketSockObject *s, PyObject *args, PyObject* kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "recvfrom_into");
    return NULL;
#if 0
    static char *kwlist[] = {"buffer", "nbytes", "flags", 0};

    int flags = 0;
    Py_buffer pbuf;
    char *buf;
    Py_ssize_t readlen, buflen, recvlen = 0;

    PyObject *addr = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "w*|ni:recvfrom_into",
                                     kwlist, &pbuf,
                                     &recvlen, &flags))
        return NULL;
    buf = pbuf.buf;
    buflen = pbuf.len;
    assert(buf != 0 && buflen > 0);

    if (recvlen < 0) {
        PyBuffer_Release(&pbuf);
        PyErr_SetString(PyExc_ValueError,
                        "negative buffersize in recvfrom_into");
        return NULL;
    }
    if (recvlen == 0) {
        /* If nbytes was not specified, use the buffer's length */
        recvlen = buflen;
    }

    readlen = sock_recvfrom_guts(s, buf, recvlen, flags, &addr);
    if (readlen < 0) {
        PyBuffer_Release(&pbuf);
        /* Return an error */
        Py_XDECREF(addr);
        return NULL;
    }

    PyBuffer_Release(&pbuf);
    /* Return the number of bytes read and the address.  Note that we do
       not do anything special here in the case that readlen < recvlen. */
    return Py_BuildValue("nN", readlen, addr);
#endif
}

PyDoc_STRVAR(recvfrom_into_doc,
"recvfrom_into(buffer[, nbytes[, flags]]) -> (nbytes, address info)\n\
\n\
Like recv_into(buffer[, nbytes[, flags]]) but also return the sender's address info.");


/* s.send(data [,flags]) method */

static PyObject *
sock_send(PySocketSockObject *s, PyObject *args)
{
    StreamSocket ^fd = (StreamSocket^)s->sock_fd;
    DataWriter ^writer = s->writer;
    int flags = 0, timeout = 0;
	size_t n;
    Py_buffer pbuf;

    if (!PyArg_ParseTuple(args, "y*|i:send", &pbuf, &flags))
        return NULL;

    auto rtbuf = ref new Array<unsigned char, 1U>((unsigned char*)pbuf.buf, pbuf.len);
    Py_BEGIN_ALLOW_THREADS
    s->writer->WriteBytes(rtbuf);
    n = create_task(s->writer->StoreAsync()).get();
    Py_END_ALLOW_THREADS

    /*
    if (timeout == 1) {
        PyBuffer_Release(&pbuf);
        //PyErr_SetString(socket_timeout, "timed out");
        return NULL;
    }
    */

    PyBuffer_Release(&pbuf);
    /*
    if (n < 0)
        return s->errorhandler();
    */
    return PyLong_FromSsize_t(n);
}

PyDoc_STRVAR(send_doc,
"send(data[, flags]) -> count\n\
\n\
Send a data string to the socket.  For the optional flags\n\
argument, see the Unix manual.  Return the number of bytes\n\
sent; this may be less than len(data) if the network is busy.");


/* s.sendall(data [,flags]) method */

static PyObject *
sock_sendall(PySocketSockObject *s, PyObject *args)
{
    Py_ssize_t len, n = -1;
    int flags = 0, timeout /* XXX implement timeout */;
    Py_buffer pbuf;
    if (!PyArg_ParseTuple(args, "y*|i:sendall", &pbuf, &flags))
        return NULL;
    if (flags != 0) {
        PyBuffer_Release(&pbuf);
        PyErr_SetString(PyExc_ValueError, "non-zero flags are not supported");
        return NULL;
    }
    len = pbuf.len;
    auto rtbuf = ref new Array<unsigned char, 1U>((unsigned char*)pbuf.buf, pbuf.len);
    Py_BEGIN_ALLOW_THREADS
    s->writer->WriteBytes(rtbuf);
    n = create_task(s->writer->StoreAsync()).get();
    Py_END_ALLOW_THREADS
    PyBuffer_Release(&pbuf);
    if (n != len) {
        PyErr_SetString(PyExc_OSError, "Could not send all data");
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(sendall_doc,
"sendall(data[, flags])\n\
\n\
Send a data string to the socket.  For the optional flags\n\
argument, see the Unix manual.  This calls send() repeatedly\n\
until all data is sent.  If an error occurs, it's impossible\n\
to tell how much data has been sent.");


/* s.sendto(data, [flags,] sockaddr) method */

static PyObject *
sock_sendto(PySocketSockObject *s, PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "sendto");
    return NULL;
#if 0
    Py_buffer pbuf;
    PyObject *addro;
    char *buf;
    Py_ssize_t len, arglen;
    sock_addr_t addrbuf;
    int addrlen, n = -1, flags, timeout;

    flags = 0;
    arglen = PyTuple_Size(args);
    switch (arglen) {
        case 2:
            PyArg_ParseTuple(args, "y*O:sendto", &pbuf, &addro);
            break;
        case 3:
            PyArg_ParseTuple(args, "y*iO:sendto",
                             &pbuf, &flags, &addro);
            break;
        default:
            PyErr_Format(PyExc_TypeError,
                         "sendto() takes 2 or 3 arguments (%d given)",
                         arglen);
            return NULL;
    }
    if (PyErr_Occurred())
        return NULL;

    buf = pbuf.buf;
    len = pbuf.len;

    if (!IS_SELECTABLE(s)) {
        PyBuffer_Release(&pbuf);
        return select_error();
    }

    if (!getsockaddrarg(s, addro, SAS2SA(&addrbuf), &addrlen)) {
        PyBuffer_Release(&pbuf);
        return NULL;
    }

    BEGIN_SELECT_LOOP(s)
    Py_BEGIN_ALLOW_THREADS
    timeout = internal_select_ex(s, 1, interval);
    if (!timeout)
        n = sendto(s->sock_fd, buf, len, flags, SAS2SA(&addrbuf), addrlen);
    Py_END_ALLOW_THREADS

    if (timeout == 1) {
        PyBuffer_Release(&pbuf);
        PyErr_SetString(socket_timeout, "timed out");
        return NULL;
    }
    END_SELECT_LOOP(s)
    PyBuffer_Release(&pbuf);
    if (n < 0)
        return s->errorhandler();
    return PyLong_FromSsize_t(n);
#endif
}

PyDoc_STRVAR(sendto_doc,
"sendto(data[, flags], address) -> count\n\
\n\
Like send(data, flags) but allows specifying the destination address.\n\
For IP sockets, the address is a pair (hostaddr, port).");


/* s.shutdown(how) method */

static PyObject *
sock_shutdown(PySocketSockObject *s, PyObject *arg)
{
    PyErr_SetString(PyExc_NotImplementedError, "shutdown");
    return NULL;
#if 0
    int how;
    int res;

    how = PyLong_AsLong(arg);
    if (how == -1 && PyErr_Occurred())
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = shutdown(s->sock_fd, how);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return s->errorhandler();
    Py_INCREF(Py_None);
    return Py_None;
#endif
}

PyDoc_STRVAR(shutdown_doc,
"shutdown(flag)\n\
\n\
Shut down the reading side of the socket (flag == SHUT_RD), the writing side\n\
of the socket (flag == SHUT_WR), or both ends (flag == SHUT_RDWR).");

/* List of methods for socket objects */

static PyMethodDef sock_methods[] = {
    {"_accept",           (PyCFunction)sock_accept, METH_NOARGS,
                      accept_doc},
    {"bind",              (PyCFunction)sock_bind, METH_O,
                      bind_doc},
    {"close",             (PyCFunction)sock_close, METH_NOARGS,
                      close_doc},
    {"connect",           (PyCFunction)sock_connect, METH_O,
                      connect_doc},
    {"connect_ex",        (PyCFunction)sock_connect_ex, METH_O,
                      connect_ex_doc},
    {"detach",            (PyCFunction)sock_detach, METH_NOARGS,
                      detach_doc},
    {"fileno",            (PyCFunction)sock_fileno, METH_NOARGS,
                      fileno_doc},
#ifdef HAVE_GETPEERNAME
    {"getpeername",       (PyCFunction)sock_getpeername,
                      METH_NOARGS, getpeername_doc},
#endif
    {"getsockname",       (PyCFunction)sock_getsockname,
                      METH_NOARGS, getsockname_doc},
#if 0
    {"getsockopt",        (PyCFunction)sock_getsockopt, METH_VARARGS,
                      getsockopt_doc},
#endif
    {"listen",            (PyCFunction)sock_listen, METH_O,
                      listen_doc},
    {"recv",              (PyCFunction)sock_recv, METH_VARARGS,
                      recv_doc},
    {"recv_into",         (PyCFunction)sock_recv_into, METH_VARARGS | METH_KEYWORDS,
                      recv_into_doc},
    {"recvfrom",          (PyCFunction)sock_recvfrom, METH_VARARGS,
                      recvfrom_doc},
    {"recvfrom_into",  (PyCFunction)sock_recvfrom_into, METH_VARARGS | METH_KEYWORDS,
                      recvfrom_into_doc},
    {"send",              (PyCFunction)sock_send, METH_VARARGS,
                      send_doc},
    {"sendall",           (PyCFunction)sock_sendall, METH_VARARGS,
                      sendall_doc},
    {"sendto",            (PyCFunction)sock_sendto, METH_VARARGS,
                      sendto_doc},
    {"setblocking",       (PyCFunction)sock_setblocking, METH_O,
                      setblocking_doc},
    {"settimeout",    (PyCFunction)sock_settimeout, METH_O,
                      settimeout_doc},
    {"gettimeout",    (PyCFunction)sock_gettimeout, METH_NOARGS,
                      gettimeout_doc},
    {"setsockopt",        (PyCFunction)sock_setsockopt, METH_VARARGS,
                      setsockopt_doc},
    {"shutdown",          (PyCFunction)sock_shutdown, METH_O,
                      shutdown_doc},
    {NULL,                      NULL}           /* sentinel */
};

/* SockObject members */
static PyMemberDef sock_memberlist[] = {
       {"family", T_INT, offsetof(PySocketSockObject, sock_family), READONLY, "the socket family"},
       {"type", T_INT, offsetof(PySocketSockObject, sock_type), READONLY, "the socket type"},
       {"proto", T_INT, offsetof(PySocketSockObject, sock_proto), READONLY, "the socket protocol"},
       {"timeout", T_DOUBLE, offsetof(PySocketSockObject, sock_timeout), READONLY, "the socket timeout"},
       {0},
};

/* Deallocate a socket object in response to the last Py_DECREF().
   First close the file description. */

static void
sock_dealloc(PySocketSockObject *s)
{
    if (s->sock_fd != nullptr) {
        PyObject *exc, *val, *tb;
        Py_ssize_t old_refcount = Py_REFCNT(s);
        ++Py_REFCNT(s);
        PyErr_Fetch(&exc, &val, &tb);
        if (PyErr_WarnFormat(PyExc_ResourceWarning, 1,
                             "unclosed %R", s))
            /* Spurious errors can appear at shutdown */
            if (PyErr_ExceptionMatches(PyExc_Warning))
                PyErr_WriteUnraisable((PyObject *) s);
        PyErr_Restore(exc, val, tb);
        // XXX compiler rejects the call to dispose
        //((Platform::IDisposable^)s->sock_fd)->Dispose();
        s->sock_fd = nullptr;
        Py_REFCNT(s) = old_refcount;
    }
    Py_TYPE(s)->tp_free((PyObject *)s);
}


static PyObject *
sock_repr(PySocketSockObject *s)
{
    return PyUnicode_FromFormat(
        "<socket object, fd=%ld, family=%d, type=%d, proto=%d>",
        /* XXX sockfd */
        (long)-1, s->sock_family,
        s->sock_type,
        s->sock_proto);
}


/* Create a new, uninitialized socket object. */

static PyObject *
sock_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *snew;

    snew = type->tp_alloc(type, 0);
    if (snew != NULL) {
        ((PySocketSockObject *)snew)->sock_fd = nullptr;
        ((PySocketSockObject *)snew)->sock_timeout = -1.0;
        ((PySocketSockObject *)snew)->errorhandler = &set_error;
    }
    return snew;
}


/* Initialize a new socket object. */

/*ARGSUSED*/
static int
sock_initobj(PyObject *self, PyObject *args, PyObject *kwds)
{
    PySocketSockObject *s = (PySocketSockObject *)self;
    PyObject *fdobj = NULL;
    Platform::Object^ fd = nullptr;
    int family = AF_INET;
    int type = SOCK_STREAM;
    int proto = 0;
    static char *keywords[] = {"family", "type", "proto", "fileno", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "|iiiO:socket", keywords,
                                     &family, &type, &proto, &fdobj))
        return -1;

    if (fdobj != NULL && fdobj != Py_None) {
        PyErr_SetString(PyExc_NotImplementedError, "socket creation from existing handle");
        return -1;
    }
    else {
        if (family != AF_INET) {
            PyErr_SetString(PyExc_NotImplementedError, "unknown socket family");
            return -1;
        }
        switch(type) {
        case SOCK_STREAM:
            fd = ref new StreamSocket();
            break;
        case SOCK_DGRAM:
            fd = ref new DatagramSocket();
            break;
        default:
            PyErr_SetString(PyExc_NotImplementedError, "unknown socket type");
            return -1;
        }
    }
    init_sockobject(s, fd, family, type, proto);

    return 0;

}


/* Type object for socket objects. */
namespace {
PyTypeObject sock_type = {
    PyVarObject_HEAD_INIT(0, 0)         /* Must fill in type value later */
    "_socket.socket",                           /* tp_name */
    sizeof(PySocketSockObject),                 /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)sock_dealloc,                   /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)sock_repr,                        /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    sock_doc,                                   /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    sock_methods,                               /* tp_methods */
    sock_memberlist,                            /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    sock_initobj,                               /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    sock_new,                                   /* tp_new */
    PyObject_Del,                               /* tp_free */
};
}


/* Python interface to gethostname(). */

/*ARGSUSED*/
static PyObject *
socket_gethostname(PyObject *self, PyObject *unused)
{
    PyErr_SetString(PyExc_NotImplementedError, "gethostname");
    return NULL;
#if 0
    /* Don't use winsock's gethostname, as this returns the ANSI
       version of the hostname, whereas we need a Unicode string.
       Otherwise, gethostname apparently also returns the DNS name. */
    wchar_t buf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = Py_ARRAY_LENGTH(buf);
    wchar_t *name;
    PyObject *result;

    if (GetComputerNameExW(ComputerNamePhysicalDnsHostname, buf, &size))
        return PyUnicode_FromWideChar(buf, size);

    if (GetLastError() != ERROR_MORE_DATA)
        return PyErr_SetFromWindowsErr(0);

    if (size == 0)
        return PyUnicode_New(0, 0);

    /* MSDN says ERROR_MORE_DATA may occur because DNS allows longer
       names */
    name = PyMem_Malloc(size * sizeof(wchar_t));
    if (!name)
        return NULL;
    if (!GetComputerNameExW(ComputerNamePhysicalDnsHostname,
                           name,
                           &size))
    {
        PyMem_Free(name);
        return PyErr_SetFromWindowsErr(0);
    }

    result = PyUnicode_FromWideChar(name, size);
    PyMem_Free(name);
    return result;
#endif
}

PyDoc_STRVAR(gethostname_doc,
"gethostname() -> string\n\
\n\
Return the current host name.");

/* Python interface to gethostbyname(name). */

/*ARGSUSED*/
static PyObject *
socket_gethostbyname(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "gethostbyname");
    return NULL;
#if 0
    char *name;
    sock_addr_t addrbuf;
    PyObject *ret = NULL;

    if (!PyArg_ParseTuple(args, "et:gethostbyname", "idna", &name))
        return NULL;
    if (setipaddr(name, SAS2SA(&addrbuf),  sizeof(addrbuf), AF_INET) < 0)
        goto finally;
    ret = makeipaddr(SAS2SA(&addrbuf), sizeof(struct sockaddr_in));
finally:
    PyMem_Free(name);
    return ret;
#endif
}

PyDoc_STRVAR(gethostbyname_doc,
"gethostbyname(host) -> address\n\
\n\
Return the IP address (a string of the form '255.255.255.255') for a host.");


/* Python interface to gethostbyname_ex(name). */

/*ARGSUSED*/
static PyObject *
socket_gethostbyname_ex(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "gethostbyname_ex");
    return NULL;
}

PyDoc_STRVAR(ghbn_ex_doc,
"gethostbyname_ex(host) -> (name, aliaslist, addresslist)\n\
\n\
Return the true host name, a list of aliases, and a list of IP addresses,\n\
for a host.  The host argument is a string giving a host name or IP number.");


/* Python interface to gethostbyaddr(IP). */

/*ARGSUSED*/
static PyObject *
socket_gethostbyaddr(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "gethostbyaddr");
    return NULL;
}

PyDoc_STRVAR(gethostbyaddr_doc,
"gethostbyaddr(host) -> (name, aliaslist, addresslist)\n\
\n\
Return the true host name, a list of aliases, and a list of IP addresses,\n\
for a host.  The host argument is a string giving a host name or IP number.");


/* Python interface to getservbyname(name).
   This only returns the port number, since the other info is already
   known or not useful (like the list of aliases). */

/*ARGSUSED*/
static PyObject *
socket_getservbyname(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "getservbyname");
    return NULL;
}

PyDoc_STRVAR(getservbyname_doc,
"getservbyname(servicename[, protocolname]) -> integer\n\
\n\
Return a port number from a service name and protocol name.\n\
The optional protocol name, if given, should be 'tcp' or 'udp',\n\
otherwise any protocol will match.");


/* Python interface to getservbyport(port).
   This only returns the service name, since the other info is already
   known or not useful (like the list of aliases). */

/*ARGSUSED*/
static PyObject *
socket_getservbyport(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "getservbyport");
    return NULL;
}

PyDoc_STRVAR(getservbyport_doc,
"getservbyport(port[, protocolname]) -> string\n\
\n\
Return the service name from a port number and protocol name.\n\
The optional protocol name, if given, should be 'tcp' or 'udp',\n\
otherwise any protocol will match.");

/* Python interface to getprotobyname(name).
   This only returns the protocol number, since the other info is
   already known or not useful (like the list of aliases). */

/*ARGSUSED*/
static PyObject *
socket_getprotobyname(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "getprotobyname");
    return NULL;
}

PyDoc_STRVAR(getprotobyname_doc,
"getprotobyname(name) -> integer\n\
\n\
Return the protocol number for the named protocol.  (Rarely used.)");


/* dup() function for socket fds */

static PyObject *
socket_dup(PyObject *self, PyObject *fdobj)
{
    PyErr_SetString(PyExc_NotImplementedError, "dup");
    return NULL;
}

PyDoc_STRVAR(dup_doc,
"dup(integer) -> integer\n\
\n\
Duplicate an integer socket file descriptor.  This is like os.dup(), but for\n\
sockets; on some platforms os.dup() won't work for socket file descriptors.");


static PyObject *
socket_ntohs(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "ntohs");
    return NULL;
#if 0
    int x1, x2;

    if (!PyArg_ParseTuple(args, "i:ntohs", &x1)) {
        return NULL;
    }
    if (x1 < 0) {
        PyErr_SetString(PyExc_OverflowError,
            "can't convert negative number to unsigned long");
        return NULL;
    }
    x2 = (unsigned int)ntohs((unsigned short)x1);
    return PyLong_FromLong(x2);
#endif
}

PyDoc_STRVAR(ntohs_doc,
"ntohs(integer) -> integer\n\
\n\
Convert a 16-bit integer from network to host byte order.");


static PyObject *
socket_ntohl(PyObject *self, PyObject *arg)
{
    PyErr_SetString(PyExc_NotImplementedError, "ntohl");
    return NULL;
#if 0
    unsigned long x;

    if (PyLong_Check(arg)) {
        x = PyLong_AsUnsignedLong(arg);
        if (x == (unsigned long) -1 && PyErr_Occurred())
            return NULL;
#if SIZEOF_LONG > 4
        {
            unsigned long y;
            /* only want the trailing 32 bits */
            y = x & 0xFFFFFFFFUL;
            if (y ^ x)
                return PyErr_Format(PyExc_OverflowError,
                            "long int larger than 32 bits");
            x = y;
        }
#endif
    }
    else
        return PyErr_Format(PyExc_TypeError,
                            "expected int/long, %s found",
                            Py_TYPE(arg)->tp_name);
    if (x == (unsigned long) -1 && PyErr_Occurred())
        return NULL;
    return PyLong_FromUnsignedLong(ntohl(x));
#endif
}

PyDoc_STRVAR(ntohl_doc,
"ntohl(integer) -> integer\n\
\n\
Convert a 32-bit integer from network to host byte order.");


static PyObject *
socket_htons(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "htons");
    return NULL;
#if 0
    int x1, x2;

    if (!PyArg_ParseTuple(args, "i:htons", &x1)) {
        return NULL;
    }
    if (x1 < 0) {
        PyErr_SetString(PyExc_OverflowError,
            "can't convert negative number to unsigned long");
        return NULL;
    }
    x2 = (unsigned int)htons((unsigned short)x1);
    return PyLong_FromLong(x2);
#endif
}

PyDoc_STRVAR(htons_doc,
"htons(integer) -> integer\n\
\n\
Convert a 16-bit integer from host to network byte order.");


static PyObject *
socket_htonl(PyObject *self, PyObject *arg)
{
    PyErr_SetString(PyExc_NotImplementedError, "ntohl");
    return NULL;
#if 0
    unsigned long x;

    if (PyLong_Check(arg)) {
        x = PyLong_AsUnsignedLong(arg);
        if (x == (unsigned long) -1 && PyErr_Occurred())
            return NULL;
#if SIZEOF_LONG > 4
        {
            unsigned long y;
            /* only want the trailing 32 bits */
            y = x & 0xFFFFFFFFUL;
            if (y ^ x)
                return PyErr_Format(PyExc_OverflowError,
                            "long int larger than 32 bits");
            x = y;
        }
#endif
    }
    else
        return PyErr_Format(PyExc_TypeError,
                            "expected int/long, %s found",
                            Py_TYPE(arg)->tp_name);
    return PyLong_FromUnsignedLong(htonl((unsigned long)x));
#endif
}

PyDoc_STRVAR(htonl_doc,
"htonl(integer) -> integer\n\
\n\
Convert a 32-bit integer from host to network byte order.");

/* socket.inet_aton() and socket.inet_ntoa() functions. */

PyDoc_STRVAR(inet_aton_doc,
"inet_aton(string) -> bytes giving packed 32-bit IP representation\n\
\n\
Convert an IP address in string format (123.45.67.89) to the 32-bit packed\n\
binary format used in low-level network functions.");

static PyObject*
socket_inet_aton(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "inet_aton");
    return NULL;
#if 0

#ifndef INADDR_NONE
#define INADDR_NONE (-1)
#endif
#ifdef HAVE_INET_ATON
    struct in_addr buf;
#endif

#if !defined(HAVE_INET_ATON) || defined(USE_INET_ATON_WEAKLINK)
#if (SIZEOF_INT != 4)
#error "Not sure if in_addr_t exists and int is not 32-bits."
#endif
    /* Have to use inet_addr() instead */
    unsigned int packed_addr;
#endif
    char *ip_addr;

    if (!PyArg_ParseTuple(args, "s:inet_aton", &ip_addr))
        return NULL;


#ifdef HAVE_INET_ATON

#ifdef USE_INET_ATON_WEAKLINK
    if (inet_aton != NULL) {
#endif
    if (inet_aton(ip_addr, &buf))
        return PyBytes_FromStringAndSize((char *)(&buf),
                                          sizeof(buf));

    PyErr_SetString(PyExc_OSError,
                    "illegal IP address string passed to inet_aton");
    return NULL;

#ifdef USE_INET_ATON_WEAKLINK
   } else {
#endif

#endif

#if !defined(HAVE_INET_ATON) || defined(USE_INET_ATON_WEAKLINK)

    /* special-case this address as inet_addr might return INADDR_NONE
     * for this */
    if (strcmp(ip_addr, "255.255.255.255") == 0) {
        packed_addr = 0xFFFFFFFF;
    } else {

        packed_addr = inet_addr(ip_addr);

        if (packed_addr == INADDR_NONE) {               /* invalid address */
            PyErr_SetString(PyExc_OSError,
                "illegal IP address string passed to inet_aton");
            return NULL;
        }
    }
    return PyBytes_FromStringAndSize((char *) &packed_addr,
                                      sizeof(packed_addr));

#ifdef USE_INET_ATON_WEAKLINK
   }
#endif

#endif
#endif
}

PyDoc_STRVAR(inet_ntoa_doc,
"inet_ntoa(packed_ip) -> ip_address_string\n\
\n\
Convert an IP address from 32-bit packed binary format to string format");

static PyObject*
socket_inet_ntoa(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "inet_ntoa");
    return NULL;
#if 0

    char *packed_str;
    int addr_len;
    struct in_addr packed_addr;

    if (!PyArg_ParseTuple(args, "y#:inet_ntoa", &packed_str, &addr_len)) {
        return NULL;
    }

    if (addr_len != sizeof(packed_addr)) {
        PyErr_SetString(PyExc_OSError,
            "packed IP wrong length for inet_ntoa");
        return NULL;
    }

    memcpy(&packed_addr, packed_str, addr_len);

    return PyUnicode_FromString(inet_ntoa(packed_addr));
#endif
}

/* Python interface to getaddrinfo(host, port). */

/*ARGSUSED*/
static PyObject *
socket_getaddrinfo(PyObject *self, PyObject *args, PyObject* kwargs)
{
    PyErr_SetString(PyExc_NotImplementedError, "getaddrinfo");
    return NULL;
#if 0

    static char* kwnames[] = {"host", "port", "family", "type", "proto",
                              "flags", 0};
    struct addrinfo hints, *res;
    struct addrinfo *res0 = NULL;
    PyObject *hobj = NULL;
    PyObject *pobj = (PyObject *)NULL;
    char pbuf[30];
    char *hptr, *pptr;
    int family, socktype, protocol, flags;
    int error;
    PyObject *all = (PyObject *)NULL;
    PyObject *idna = NULL;

    family = socktype = protocol = flags = 0;
    family = AF_UNSPEC;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|iiii:getaddrinfo",
                          kwnames, &hobj, &pobj, &family, &socktype,
                          &protocol, &flags)) {
        return NULL;
    }
    if (hobj == Py_None) {
        hptr = NULL;
    } else if (PyUnicode_Check(hobj)) {
        _Py_IDENTIFIER(encode);

        idna = _PyObject_CallMethodId(hobj, &PyId_encode, "s", "idna");
        if (!idna)
            return NULL;
        assert(PyBytes_Check(idna));
        hptr = PyBytes_AS_STRING(idna);
    } else if (PyBytes_Check(hobj)) {
        hptr = PyBytes_AsString(hobj);
    } else {
        PyErr_SetString(PyExc_TypeError,
                        "getaddrinfo() argument 1 must be string or None");
        return NULL;
    }
    if (PyLong_CheckExact(pobj)) {
        long value = PyLong_AsLong(pobj);
        if (value == -1 && PyErr_Occurred())
            goto err;
        PyOS_snprintf(pbuf, sizeof(pbuf), "%ld", value);
        pptr = pbuf;
    } else if (PyUnicode_Check(pobj)) {
        pptr = _PyUnicode_AsString(pobj);
        if (pptr == NULL)
            goto err;
    } else if (PyBytes_Check(pobj)) {
        pptr = PyBytes_AS_STRING(pobj);
    } else if (pobj == Py_None) {
        pptr = (char *)NULL;
    } else {
        PyErr_SetString(PyExc_OSError, "Int or String expected");
        goto err;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_protocol = protocol;
    hints.ai_flags = flags;
    Py_BEGIN_ALLOW_THREADS
    ACQUIRE_GETADDRINFO_LOCK
    error = getaddrinfo(hptr, pptr, &hints, &res0);
    Py_END_ALLOW_THREADS
    RELEASE_GETADDRINFO_LOCK  /* see comment in setipaddr() */
    if (error) {
        set_gaierror(error);
        goto err;
    }

    if ((all = PyList_New(0)) == NULL)
        goto err;
    for (res = res0; res; res = res->ai_next) {
        PyObject *single;
        PyObject *addr =
            makesockaddr(-1, res->ai_addr, res->ai_addrlen, protocol);
        if (addr == NULL)
            goto err;
        single = Py_BuildValue("iiisO", res->ai_family,
            res->ai_socktype, res->ai_protocol,
            res->ai_canonname ? res->ai_canonname : "",
            addr);
        Py_DECREF(addr);
        if (single == NULL)
            goto err;

        if (PyList_Append(all, single))
            goto err;
        Py_XDECREF(single);
    }
    Py_XDECREF(idna);
    if (res0)
        freeaddrinfo(res0);
    return all;
 err:
    Py_XDECREF(all);
    Py_XDECREF(idna);
    if (res0)
        freeaddrinfo(res0);
    return (PyObject *)NULL;
#endif
}

PyDoc_STRVAR(getaddrinfo_doc,
"getaddrinfo(host, port [, family, socktype, proto, flags])\n\
    -> list of (family, socktype, proto, canonname, sockaddr)\n\
\n\
Resolve host and port into addrinfo struct.");

/* Python interface to getnameinfo(sa, flags). */

/*ARGSUSED*/
static PyObject *
socket_getnameinfo(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "getnameinfo");
    return NULL;
#if 0
    PyObject *sa = (PyObject *)NULL;
    int flags;
    char *hostp;
    int port;
    unsigned int flowinfo, scope_id;
    char hbuf[NI_MAXHOST], pbuf[NI_MAXSERV];
    struct addrinfo hints, *res = NULL;
    int error;
    PyObject *ret = (PyObject *)NULL;

    flags = flowinfo = scope_id = 0;
    if (!PyArg_ParseTuple(args, "Oi:getnameinfo", &sa, &flags))
        return NULL;
    if (!PyTuple_Check(sa)) {
        PyErr_SetString(PyExc_TypeError,
                        "getnameinfo() argument 1 must be a tuple");
        return NULL;
    }
    if (!PyArg_ParseTuple(sa, "si|II",
                          &hostp, &port, &flowinfo, &scope_id))
        return NULL;
    if (flowinfo > 0xfffff) {
        PyErr_SetString(PyExc_OverflowError,
                        "getsockaddrarg: flowinfo must be 0-1048575.");
        return NULL;
    }
    PyOS_snprintf(pbuf, sizeof(pbuf), "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;     /* make numeric port happy */
    hints.ai_flags = AI_NUMERICHOST;    /* don't do any name resolution */
    Py_BEGIN_ALLOW_THREADS
    ACQUIRE_GETADDRINFO_LOCK
    error = getaddrinfo(hostp, pbuf, &hints, &res);
    Py_END_ALLOW_THREADS
    RELEASE_GETADDRINFO_LOCK  /* see comment in setipaddr() */
    if (error) {
        set_gaierror(error);
        goto fail;
    }
    if (res->ai_next) {
        PyErr_SetString(PyExc_OSError,
            "sockaddr resolved to multiple addresses");
        goto fail;
    }
    switch (res->ai_family) {
    case AF_INET:
        {
        if (PyTuple_GET_SIZE(sa) != 2) {
            PyErr_SetString(PyExc_OSError,
                "IPv4 sockaddr must be 2 tuple");
            goto fail;
        }
        break;
        }
#ifdef ENABLE_IPV6
    case AF_INET6:
        {
        struct sockaddr_in6 *sin6;
        sin6 = (struct sockaddr_in6 *)res->ai_addr;
        sin6->sin6_flowinfo = htonl(flowinfo);
        sin6->sin6_scope_id = scope_id;
        break;
        }
#endif
    }
    error = getnameinfo(res->ai_addr, (socklen_t) res->ai_addrlen,
                    hbuf, sizeof(hbuf), pbuf, sizeof(pbuf), flags);
    if (error) {
        set_gaierror(error);
        goto fail;
    }
    ret = Py_BuildValue("ss", hbuf, pbuf);

fail:
    if (res)
        freeaddrinfo(res);
    return ret;
#endif
}

PyDoc_STRVAR(getnameinfo_doc,
"getnameinfo(sockaddr, flags) --> (host, port)\n\
\n\
Get host and port for a sockaddr.");


/* Python API to getting and setting the default timeout value. */

static PyObject *
socket_getdefaulttimeout(PyObject *self)
{
    if (defaulttimeout < 0.0) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    else
        return PyFloat_FromDouble(defaulttimeout);
}

PyDoc_STRVAR(getdefaulttimeout_doc,
"getdefaulttimeout() -> timeout\n\
\n\
Returns the default timeout in seconds (float) for new socket objects.\n\
A value of None indicates that new socket objects have no timeout.\n\
When the socket module is first imported, the default is None.");

static PyObject *
socket_setdefaulttimeout(PyObject *self, PyObject *arg)
{
    double timeout;

    if (arg == Py_None)
        timeout = -1.0;
    else {
        timeout = PyFloat_AsDouble(arg);
        if (timeout < 0.0) {
            if (!PyErr_Occurred())
                PyErr_SetString(PyExc_ValueError,
                                "Timeout value out of range");
            return NULL;
        }
    }

    defaulttimeout = timeout;

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(setdefaulttimeout_doc,
"setdefaulttimeout(timeout)\n\
\n\
Set the default timeout in seconds (float) for new socket objects.\n\
A value of None indicates that new socket objects have no timeout.\n\
When the socket module is first imported, the default is None.");

/* List of functions exported by this module. */

static PyMethodDef socket_methods[] = {
    {"gethostbyname",           socket_gethostbyname,
     METH_VARARGS, gethostbyname_doc},
    {"gethostbyname_ex",        socket_gethostbyname_ex,
     METH_VARARGS, ghbn_ex_doc},
    {"gethostbyaddr",           socket_gethostbyaddr,
     METH_VARARGS, gethostbyaddr_doc},
    {"gethostname",             socket_gethostname,
     METH_NOARGS,  gethostname_doc},
    {"getservbyname",           socket_getservbyname,
     METH_VARARGS, getservbyname_doc},
    {"getservbyport",           socket_getservbyport,
     METH_VARARGS, getservbyport_doc},
    {"getprotobyname",          socket_getprotobyname,
     METH_VARARGS, getprotobyname_doc},
    {"dup",                     socket_dup,
     METH_O, dup_doc},
    {"ntohs",                   socket_ntohs,
     METH_VARARGS, ntohs_doc},
    {"ntohl",                   socket_ntohl,
     METH_O, ntohl_doc},
    {"htons",                   socket_htons,
     METH_VARARGS, htons_doc},
    {"htonl",                   socket_htonl,
     METH_O, htonl_doc},
    {"inet_aton",               socket_inet_aton,
     METH_VARARGS, inet_aton_doc},
    {"inet_ntoa",               socket_inet_ntoa,
     METH_VARARGS, inet_ntoa_doc},
    {"getaddrinfo",             (PyCFunction)socket_getaddrinfo,
     METH_VARARGS | METH_KEYWORDS, getaddrinfo_doc},
    {"getnameinfo",             socket_getnameinfo,
     METH_VARARGS, getnameinfo_doc},
    {"getdefaulttimeout",       (PyCFunction)socket_getdefaulttimeout,
     METH_NOARGS, getdefaulttimeout_doc},
    {"setdefaulttimeout",       socket_setdefaulttimeout,
     METH_O, setdefaulttimeout_doc},
    {NULL,                      NULL}            /* Sentinel */
};


/* Initialize the _socket module.

   This module is actually called "_socket", and there's a wrapper
   "socket.py" which implements some additional functionality.
   The import of "_socket" may fail with an ImportError exception if
   os-specific initialization fails.  On Windows, this does WINSOCK
   initialization.  When WINSOCK is initialized successfully, a call to
   WSACleanup() is scheduled to be made at exit time.
*/

PyDoc_STRVAR(socket_doc,
"Implementation module for socket operations.\n\
\n\
See the socket module for documentation.");

static struct PyModuleDef socketmodule = {
    PyModuleDef_HEAD_INIT,
    "_socket",
    socket_doc,
    -1,
    socket_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

extern "C"
PyMODINIT_FUNC
PyInit__socket(void)
{
    PyObject *m, *has_ipv6;

    Py_TYPE(&sock_type) = &PyType_Type;
    m = PyModule_Create(&socketmodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(PyExc_OSError);
    PyModule_AddObject(m, "error", PyExc_OSError);
    socket_herror = PyErr_NewException("socket.herror",
                                       PyExc_OSError, NULL);
    if (socket_herror == NULL)
        return NULL;
    Py_INCREF(socket_herror);
    PyModule_AddObject(m, "herror", socket_herror);
    socket_gaierror = PyErr_NewException("socket.gaierror", PyExc_OSError,
        NULL);
    if (socket_gaierror == NULL)
        return NULL;
    Py_INCREF(socket_gaierror);
    PyModule_AddObject(m, "gaierror", socket_gaierror);
    socket_timeout = PyErr_NewException("socket.timeout",
                                        PyExc_OSError, NULL);
    if (socket_timeout == NULL)
        return NULL;
    Py_INCREF(socket_timeout);
    PyModule_AddObject(m, "timeout", socket_timeout);
    Py_INCREF((PyObject *)&sock_type);
    if (PyModule_AddObject(m, "SocketType",
                           (PyObject *)&sock_type) != 0)
        return NULL;
    Py_INCREF((PyObject *)&sock_type);
    if (PyModule_AddObject(m, "socket",
                           (PyObject *)&sock_type) != 0)
        return NULL;

    has_ipv6 = Py_True;
    Py_INCREF(has_ipv6);
    PyModule_AddObject(m, "has_ipv6", has_ipv6);

    /* Address families (we only support AF_INET and AF_UNIX) */
    PyModule_AddIntConstant(m, "AF_INET", AF_INET);
    PyModule_AddIntConstant(m, "AF_INET6", AF_INET6);
    /* Socket types */
    PyModule_AddIntConstant(m, "SOCK_STREAM", SOCK_STREAM);
    PyModule_AddIntConstant(m, "SOCK_DGRAM", SOCK_DGRAM);
    /* Some reserved IP v.4 addresses */
    PyModule_AddIntConstant(m, "INADDR_ANY", 0x00000000);
    PyModule_AddIntConstant(m, "INADDR_BROADCAST", 0xffffffff);
    PyModule_AddIntConstant(m, "INADDR_LOOPBACK", 0x7F000001);
    PyModule_AddIntConstant(m, "INADDR_UNSPEC_GROUP", 0xe0000000);
    PyModule_AddIntConstant(m, "INADDR_ALLHOSTS_GROUP", 0xe0000001);
    PyModule_AddIntConstant(m, "INADDR_MAX_LOCAL_GROUP", 0xe00000ff);
    PyModule_AddIntConstant(m, "INADDR_NONE", 0xffffffff);
    /* shutdown() parameters */
    PyModule_AddIntConstant(m, "SHUT_RD", 0);
    PyModule_AddIntConstant(m, "SHUT_WR", 1);
    PyModule_AddIntConstant(m, "SHUT_RDWR", 2);
    return m;
}
