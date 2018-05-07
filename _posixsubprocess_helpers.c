/* Functions and macros from Python 3.2 not found in 2.x.
   This file is #included by _posixsubprocess.c and the functions
   are declared static to avoid exposing them outside this module. */

#include "unicodeobject.h"

#if (PY_VERSION_HEX < 0x02050000)
#define Py_ssize_t      int
#endif

#define Py_CLEANUP_SUPPORTED 0x20000

/* Issue #1983: pid_t can be longer than a C long on some systems */
#if !defined(SIZEOF_PID_T) || SIZEOF_PID_T == SIZEOF_INT
#define PyLong_FromPid PyLong_FromLong
#elif SIZEOF_PID_T == SIZEOF_LONG
#define PyLong_FromPid PyLong_FromLong
#elif defined(SIZEOF_LONG_LONG) && SIZEOF_PID_T == SIZEOF_LONG_LONG
#define PyLong_FromPid PyLong_FromLongLong
#else
#error "sizeof(pid_t) is neither sizeof(int), sizeof(long) or sizeof(long long)"
#endif /* SIZEOF_PID_T */


static PyObject *PyUnicode_EncodeFSDefault(PyObject *unicode)
{
    if (Py_FileSystemDefaultEncoding)
        return PyUnicode_AsEncodedString(unicode,
                                         Py_FileSystemDefaultEncoding,
                                         "strict");
    else
        return PyUnicode_EncodeUTF8(PyUnicode_AS_UNICODE(unicode),
                                    PyUnicode_GET_SIZE(unicode),
                                    "strict");
}


/* Convert the argument to a bytes object, according to the file
   system encoding.  The addr param must be a PyObject**.
   This is designed to be used with "O&" in PyArg_Parse APIs. */

static int
PyUnicode_FSConverter(PyObject* arg, void* addr)
{
    PyObject *output = NULL;
    Py_ssize_t size;
    void *data;
    if (arg == NULL) {
        Py_DECREF(*(PyObject**)addr);
        return 1;
    }
    if (PyString_Check(arg)) {
        output = arg;
        Py_INCREF(output);
    }
    else {
        arg = PyUnicode_FromObject(arg);
        if (!arg)
            return 0;
        output = PyUnicode_EncodeFSDefault(arg);
        Py_DECREF(arg);
        if (!output)
            return 0;
        if (!PyString_Check(output)) {
            Py_DECREF(output);
            PyErr_SetString(PyExc_TypeError, "encoder failed to return bytes");
            return 0;
        }
    }
    size = PyString_GET_SIZE(output);
    data = PyString_AS_STRING(output);
    if (size != strlen(data)) {
        PyErr_SetString(PyExc_TypeError, "embedded NUL character");
        Py_DECREF(output);
        return 0;
    }
    *(PyObject**)addr = output;
    return Py_CLEANUP_SUPPORTED;
}


/* Free's a NULL terminated char** array of C strings. */
static void
_Py_FreeCharPArray(char *const array[])
{
    Py_ssize_t i;
    for (i = 0; array[i] != NULL; ++i) {
        free(array[i]);
    }
    free((void*)array);
}


/*
 * Flatten a sequence of bytes() objects into a C array of
 * NULL terminated string pointers with a NULL char* terminating the array.
 * (ie: an argv or env list)
 *
 * Memory allocated for the returned list is allocated using malloc() and MUST
 * be freed by the caller using a free() loop or _Py_FreeCharPArray().
 */
static char *const *
_PySequence_BytesToCharpArray(PyObject* self)
{
    char **array;
    Py_ssize_t i, argc;
    PyObject *item = NULL;

    argc = PySequence_Size(self);
    if (argc == -1)
        return NULL;

    array = malloc((argc + 1) * sizeof(char *));
    if (array == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    for (i = 0; i < argc; ++i) {
        char *data;
        item = PySequence_GetItem(self, i);
        data = PyString_AsString(item);
        if (data == NULL) {
            /* NULL terminate before freeing. */
            array[i] = NULL;
            goto fail;
        }
        array[i] = strdup(data);
        if (!array[i]) {
            PyErr_NoMemory();
            goto fail;
        }
        Py_DECREF(item);
    }
    array[argc] = NULL;

    return array;

fail:
    Py_XDECREF(item);
    _Py_FreeCharPArray(array);
    return NULL;
}


/* Restore signals that the interpreter has called SIG_IGN on to SIG_DFL.
 *
 * All of the code in this function must only use async-signal-safe functions,
 * listed at `man 7 signal` or
 * http://www.opengroup.org/onlinepubs/009695399/functions/xsh_chap02_04.html.
 */
static void
_Py_RestoreSignals(void)
{
#ifdef SIGPIPE
    PyOS_setsig(SIGPIPE, SIG_DFL);
#endif
#ifdef SIGXFZ
    PyOS_setsig(SIGXFZ, SIG_DFL);
#endif
#ifdef SIGXFSZ
    PyOS_setsig(SIGXFSZ, SIG_DFL);
#endif
}


static int
set_inheritable(int fd, int inheritable)
{
#if defined(HAVE_SYS_IOCTL_H) && defined(FIOCLEX) && defined(FIONCLEX)
    static int ioctl_works = -1;
    int request;
    int err;
#endif
    int flags;
    int res;

#if defined(HAVE_SYS_IOCTL_H) && defined(FIOCLEX) && defined(FIONCLEX)
    if (ioctl_works != 0) {
        /* fast-path: ioctl() only requires one syscall */
        if (inheritable)
            request = FIONCLEX;
        else
            request = FIOCLEX;
        err = ioctl(fd, request, NULL);
        if (!err) {
            ioctl_works = 1;
            return 0;
        }

        if (errno != ENOTTY) {
            return -1;
        }
        else {
            /* Issue #22258: Here, ENOTTY means "Inappropriate ioctl for
               device". The ioctl is declared but not supported by the kernel.
               Remember that ioctl() doesn't work. It is the case on
               Illumos-based OS for example. */
            ioctl_works = 0;
        }
        /* fallback to fcntl() if ioctl() does not work */
    }
#endif

    /* slow-path: fcntl() requires two syscalls */
    flags = fcntl(fd, F_GETFD);
    if (flags < 0) {
        return -1;
    }

    if (inheritable)
        flags &= ~FD_CLOEXEC;
    else
        flags |= FD_CLOEXEC;
    res = fcntl(fd, F_SETFD, flags);
    if (res < 0) {
        return -1;
    }
    return 0;
}
