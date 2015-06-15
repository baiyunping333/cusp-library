These instructions assume that [Thrust](http://code.google.com/p/thrust/) is already installed in your system.

Checkout Cusp repository
```
$ hg clone https://cusp-library.googlecode.com/hg/ cusp
```

Build SpMV driver (with Scons)
```
$ cd cusp/performance/spmv
$ scons arch=sm_20 
```
or
```
$ nvcc -O2 -o spmv spmv.cu -arch=sm_20 -I../../
$ nvcc -O2 -o spmv spmv.cu -arch=sm_20 -I../../ -I/path/to/thrust/
```

Run the benchmark driver on the default matrix (auto-generated)
```
$ ./spmv
$ ./spmv --help
```

Download sparse matrix collection
```
$ http://www.nvidia.com/content/NV_Research/matrices.zip
$ unzip matrices.zip
```

Run the benchmark driver on user-defined matrix
```
./spmv pwtk.mtx
```

Run the benchmark script on all user-defined matrices
```
$ cd scripts
$ gedit benchmark.py (edit paths to files)
$ python benchmark.py
```
benchmark output is stored in .csv files.