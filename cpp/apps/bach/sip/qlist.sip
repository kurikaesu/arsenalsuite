
// QList<double> is implemented as a Python list of floats.
%MappedType QList<float>
{
%TypeHeaderCode
#include <qlist.h>
%End

%ConvertFromTypeCode
    // Handle no list.
    if (!sipCpp)
        return PyList_New(0);

    // Create the list.
    PyObject *l;

    if ((l = PyList_New(sipCpp->size())) == NULL)
        return NULL;

    // Set the list elements.
    for (int i = 0; i < sipCpp->size(); ++i)
    {
        PyObject *pobj;

        if ((pobj = PyFloat_FromDouble(double(sipCpp->value(i)))) == NULL)
        {
            Py_DECREF(l);

            return NULL;
        }

        PyList_SET_ITEM(l, i, pobj);
    }

    return l;
%End

%ConvertToTypeCode
    // Check the type if that is all that is required.
    if (sipIsErr == NULL)
        return PyList_Check(sipPy);

    // Map None to NULL.
    if (sipPy == Py_None)
    {
        *sipCppPtr = NULL;
        return 0;
    }

    QList<float> *ql = new QList<float>;
 
    for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i)
        ql->append(float(PyFloat_AsDouble(PyList_GET_ITEM(sipPy, i))));
 
    *sipCppPtr = ql;
 
    return sipGetState(sipTransferObj);
%End
};


// QList<bool> is implemented as a Python list of floats.
%MappedType QList<bool>
{
%TypeHeaderCode
#include <qlist.h>
%End

%ConvertFromTypeCode
    // Handle no list.
    if (!sipCpp)
        return PyList_New(0);

    // Create the list.
    PyObject *l;

    if ((l = PyList_New(sipCpp->size())) == NULL)
        return NULL;

    // Set the list elements.
    for (int i = 0; i < sipCpp->size(); ++i)
    {
        PyList_SET_ITEM(l, i, PyBool_FromLong(sipCpp->value(i) ? 1 : 0));
    }

    return l;
%End

%ConvertToTypeCode
    // Check the type if that is all that is required.
    if (sipIsErr == NULL)
        return PyList_Check(sipPy);

    // Map None to NULL.
    if (sipPy == Py_None)
    {
        *sipCppPtr = NULL;
        return 0;
    }

    QList<bool> *ql = new QList<bool>;
 
    for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i)
        ql->append( PyList_GET_ITEM(sipPy, i) == Py_True );
 
    *sipCppPtr = ql;
 
    return sipGetState(sipTransferObj);
%End
};
