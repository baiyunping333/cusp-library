

# Introduction #

This page describes how to develop CUDA applications with CUSP, a C++ template library for sparse matrix computations.  This guide is intended to be accessible, even to developers with limited C++ experience.

# Prerequisites #

[Cusp v0.3.0](http://code.google.com/p/cusp-library/downloads/list) requires [CUDA 4.1](http://www.nvidia.com/object/cuda_get.html) (or newer).  You can confirm that CUDA 4.1 is installed by running `nvcc --version` on the command line.  For example on a Linux system,
```
$ nvcc --version
nvcc: NVIDIA (R) Cuda compiler driver
Copyright (c) 2005-2011 NVIDIA Corporation
Built on Thu_Jan_12_14:41:45_PST_2012
Cuda compilation tools, release 4.1, V0.2.1221
```
outputs the compiler version information.


Since Cusp is a C++ template library there is nothing to "build".  Simply download the newest version of Cusp from [here](http://code.google.com/p/cusp-library/downloads) and extract the contents of each zip file to a directory.  We suggest installing Cusp to the CUDA `include` directory, which is usually
  * `/usr/local/cuda/include/` on a Linux and Mac OSX
  * `C:\CUDA\include\` on a Windows system

If you are unable to install to the CUDA `include` directory, then you can place the files somewhere in your home directory, for example: `/home/nathan/cuda_libraries/cusp/`.

**Warning:** Do not install Cusp to a standard include path like `/usr/local/include/`.  It appears that `nvcc` treats such paths differently than the suggestions above which causes errors like `error: expected primary-expression before ‘<’ token`.


## Simple Example ##

Let's compile a simple program to make sure all the prerequisites are satisfied.  Save the following source code to a file named `version.cu`.
```
#include <thrust/version.h>
#include <cusp/version.h>
#include <iostream>

int main(void)
{
    int cuda_major =  CUDA_VERSION / 1000;
    int cuda_minor = (CUDA_VERSION % 1000) / 10;

    int thrust_major = THRUST_MAJOR_VERSION;
    int thrust_minor = THRUST_MINOR_VERSION;

    int cusp_major = CUSP_MAJOR_VERSION;
    int cusp_minor = CUSP_MINOR_VERSION;

    std::cout << "CUDA   v" << cuda_major   << "." << cuda_minor   << std::endl;
    std::cout << "Thrust v" << thrust_major << "." << thrust_minor << std::endl;
    std::cout << "Cusp   v" << cusp_major   << "." << cusp_minor   << std::endl;

    return 0;
}
```
**Note:** Thrust is now distributed with the CUDA Toolkit, so it is automatically installed to the standard CUDA include path.


Now compile `version.cu` with `nvcc`.  If Cusp was installed to the CUDA `include` directory then the following commands should work.
```
$ nvcc version.cu -o version
$ ls
thrust  version version.cu
$ ./version
CUDA   v4.1
Thrust v1.5
Cusp   v0.3
```

If the `cusp` directory is placed somewhere else, use the `-I` option to tell `nvcc` where to look.  For example, if `cusp` was placed in `/home/nathan/cuda_libraries/` then the following command should be used.
```
$ nvcc version.cu -o version -I /home/nathan/cuda_libraries/
```

## Other Examples ##

Additional Cusp examples are available for [download](http://code.google.com/p/cusp-library/downloads/list) or [online browsing](http://code.google.com/p/cusp-library/source/browse/#hg%2Fexamples).  These examples can be compiled using the same procedure as above.  For instance, the Conjugate Gradient solver example is compiled and run as follows:
```
$ cd examples/Solvers/
$ nvcc -O2 cg.cu -o cg
$ ./cg
Solver will continue until residual norm 0.01 or reaching 100 iterations 
  Iteration Number  | Residual Norm
                0       1.000000e+01
                1       1.414214e+01
                2       1.093707e+01
                3       8.949319e+00
                4       6.190055e+00
                5       3.835189e+00
                6       1.745481e+00
                7       5.963546e-01
                8       2.371134e-01
                9       1.152524e-01
               10       3.134467e-02
               11       1.144415e-02
               12       1.824176e-03
Successfully converged after 12 iterations.
```

# Sparse Matrices #

Cusp natively supports several sparse matrix formats:
  * [Coordinate (COO)](http://docs.cusp-library.googlecode.com/hg/classcusp_1_1coo__matrix.html)
  * [Compressed Sparse Row (CSR)](http://docs.cusp-library.googlecode.com/hg/classcusp_1_1csr__matrix.html)
  * [Diagonal (DIA)](http://docs.cusp-library.googlecode.com/hg/classcusp_1_1dia__matrix.html)
  * [ELL (ELL)](http://docs.cusp-library.googlecode.com/hg/classcusp_1_1ell__matrix.html)
  * [Hybrid (HYB)](http://docs.cusp-library.googlecode.com/hg/classcusp_1_1hyb__matrix.html)

When manipulating matrices it's important to understand the advantages and disadvantages of each format.  Broadly speaking, the DIA and ELL formats are the most efficient for computing sparse matrix-vector products, and therefore are the fastest formats for solving sparse linear systems with iterative methods (e.g. Conjugate Gradients).  The COO and CSR formats are more flexible than DIA and ELL and easier manipulate.  The HYB format is a hybrid combination of the ELL (fast) and COO (flexible) formats and is a good default choice.  Refer to the matrix format [examples](http://code.google.com/p/cusp-library/source/browse/#hg%2Fexamples%2FMatrixFormats) for additional information.

## Format Conversions ##

Cusp makes it easy to transfer data between the host and device and convert between sparse matrix formats.  For example,
```
cusp::csr_matrix<int,float,cusp::host_memory> A(5,8,12);
```
allocates storage for a CSR matrix on the host with 5 rows, 8 columns, and 12 nonzero values.  After initializing the entries of the matrix, we can copy it to the device with a single line of code,
```
cusp::csr_matrix<int,float,cusp::device_memory> B = A;
```
or transparently convert it to another format,
```
cusp::hyb_matrix<int,float,cusp::device_memory> C = A;
```
before copying it to the device.

## Dense Arrays ##

Cusp provides one and two-dimensional [dense arrays](http://docs.cusp-library.googlecode.com/hg/group__array__containers.html).

## File I/O ##

Cusp provides support for reading and writing [MatrixMarket](http://math.nist.gov/MatrixMarket/formats.html#MMformat) files.  Refer to the [documentation](http://docs.cusp-library.googlecode.com/hg/group__matrix__market.html) and [examples](http://code.google.com/p/cusp-library/source/browse/#hg%2Fexamples%2FInputOutput) for usage information.

## Algorithms ##

  * `cusp::multiply` - common interface for sparse matrix-vector multiplication
  * `cusp::transpose` - common interface for computing matrix transposes


# Iterative Solvers #

Cusp provides a variety of [iterative methods](http://en.wikipedia.org/wiki/Iterative_method) for solving sparse linear systems.  Established [Krylov subspace](http://en.wikipedia.org/wiki/Krylov_subspace)
methods are available:
  * [Conjugate-Gradient (CG)](http://docs.cusp-library.googlecode.com/hg/cg_8h.html)
  * [Biconjugate Gradient (BiCG)](http://docs.cusp-library.googlecode.com/hg/bicg_8h.html)
  * [Biconjugate Gradient Stabilized (BiCGstab](http://docs.cusp-library.googlecode.com/hg/bicgstab_8h.html)
  * [Generalized Minimum Residual (GMRES)](http://docs.cusp-library.googlecode.com/hg/gmres_8h.html)
  * [Multi-mass Conjugate-Gradient (CG-M)](http://docs.cusp-library.googlecode.com/hg/cg__m_8h.html)
  * [Multi-mass Biconjugate Gradient stabilized (BiCGstab-M)](http://docs.cusp-library.googlecode.com/hg/bicgstab__m_8h.html)
More detailed examples are available [here](http://code.google.com/p/cusp-library/source/browse/#hg%2Fexamples%2FSolvers).


## Monitors ##

Monitors determine the solver convergence criteria (i.e. when to stop iterating) and report information about the iteration.  Users can provide their own monitors to implement specialized convergence criteria or record information during the course of iteration.  The built-in monitors provided by Cusp are:
  * [`cusp::default\_monitor`](http://docs.cusp-library.googlecode.com/hg/classcusp_1_1default__monitor.html) - applies standard convergence criteria
  * [`cusp::verbose\_monitor`](http://docs.cusp-library.googlecode.com/hg/classcusp_1_1verbose__monitor.html) - prints convergence information during iteration

## Preconditioners ##

[Preconditioners](http://en.wikipedia.org/wiki/Preconditioner) are a way to improve the [rate of convergence](http://en.wikipedia.org/wiki/Rate_of_convergence) of iterative solvers.  The good preconditioner is fast to compute and approximates the inverse of the matrix in some sense. Cusp provides the following preconditioners:

  * [Algebraic Multigrid (AMG) based on Smoothed Aggregation](http://docs.cusp-library.googlecode.com/hg/classcusp_1_1precond_1_1smoothed__aggregation.html)
  * [Approximate Inverse (AINV)](http://docs.cusp-library.googlecode.com/hg/ainv_8h.html)
  * [Diagonal](http://docs.cusp-library.googlecode.com/hg/classcusp_1_1precond_1_1diagonal.html)

## User-Defined Linear Operators ##

Sometimes it is useful to solve a linear system `A*x = b` without converting the matrix `A` into one of Cusp's formats.  For this reason Cusp supports user-defined linear operators that take in a vector `x` and compute the result `y = A * x`.  These black-box operators can be used to interface [matrix-free methods](http://en.wikipedia.org/wiki/Matrix-free_methods) with Cusp's iterative solvers.
  * `cusp::linear_operator` - interface for user-defined linear operators

# Utilities #

  * `cusp::gallery::*` - routines for generating matrix examples
  * `cusp::blas::*` - collection of level-1 BLAS routines used by iterative solvers
  * `cusp::print` - basic support for printing matrix contents
  * `cusp::is_valid_matrix` - check validity of matrix format

# Additional Resources #

This guide only scratches the surface of what you can do with Cusp.  The following resources can help you learn to do more with Thrust or provide assistance when problems arise.

  * Comprehensive [Documentation](Documentation.md) of Cusp's API
  * A list of [Frequently Asked Questions](FrequentlyAskedQuestions.md)
  * Collection of [example](http://code.google.com/p/cusp-library/downloads/list) programs

We strongly encourage users to subscribe to the [cusp-users](http://groups.google.com/group/cusp-users) mailing list.  The mailing list is a great place to seek out help from the Cusp developers and other Cusp users.