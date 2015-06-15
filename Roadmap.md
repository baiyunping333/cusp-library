# Milestone 0.1 #
  * Sparse matrix support
    * COO, CSR, DIA, ELL, HYB
    * Transparent matrix format conversions
    * Common interface to SpMV
    * Matrix transpose
    * Matrix-Matrix multiplication (preliminary)
  * Iterative method support
    * CG, BiCGstab
    * Diagonal preconditioner
    * User-defined linear operators, preconditioners
    * User-defined monitors
  * File I/O support
    * MatrixMarket format

# Milestone 0.x #

  * Enhanced Sparse Matrix-Vector Multiplication
    * Currently we implement only (y = A\*x), but we also want (y = alpha\*A\*x + beta\*y).
    * Generalized SpMV with arbitrary functors instead of + and `*`
    * Implement Block CSR SpMV
  * Sparse matrix arithmetic
    * Provide host and device implementations of operations such as
      * matrix-matrix multiplication: A x B
      * matrix-matrix addition: A + B
  * Preconditioners
    * Algebraic Multigrid (Smoothed-Aggregation)
    * Approximate Inverse