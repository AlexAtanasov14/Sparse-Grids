// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

%include "base/src/sgpp/globaldef.hpp"


namespace SGPP
{
namespace base
{

%apply SGPP::float_t *OUTPUT { SGPP::float_t* min, SGPP::float_t* max };
%apply std::string *OUTPUT { std::string& text };
%rename(__str__) DataMatrix::toString const;

//%rename(__getitem__) DataMatrix::get(size_t row, size_t col) const;
//%rename(__setitem__) DataMatrix::set(int i, SGPP::float_t value);
//%rename(assign) DataMatrix::operator=;
//%rename(__len__) DataMatrix::getSize const;



class DataMatrix
{
public:

// typemap allowing to pass sequence of numbers to constructor
%typemap(in) (SGPP::float_t *input, int nrows, int ncols)
{
  if (!PySequence_Check($input)) {
    PyErr_SetString(PyExc_ValueError, "Expected a sequence");
    return NULL;
  }
  $2 = PySequence_Size($input);
  $3 = 0;
  for (int i = 0; i < $2; i++) {
    PyObject *row = PySequence_GetItem($input,i);
    if (!PySequence_Check(row)) {
      PyErr_SetString(PyExc_ValueError, "Expected a sequence of sequences");
      return NULL;
    }
    if ($3 == 0) {
      $3 = PySequence_Size(row);
      $1 = (SGPP::float_t *) malloc(sizeof(SGPP::float_t)*$2*$3);
    }
    if ($3 != PySequence_Size(row)) {
      PyErr_SetString(PyExc_ValueError, "Row dimensions do not match");
      return NULL;
    }
    else {
      for (int j = 0; j < $3; j++) {
  PyObject *o = PySequence_GetItem(row,j);
  if (PyNumber_Check(o)) {
    $1[i*$3+j] = (SGPP::float_t) PyFloat_AsDouble(o);
  } else {
    PyErr_SetString(PyExc_ValueError,"Sequence elements must be numbers");      
    return NULL;
  }
      }
    }
  }
}
%typemap(freearg) (SGPP::float_t *input, int nrows, int ncols)
{
  if ($1) free($1);
}
%typecheck(SWIG_TYPECHECK_FLOAT) (SGPP::float_t *input, int nrows, int ncols)
{
$1 = PySequence_Check($input) ? 1 : 0;
}

    // Constructors
    DataMatrix(size_t nrows, size_t ncols);
    DataMatrix(size_t nrows, size_t ncols, float_t value);
    DataMatrix(const DataMatrix& matr);
    DataMatrix(SGPP::float_t* input, int nrows, int ncols);

    void resize(size_t size);
    void resizeZero(size_t nrows);
    void transpose();

    void addSize(size_t inc_nrows);
    size_t appendRow();
    void setAll(SGPP::float_t value);
    void copyFrom(const DataMatrix& matr);

    SGPP::float_t get(size_t row, size_t col) const;
    void set(size_t row, size_t col, SGPP::float_t value);

//    %extend {
//          SGPP::float_t getxy(int* INPUT) {
//            return INPUT[0]*INPUT[1];
//      };
//    }     

    void getRow(size_t row, DataVector& vec) const;
    void setRow(size_t row, const DataVector& vec);
    void getColumn(size_t col, DataVector& vec) const;
    void setColumn(size_t col, const DataVector& vec);
    SGPP::float_t* getPointer();

    void add(DataMatrix& matr);
    void sub(DataMatrix& matr);
    void componentwise_mult(DataMatrix& matr);
    void componentwise_div(DataMatrix& matr);
    void mult(SGPP::float_t scalar);
    void sqr();
    void sqrt();
    void abs();
    SGPP::float_t sum() const;

    size_t getSize() const;
    size_t getUnused() const;
    size_t getNumberNonZero() const;
    size_t getNrows() const;
    size_t getNcols() const;
    size_t getInc() const;
    void setInc(size_t inc_rows);

    void normalizeDimension(size_t d);
    void normalizeDimension(size_t d, SGPP::float_t border);

    SGPP::float_t min(size_t col) const;
    SGPP::float_t min() const;
    SGPP::float_t max(size_t col) const;
    SGPP::float_t max() const;
    void minmax(size_t col, SGPP::float_t* min, SGPP::float_t* max) const;
    void minmax(SGPP::float_t* min, SGPP::float_t* max) const;

//    void toString(std::string& text) const; // otherwise overloaded duplicate
    std::string toString() const;
    
    %extend {
    // Create a ndarray view from the DataMatrix data
    // an alternative approach using ARGOUTVIEW will fail since it does not allow to do a proper memory management
    PyObject* __array(PyObject* datavector){
        //Get the data and number of entries
      SGPP::float_t *vec = $self->getPointer();
      int rows = $self->getNrows();
      int cols = $self->getNcols();

      npy_intp dims[2] = {rows,cols};
      
      // Create a ndarray with data from vec
      PyObject* arr = PyArray_SimpleNewFromData(2,dims, NPY_DOUBLE, vec);
      
      // Let the array base point on the original data, free_array is a additional destructor for our ndarray, 
      // since we need to DECREF DataVector object
      PyObject* base = PyCObject_FromVoidPtrAndDesc((void*)vec, (void*)datavector, free_array);
      PyArray_BASE(arr) = base;
      
      // Increase the number of references to PyObject DataMatrix, after the object the variable is reinitialized or deleted the object
      // will still be on the heap, if the reference counter is positive.
      Py_INCREF(datavector);
      
      return arr;
    }
     %pythoncode
     {
        def array(self):   
          return self.__array(self)
     }
  }

};

}
}



//// Copyright (C) 2008-today The SG++ project
//// This file is part of the SG++ project. For conditions of distribution and
//// use, please see the copyright notice provided with SG++ or at
//// sgpp.sparsegrids.org
//
//
//%include "base/src/sgpp/globaldef.hpp"
//
//namespace SGPP
//{
//namespace base
//{
//
//%apply SGPP::float_t *OUTPUT { SGPP::float_t* min, SGPP::float_t* max };
//%apply std::string *OUTPUT { std::string& text };
//%rename(__str__) DataMatrix::toString;
//
////%rename(__getitem__) DataMatrix::get(size_t row, size_t col) const;
////%rename(__setitem__) DataMatrix::set(int i, SGPP::float_t value);
////%rename(assign) DataMatrix::operator=;
////%rename(__len__) DataMatrix::getSize;
//
//%ignore DataMatrix::operator=;
//%ignore DataMatrix::operator[];
//%ignore DataMatrix::toString(std::string& text);
//
//// typemap allowing to pass sequence of numbers to constructor
//%typemap(in) (SGPP::float_t *input, int nrows, int ncols)
//{
//  if (!PySequence_Check($input)) {
//    PyErr_SetString(PyExc_ValueError, "Expected a sequence");
//    return NULL;
//  }
//  $2 = PySequence_Size($input);
//  $3 = 0;
//  for (int i = 0; i < $2; i++) {
//    PyObject *row = PySequence_GetItem($input,i);
//    if (!PySequence_Check(row)) {
//      PyErr_SetString(PyExc_ValueError, "Expected a sequence of sequences");
//      return NULL;
//    }
//    if ($3 == 0) {
//      $3 = PySequence_Size(row);
//      $1 = (SGPP::float_t *) malloc(sizeof(SGPP::float_t)*$2*$3);
//    }
//    if ($3 != PySequence_Size(row)) {
//      PyErr_SetString(PyExc_ValueError, "Row dimensions do not match");
//      return NULL;
//    }
//    else {
//      for (int j = 0; j < $3; j++) {
//  PyObject *o = PySequence_GetItem(row,j);
//  if (PyNumber_Check(o)) {
//    $1[i*$3+j] = (SGPP::float_t) PyFloat_AsDouble(o);
//  } else {
//    PyErr_SetString(PyExc_ValueError,"Sequence elements must be numbers");      
//    return NULL;
//  }
//      }
//    }
//  }
//}
//%typemap(freearg) (SGPP::float_t *input, int nrows, int ncols)
//{
//  if ($1) free($1);
//}
//%typecheck(SWIG_TYPECHECK_FLOAT) (SGPP::float_t *input, int nrows, int ncols)
//{
//$1 = PySequence_Check($input) ? 1 : 0;
//}
//
//%extend DataMatrix {
//  // Create a ndarray view from the DataMatrix data
//  // an alternative approach using ARGOUTVIEW will fail since it does not allow to do a proper memory management
//  PyObject* __array(PyObject* datavector){
//      //Get the data and number of entries
//    SGPP::float_t *vec = $self->getPointer();
//    int rows = $self->getNrows();
//    int cols = $self->getNcols();
//
//    npy_intp dims[2] = {rows,cols};
//    
//    // Create a ndarray with data from vec
//    PyObject* arr = PyArray_SimpleNewFromData(2,dims, NPY_DOUBLE, vec);
//    
//    // Let the array base point on the original data, free_array is a additional destructor for our ndarray, 
//    // since we need to DECREF DataVector object
//    PyObject* base = PyCObject_FromVoidPtrAndDesc((void*)vec, (void*)datavector, free_array);
//    PyArray_BASE(arr) = base;
//    
//    // Increase the number of references to PyObject DataMatrix, after the object the variable is reinitialized or deleted the object
//    // will still be on the heap, if the reference counter is positive.
//    Py_INCREF(datavector);
//    
//    return arr;
//  }
//   %pythoncode
//   {
//      def array(self):   
//        return self.__array(self)
//   }
//}
//
//}
//}
//%include "base/src/sgpp/base/datatypes/DataMatrix.hpp"
