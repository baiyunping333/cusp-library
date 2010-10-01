/*
 *  Copyright 2008-2009 NVIDIA Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


#include <cusp/array1d.h>
#include <cusp/blas.h>
#include <cusp/multiply.h>
#include <cusp/monitor.h>

#include <thrust/copy.h>
#include <thrust/fill.h>
#include <thrust/functional.h>
#include <thrust/transform.h>
#include <thrust/transform_reduce.h>
#include <thrust/inner_product.h>

#include <thrust/iterator/transform_iterator.h>

/*
 * The point of these routines is to solve systems of the type
 *
 * (A+\sigma Id)x = b
 *
 * for a number of different \sigma, iteratively, for sparse A, without
 * additional matrix-vector multiplication.
 *
 * The idea comes from the following paper:
 *     Krylov space solvers for shifted linear systems
 *     B. Jegerlehner
 *     http://arxiv.org/abs/hep-lat/9612014
 *
 * This implementation was contributed by Greg van Anders.
 *
 */

namespace cusp
{
namespace krylov
{

// structs in this namespace do things that are somewhat blas-like, but
// are not usual blas operations (e.g. they aren't all linear in all arguments)
//
// except for KERNEL_VCOPY all of these structs perform operations that
// are specific to CG-M
namespace detail_m
{
  // computes new \zeta
  template <typename ScalarType>
    struct KERNEL_Z
  {
    ScalarType beta_m1;
    ScalarType beta_0;
    ScalarType alpha_0;

    KERNEL_Z(ScalarType _beta_m1, ScalarType _beta_0, ScalarType _alpha_0)
      : beta_m1(_beta_m1), beta_0(_beta_0), alpha_0(_alpha_0)
    {}

    template <typename Tuple>
    __host__ __device__
      void operator()(Tuple t)
    {
      // compute \zeta_1^\sigma
      thrust::get<0>(t)=thrust::get<1>(t)*thrust::get<2>(t)*beta_m1/
                        (beta_0*alpha_0*(thrust::get<2>(t)-thrust::get<1>(t))
                         +beta_m1*thrust::get<2>(t)*(ScalarType(1)-
                                                     beta_0*thrust::get<3>(t)));
    }
  };

  // computes new \beta
  template <typename ScalarType>
    struct KERNEL_B
  {
    ScalarType beta_0;

    KERNEL_B(ScalarType _beta_0) : beta_0(_beta_0)
    {}

    template <typename Tuple>
    __host__ __device__
      void operator()(Tuple t)
    {
      // compute \beta_0^\sigma
      thrust::get<0>(t)=beta_0*thrust::get<1>(t)/thrust::get<2>(t);
      //thrust::get<0>(t)=thrust::get<1>(t);
    }
  };

  // computes new alpha
  template <typename ScalarType>
    struct KERNEL_A
  {
    ScalarType beta_0;
    ScalarType alpha_0;

    // note: only the ratio alpha_0/beta_0 enters in the computation, it might
    // be better just to pass this ratio
    KERNEL_A(ScalarType _beta_0, ScalarType _alpha_0)
      : beta_0(_beta_0), alpha_0(_alpha_0)
    {}

    template <typename Tuple>
    __host__ __device__
      void operator()(Tuple t)
    {
      // compute \alpha_0^\sigma
      thrust::get<0>(t)=alpha_0/beta_0*thrust::get<2>(t)*thrust::get<3>(t)/
                        thrust::get<1>(t);
    }
  };

  // computes new x
  template <typename ScalarType>
    struct KERNEL_X : thrust::binary_function<int, ScalarType, ScalarType>
  {
    int N;
    const ScalarType *beta_0_s;
    const ScalarType *r_0;
    const ScalarType *p_0_s;

    KERNEL_X(int _N, const ScalarType *_beta_0_s,
		    const ScalarType *_p_0_s) : N(_N),
	            beta_0_s(_beta_0_s), p_0_s(_p_0_s)
   {}

    __host__ __device__
      ScalarType operator()(int index, ScalarType val)
    {
      unsigned int N_s = index / N;
      
      // return the transformed result
      return val-beta_0_s[N_s]*p_0_s[index];
    }
  };

  // computes new p
  template <typename ScalarType>
    struct KERNEL_P : thrust::binary_function<int, ScalarType, ScalarType>
  {
    int N;
    const ScalarType *alpha_0_s;
    const ScalarType *z_1_s;
    const ScalarType *r_0;

    KERNEL_P(int _N, const ScalarType *_alpha_0_s,
		    const ScalarType *_z_1_s, const ScalarType *_r_0):
	    N(_N), alpha_0_s(_alpha_0_s), z_1_s(_z_1_s), r_0(_r_0)
    {}

    __host__ __device__
      ScalarType operator()(int index, ScalarType val)
    {
      unsigned int N_s = index / N;
      unsigned int N_i = index % N;
      
      // return the transformed result
      return z_1_s[N_s]*r_0[N_i]+alpha_0_s[N_s]*val;
    }
  };

  // like blas::copy, but copies the same array many times into a larger array
  template <typename ScalarType>
    struct KERNEL_VCOPY : thrust::unary_function<int, ScalarType>
  {
    int N_t;
    const ScalarType *source;

    KERNEL_VCOPY(int _N_t, const ScalarType *_source) :
	    N_t(_N_t), source(_source)
    {}

    __host__ __device__
      ScalarType operator()(int index)
    {
      unsigned int N   = index % N_t;
      return source[N];
    }

  };

} // end namespace detail_m

// Methods in this namespace are all routines that involve using
// thrust::for_each to perform some transformations on arrays of data.
//
// Except for vectorize_copy, these are specific to CG-M.
//
// Each has a version that takes Array inputs, and another that takes iterators
// as input. The CG-M routine only explicitly refers version with Arrays as
// arguments. The Array version calls the iterator version which uses
// a struct from cusp::krylov::detail_m.
namespace trans_m
{
  // compute \zeta_1^\sigma, using iterators
  // uses detail_m::KERNEL_Z
  template <typename InputIterator1, typename InputIterator2,
            typename InputIterator3,
	    typename OutputIterator1,
	    typename ScalarType>
  void compute_z_m(InputIterator1 z_0_s_b, InputIterator1 z_0_s_e,
		InputIterator2 z_m1_s_b, InputIterator3 sig_b,
		OutputIterator1 z_1_s_b,
		ScalarType beta_m1, ScalarType beta_0, ScalarType alpha_0)
  {
    size_t N = z_0_s_e - z_0_s_b;
    thrust::for_each(
    thrust::make_zip_iterator(thrust::make_tuple(z_1_s_b,z_0_s_b,z_m1_s_b,sig_b)),
    thrust::make_zip_iterator(thrust::make_tuple(z_1_s_b,z_0_s_b,z_m1_s_b,sig_b))+N,
    cusp::krylov::detail_m::KERNEL_Z<ScalarType>(beta_m1,beta_0,alpha_0)
    );
  }

  // compute \beta_0^\sigma, using iterators
  // uses detail_m::KERNEL_B
  template <typename InputIterator1, typename InputIterator2,
	    typename OutputIterator1,
	    typename ScalarType>
  void compute_b_m(InputIterator1 z_1_s_b, InputIterator1 z_1_s_e,
		InputIterator2 z_0_s_b, OutputIterator1 beta_0_s_b,
		ScalarType beta_0)
  {
    size_t N = z_1_s_e - z_1_s_b;

    thrust::for_each(
    thrust::make_zip_iterator(thrust::make_tuple(beta_0_s_b,z_1_s_b,z_0_s_b)),
    thrust::make_zip_iterator(thrust::make_tuple(beta_0_s_b,z_1_s_b,z_0_s_b))+N,
    cusp::krylov::detail_m::KERNEL_B<ScalarType>(beta_0)
    );
  }

  // compute \zeta_1^\sigma, using arrays
  template <typename Array1, typename Array2, typename Array3,
            typename Array4, typename ScalarType>
  void compute_z_m(const Array1& z_0_s, const Array2& z_m1_s,
		const Array3& sig, Array4& z_1_s,
		ScalarType beta_m1, ScalarType beta_0, ScalarType alpha_0)
  {
    // sanity checks
    cusp::blas::detail::assert_same_dimensions(z_0_s,z_m1_s,z_1_s);
    cusp::blas::detail::assert_same_dimensions(z_1_s,sig);

    // compute
    cusp::krylov::trans_m::compute_z_m(z_0_s.begin(),z_0_s.end(),
		    z_m1_s.begin(),sig.begin(),z_1_s.begin(),
                    beta_m1,beta_0,alpha_0);

  }

  // \beta_0^\sigma using arrays
  template <typename Array1, typename Array2, typename Array3,
            typename ScalarType>
  void compute_b_m(const Array1& z_1_s, const Array2& z_0_s,
		Array3& beta_0_s, ScalarType beta_0)
  {
    // sanity checks
    cusp::blas::detail::assert_same_dimensions(z_1_s,z_0_s,beta_0_s);

    // compute
    cusp::krylov::trans_m::compute_b_m(z_1_s.begin(),z_1_s.end(),
		    z_0_s.begin(),beta_0_s.begin(),beta_0);
  }

  // compute \alpha_0^\sigma, and swap \zeta_i^\sigma using iterators
  // uses detail_m::KERNEL_A
  template <typename InputIterator1, typename InputIterator2,
            typename InputIterator3, typename OutputIterator,
            typename ScalarType>
  void compute_a_m(InputIterator1 z_0_s_b, InputIterator1 z_0_s_e,
		InputIterator2 z_1_s_b, InputIterator3 beta_0_s_b,
                OutputIterator alpha_0_s_b,
		ScalarType beta_0, ScalarType alpha_0)
  {
    size_t N = z_0_s_e - z_0_s_b;
    thrust::for_each(
    thrust::make_zip_iterator(thrust::make_tuple(alpha_0_s_b,z_0_s_b,z_1_s_b,beta_0_s_b)),
    thrust::make_zip_iterator(thrust::make_tuple(alpha_0_s_b,z_0_s_b,z_1_s_b,beta_0_s_b))+N,
    cusp::krylov::detail_m::KERNEL_A<ScalarType>(beta_0,alpha_0));
  }

  // compute \alpha_0^\sigma, and swap \zeta_i^\sigma using arrays
  template <typename Array1, typename Array2, typename Array3,
            typename Array4, typename ScalarType>
  void compute_a_m(const Array1& z_0_s, const Array2& z_1_s,
                const Array3& beta_0_s, Array4& alpha_0_s,
		ScalarType beta_0, ScalarType alpha_0)
  {
    // sanity checks
    cusp::blas::detail::assert_same_dimensions(z_0_s,z_1_s);
    cusp::blas::detail::assert_same_dimensions(z_0_s,alpha_0_s,beta_0_s);

    // compute
    cusp::krylov::trans_m::compute_a_m(z_0_s.begin(), z_0_s.end(),
		z_1_s.begin(), beta_0_s.begin(), alpha_0_s.begin(),
                beta_0, alpha_0);
  }

  // compute x^\sigma, p^\sigma
  // this is currently done by calling two different kernels... this is likely
  // not optimal
  // uses detail_m::KERNEL_X, detail_m::KERNEL_P
  template <typename Array1, typename Array2, typename Array3,
            typename Array4, typename Array5, typename Array6>
  void compute_xp_m(const Array1& alpha_0_s, const Array2& z_1_s,
                const Array3& beta_0_s, const Array4& r_0,
                Array5& x_0_s, Array6& p_0_s)
  {
    // sanity check
    cusp::blas::detail::assert_same_dimensions(alpha_0_s,z_1_s,beta_0_s);
    cusp::blas::detail::assert_same_dimensions(x_0_s,p_0_s);
    size_t N = r_0.end()-r_0.begin();
    size_t N_s = alpha_0_s.end()-alpha_0_s.begin();
    size_t N_t = x_0_s.end()-x_0_s.begin();
    assert (N_t == N*N_s);

    // counting iterators to pass to thrust::transform
    thrust::counting_iterator<int> counter_1(0);
    thrust::counting_iterator<int> counter_2(0);

    // get raw pointers for passing to kernels
    typedef typename Array1::value_type   ScalarType;
    const ScalarType *raw_ptr_alpha_0_s = thrust::raw_pointer_cast(alpha_0_s.data());
    const ScalarType *raw_ptr_z_1_s     = thrust::raw_pointer_cast(z_1_s.data());
    const ScalarType *raw_ptr_beta_0_s  = thrust::raw_pointer_cast(beta_0_s.data());
    const ScalarType *raw_ptr_r_0       = thrust::raw_pointer_cast(r_0.data());
    const ScalarType *raw_ptr_p_0_s     = thrust::raw_pointer_cast(p_0_s.data());

    // compute new x,p
    // this might be more efficiently done with a single kernel (?)

    // compute x
    thrust::transform(counter_1,counter_1+N_t,x_0_s.begin(),x_0_s.begin(),
    cusp::krylov::detail_m::KERNEL_X<ScalarType>(N,raw_ptr_beta_0_s,raw_ptr_p_0_s));
    // compute p
    thrust::transform(counter_2,counter_2+N_t,p_0_s.begin(),p_0_s.begin(),
    cusp::krylov::detail_m::KERNEL_P<ScalarType>(N,raw_ptr_alpha_0_s,raw_ptr_z_1_s,raw_ptr_r_0));
  }

  // multiple copy of array to another array
  // this is just a vectorization of blas::copy
  // uses detail_m::KERNEL_VCOPY
  template <typename Array1, typename Array2>
    void vectorize_copy(const Array1& source, Array2& dest)
  {
    // sanity check
    size_t N = source.end()-source.begin();
    size_t N_t = dest.end()-dest.begin();
    assert ( N_t%N == 0 );

    // counting iterators to pass to thrust::transform
    thrust::counting_iterator<int> counter(0);

    // pointer to data
    typedef typename Array1::value_type   ScalarType;
    const ScalarType *raw_ptr_source = thrust::raw_pointer_cast(source.data());

    // compute
    thrust::transform(counter,counter+N_t,dest.begin(),
    cusp::krylov::detail_m::KERNEL_VCOPY<ScalarType>(N,raw_ptr_source));

  }

} // end namespace trans_m



// CG-M routine that uses the default monitor to determine completion
template <class LinearOperator,
          class VectorType1,
          class VectorType2,
          class VectorType3>
void cg_m(LinearOperator& A,
          VectorType1& x,
          VectorType2& b,
          VectorType3& sigma)
{
    typedef typename LinearOperator::value_type   ValueType;

    cusp::default_monitor<ValueType> monitor(b);

    return cg_m(A, x, b, sigma, monitor);
}

// CG-M routine that takes a user specified monitor
template <class LinearOperator,
          class VectorType1,
          class VectorType2,
          class VectorType3,
          class Monitor>
void cg_m(LinearOperator& A,
          VectorType1& x,
          VectorType2& b,
          VectorType3& sigma,
          Monitor& monitor)
{
  //
  // This bit is initialization of the solver.
  //

  // shorthand for typenames
  typedef typename LinearOperator::value_type   ValueType;
  typedef typename LinearOperator::memory_space MemorySpace;

  // sanity checking
  const size_t N = A.num_rows;
  const size_t N_t = x.end()-x.begin();
  const size_t test = b.end()-b.begin();
  const size_t N_s = sigma.end()-sigma.begin();

  assert(A.num_rows == A.num_cols);
  assert(N_t == N*N_s);
  assert(N == test);

  //clock_t start = clock();

  // p has data used in computing the soln.
  cusp::array1d<ValueType,MemorySpace> p_0_s(N_t);

  // stores residuals
  cusp::array1d<ValueType,MemorySpace> r_0(N);
  // used in iterates
  cusp::array1d<ValueType,MemorySpace> p_0(N);
  // used in iterates
  cusp::array1d<ValueType,MemorySpace> xx_0(N,ValueType(0));

  // stores parameters used in the iteration
  cusp::array1d<ValueType,MemorySpace> z_m1_s(N_s,ValueType(1));
  cusp::array1d<ValueType,MemorySpace> z_0_s(N_s,ValueType(1));
  cusp::array1d<ValueType,MemorySpace> z_1_s(N_s);

  cusp::array1d<ValueType,MemorySpace> alpha_0_s(N_s,ValueType(0));
  cusp::array1d<ValueType,MemorySpace> beta_0_s(N_s);

  // stores parameters used in the iteration for the undeformed system
  ValueType beta_m1, beta_0(ValueType(1));
  ValueType alpha_0(ValueType(0));
  ValueType alpha_0_inv;

  // stores the value of the matrix-vector product we have to compute
  cusp::array1d<ValueType,MemorySpace> Ap(N);

  // stores the value of the inner product (p,Ap)
  ValueType pAp;

  // store the values of (r_i,r_i) and (r_{i+1},r_{i+1})
  ValueType rsq_0, rsq_1;

  // set up the initial conditions for the iteration
  cusp::blas::copy(b,r_0);
  rsq_1=cusp::blas::dotc(r_0,r_0);

  // set up the intitial guess
  cusp::blas::fill(x.begin(),x.end(),ValueType(0));

  // set up initial value of p_0 and p_0^\sigma
  cusp::krylov::trans_m::vectorize_copy(b,p_0_s);
  cusp::blas::copy(b,p_0);
  
  //
  // Initialization is done. Solve iteratively
  //
  while (!monitor.finished(r_0))
  {
    // recycle iterates
    rsq_0 = rsq_1;
    beta_m1 = beta_0;

    // compute the matrix-vector product Ap
    cusp::multiply(A,p_0,Ap);

    // compute the inner product (p,Ap)
    pAp=cusp::blas::dotc(p_0,Ap);

    // compute \beta_0
    beta_0 = -rsq_0/pAp;

    // compute the new residual
    cusp::blas::axpy(Ap,r_0,beta_0);

    // compute \zeta_1^\sigma
    cusp::krylov::trans_m::compute_z_m(z_0_s, z_m1_s, sigma, z_1_s,
                                      beta_m1, beta_0, alpha_0);
    // compute \beta_0^\sigma
    cusp::krylov::trans_m::compute_b_m(z_1_s, z_0_s, beta_0_s, beta_0);

    // compute \alpha_0
    rsq_1 = cusp::blas::dotc(r_0,r_0);
    alpha_0 = rsq_1/rsq_0;
    alpha_0_inv = rsq_0/rsq_1;
    cusp::blas::axpy(p_0,xx_0,-beta_0);
    cusp::blas::axpy(r_0,p_0,alpha_0_inv);
    cusp::blas::scal(p_0,alpha_0);
    
    // calculate \alpha_0^\sigma
    cusp::krylov::trans_m::compute_a_m(z_0_s, z_1_s, beta_0_s,
                                      alpha_0_s, beta_0, alpha_0);

    // compute x_0^\sigma, p_0^\sigma
    cusp::krylov::trans_m::compute_xp_m(alpha_0_s, z_1_s, beta_0_s, r_0,
                                      x, p_0_s);

    // recycle \zeta_i^\sigma
    cusp::blas::copy(z_0_s,z_m1_s);
    cusp::blas::copy(z_1_s,z_0_s);
    
    ++monitor;

  }// finished iteration

  //cudaThreadSynchronize();

  // MFLOPs excludes BLAS operations
  //double elapsed = ((double) (clock() - start)) / CLOCKS_PER_SEC;
  //double MFLOPs = 2* ((double) i * (double) A.num_entries)/ (1e6 * elapsed);
  //printf("-iteration completed in %lfms  ( > %6.2lf MFLOPs )\n",1000*elapsed, MFLOPs );

  
} // end cg_m

} // end namespace krylov
} // end namespace cusp
