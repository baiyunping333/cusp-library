

# Matrix Formats #

Cusp provides the following matrix formats

  * Sparse
    * coo\_matrix - Coordinate
    * csr\_matrix - Compressed Sparse Row
    * dia\_matrix - Diagonal
    * ell\_matrix - ELLPACK/ITPACK
    * hyb\_matrix - Hybrid ELL/COO structure
  * Dense
    * array1d - one-dimensional linear array
    * array2d - two-dimensional array with row and column major format


## Matrix Format Conversion ##

The following format conversions are supported

### Host Conversion Functions ###

  * COO ->
    * CSR
    * Dense
  * CSR ->
    * COO
    * DIA
    * ELL
    * HYB
    * Dense
  * DIA ->
    * CSR
  * ELL ->
    * CSR
  * HYB ->
    * CSR
  * Dense ->
    * COO
    * CSR

### Device Conversion Functions ###

  * COO ->
    * CSR
  * CSR ->
    * COO

# Sparse Matrix Operations #

## Matrix-Vector Multiplication ##

Cusp provides host and device implementations of sparse matrix-vector multiplication (SpMV) for all sparse matrix formats.


# Input/Output #

Cusp provides the following I/O support

  * Reading & Writing
    * MatrixMatrix (only real-valued matrices in sparse format)

# Solvers #

## Linear Systems ##

Cusp provides the following solvers for linear systems

  * Krylov
    * Conjugate Gradient (CG)
    * Biconjugate Gradient Stabilized (BiCGstab)

## Eigenvalue Problems ##

None