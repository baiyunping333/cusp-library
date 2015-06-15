

# General #

## How do I cite Cusp? ##

Refer to the [Citing](Citing.md) page.

# Functionality #
## Does Cusp support dense matrices? ##

Short answer: Yes, but only for a limited set of purposes.

Long answer: Cusp provides an `array2d` container for dense matrices and supports many operations on dense matrices.  However, Cusp is focused on sparse matrix operations and does not subsume the functionality provided by BLAS or LAPACK libraries.  For instance, while the iterative solvers employed by Cusp are efficient for sparse matrices, they can seldom compete with factorization methods on dense matrices.  If your linear system is dense, you will almost certainly obtain better performance by using one of the libraries designed for solving dense linear systems.

  * [Magma](http://icl.cs.utk.edu/magma/index.html) (open source)
  * [CULA Tools](http://www.culatools.com/) (commercial)