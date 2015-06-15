<table align='center' border='0'>
<tr>
<td>
<a href='http://code.google.com/p/cusp-library/downloads/list'>
<img src='http://cusp-library.googlecode.com/svn/trunk/media/cusp_logo.png' />
</a>
</td>

<td width='50'> </td>

<td>
<table align='center' border='0'>
<tr><td align='center'>
<h1> What is Cusp?</h1>
</td></tr>
<tr><td>
Cusp is a library for <b>sparse linear algebra</b> and <b>graph</b> computations on CUDA.  Cusp provides a flexible, <b>high-level</b> interface for manipulating sparse matrices and solving sparse linear systems.  <a href='QuickStartGuide.md'>Get Started</a> with Cusp today!<br>
</td></tr>
</table>
</td>
</tr>
</table>

<h1><a></a>Github<a href='#Github'></a></h1><p><strong> <font color='#FF0000'>We no longer maintain the CUSP project on Google Code. Find the new home of CUSP at <a href='http://cusplibrary.github.com'>cusplibrary.github.com</a></font></strong> </p><p>

<br><br>

<h1>News</h1>
<ul><li><b>CUSP is now host on <a href='http://github.com/cusplibrary/cusplibrary'>Github</a></b>
</li><li>Cusp v0.3.0 has been <a href='http://code.google.com/p/cusp-library/downloads/list'>released</a> with support for CUDA 4.1.  See <a href='http://code.google.com/p/cusp-library/source/browse/CHANGELOG'>CHANGELOG</a> for release information.<br>
</li><li>Cusp v0.2.0 has been <a href='http://code.google.com/p/cusp-library/downloads/list'>released</a>!  See <a href='http://code.google.com/p/cusp-library/source/browse/CHANGELOG'>CHANGELOG</a> for release information.<br>
</li><li>Cusp v0.1.2 has been <a href='http://code.google.com/p/cusp-library/downloads/list'>released</a>!  v0.1.2 contains compatibility fixes for Thrust v1.3.0.<br>
</li><li>Cusp v0.1.1 has been <a href='http://code.google.com/p/cusp-library/downloads/list'>released</a>!  v0.1.1 contains compatibility fixes for CUDA 3.1.<br>
</li><li>Cusp v0.1.0 has been <a href='http://code.google.com/p/cusp-library/downloads/list'>released</a>!.<br>
</li><li>Added <a href='QuickStartGuide.md'>QuickStartGuide</a> page.</li></ul>

<h1>Examples</h1>

The following example loads a matrix from disk, transparently converts the matrix to the highly-efficient HYB format, and transfers the matrix to the GPU device.  The linear system <code>A*x = b</code> is them solved on the device using the Conjugate Gradient method.  A more detailed version of this example is also <a href='http://code.google.com/p/cusp-library/source/browse/examples/Solvers/cg.cu'>available</a>.<br>
<br>
<pre><code>#include &lt;cusp/hyb_matrix.h&gt;<br>
#include &lt;cusp/io/matrix_market.h&gt;<br>
#include &lt;cusp/krylov/cg.h&gt;<br>
<br>
int main(void)<br>
{<br>
    // create an empty sparse matrix structure (HYB format)<br>
    cusp::hyb_matrix&lt;int, float, cusp::device_memory&gt; A;<br>
<br>
    // load a matrix stored in MatrixMarket format<br>
    cusp::io::read_matrix_market_file(A, "5pt_10x10.mtx");<br>
<br>
    // allocate storage for solution (x) and right hand side (b)<br>
    cusp::array1d&lt;float, cusp::device_memory&gt; x(A.num_rows, 0);<br>
    cusp::array1d&lt;float, cusp::device_memory&gt; b(A.num_rows, 1);<br>
<br>
    // solve the linear system A * x = b with the Conjugate Gradient method<br>
    cusp::krylov::cg(A, x, b);<br>
<br>
    return 0;<br>
}<br>
</code></pre>