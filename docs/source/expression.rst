.. Copyright (c) 2016, Johan Mabille and Sylvain Corlay

   Distributed under the terms of the BSD 3-Clause License.

   The full license is in the file LICENSE, distributed with this software.

Expressions and lazy evaluation
===============================

`xtensor` is more than an N-dimensional array library: it is an expression engine that allows numerical computation on any object implementing the expression interface.
These objects can be in-memory containers such as ``xarray<T>`` and ``xtensor<T>``, but can also be backed by a database or a representation on the file system. This
also enables creating adaptors as expressions for other data structures.

Expressions
-----------

Assume ``x``, ``y`` and ``z`` are arrays of *compatible shapes* (we'll come back to that later), the return type of an expression such as ``x + y * sin(z)`` is **not an array**.
The result is an ``xexpression`` which offers the same interface as an N-dimensional array but does not hold any value. Such expressions can be plugged into others to build
more complex expressions:

.. code::

    auto f = x + y * sin(z);
    auto f2 = w + 2 * cos(f);

The expression engine avoids the evaluation of intermediate results and their storage in temporary arrays, so you can achieve the same performance as if you had written
a simple loop. Assuming ``x``, ``y`` and ``z`` are one-dimensional arrays of length ``n``, 

.. code::

    xt::array<double> res = x + y * sin(z)
   
will produce quite the same assembly as the following loop:

.. code::

    xt::array<double> res(n);
    for(size_t i = 0; i < n; ++i)
    {
        res(i) = x(i) + y(i) * sin(z(i));
    }

Lazy evaluation
---------------

An expression such as ``x + y * sin(z)`` does not hold the result. **Values are only computed upon access or when the expression is assigned to a container**. This
allows to operate symbolically on very large arrays and only compute the result for the indices of interest:

.. code::

    // Assume x and y are arrays each containing 1 000 000 objects
    auto f = cos(x) + sin(y);

    double first_res = f(1200);
    double second_res = f(2500);
    // Only two values have been computed

That means if you use the same expression in two assign statements, the computation of the expression will be done twice. Depending on the complexity of the computation
and the size of the data, it might be convenient to store the result of the expression in a temporary variable:

.. code::

    // Assume x and y are small arrays
    xt::array<double> tmp = cos(x) + sin(y);
    xt::array<double> res1 = tmp + 2 * x;
    xt::array<double> res2 = tmp - 2 * x;

Broadcasting
------------

The number of dimensions of an ``xexpression`` and the sizes of these dimensions are provided by the ``shape()`` method, which returns a sequence of unsigned integers
specifying the size of each dimension. We can operate on expressions of different shapes of dimensions in a elementwise fashion. Broadcasting rules of `xtensor` are
similar to those of Numpy_ and libdynd_.

In an operation involving two arrays of different dimensions, the array with the lesser dimensions is broadcast across the leading dimensions of the other.
For example, if ``A`` has shape ``(2, 3)``, and ``B`` has shape ``(4, 2, 3)``, the result of a broadcasted operation with ``A`` and ``B`` has shape ``(4, 2, 3)``.

.. code::

       (2, 3) # A
    (4, 2, 3) # B
    ---------
    (4, 2, 3) # Result

The same rule holds for scalars, which are handled as 0-D expressions. If `A` is a scalar, the equation becomes:

.. code::

           () # A
    (4, 2, 3) # B
    ---------
    (4, 2, 3) # Result

If matched up dimensions of two input arrays are different, and one of them has size ``1``, it is broadcast to match the size of the other. Let's say B has the shape ``(4, 2, 1)``
in the previous example, so the broadcasting happens as follows:

.. code::

       (2, 3) # A
    (4, 2, 1) # B
    ---------
    (4, 2, 3) # Result

Expression interface
--------------------

All ``xexpression`` s in `xtensor` provide at least the following interface:

Shape
~~~~~

- ``dimension()`` returns the number of dimension of the expression.
- ``shape()`` returns the shape of the expression.

.. code::

    #include <vector>
    #include "xtensor/xarray.hpp"

    std::vector<size_t> shape = {3, 2, 4};
    xt::xarray<double> a(shape);
    size_t d = a.dimension();
    const std::vector<size_t>& s = a.shape();
    bool res = (d == shape.size()) && (s == shape);
    // => res = true

Element access
~~~~~~~~~~~~~~

- ``operator()`` is an access operator which can take multiple integral arguments of none.
- ``operator[]`` has two overloads: one that takes a single integral argument and is equivalent to the call of ``operator()`` with one argument, and one with a single
multi-index argument, which can be of size determined at runtime. This operator also supports braced initializer arguments.
- ``element()`` is an access operator which takes a pair of iterators on a container of indices.

.. code::

    #include <vector>
    #inclde "xtensor/xarray.hpp"

    // xt::xarray<double> a = ...
    std::vector<size_t> index = {1, 1, 1};
    double v1 = a(1, 1, 1);
    double v2 = a[index],
    double v3 = a.element(index.begin(), index.end());
    // => v1 = v2 = v3

Iterators
~~~~~~~~~

- ``xbegin()`` and ``xend()`` return instances of ``xiterator`` which can be used to iterate over all the elements of the expression. The order of the iteration is ``row-major``
  in that the index of the last dimension is incremented first. This iterator pair permits to use algorithms of the STL with ``xexpression`` as if they were simple containers.
- ``xbegin(shape)`` and ``xend(shape)`` are similar but take a *broadcasting shape* as an argument. Elements are iterated upon in a row-major way, but certain dimensions are
  repeated to match the provided shape as per the rules described above. For an expression ``e``, ``e.xbegin(e.shape())`` and ``e.begin()`` are equivalent.
- ``begin()`` and ``end()`` return iterators on the buffer containing the elements of the ``xexpression`` when it is an in-memory container. Otherwise, they are similar
  to ``xbegin()`` and ``xend()``. For in-memory containers, the iteration is done directly on the buffer and may be faster than the one provided by ``xbegin()`` / ``xend()`` .

.. _NumPy: http://www.numpy.org
.. _libdynd: http://libdynd.org
