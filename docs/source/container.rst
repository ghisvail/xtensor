.. Copyright (c) 2016, Johan Mabille and Sylvain Corlay

   Distributed under the terms of the BSD 3-Clause License.

   The full license is in the file LICENSE, distributed with this software.

Arrays and tensors
==================

Internal memory layout
----------------------

A multi-dimensional array of `xtensor` consists of a contiguous one-dimensional buffer combined with an indexing scheme that maps
unsigned integers to the location of an element in the buffer. The range in which the indices can vary is specified by the
`shape` of the array.

The scheme used to map indices into a location in the buffer is a strided index scheme. In such a scheme, the index ``(i0, ..., in)``
corresponds to the offset ``sum(ik * sk)`` from the beginning of the one-dimensional buffer, where ``(s0, ..., sn)`` are the `strides`
of the array. The strides may be specified when instantiating an array:

.. code::

    #include <vector>
    #include "xtensor/xarray.hpp"
    
    std::vector<size_t> shape = { 3, 2, 4 };
    std::vector<size_t> strides = { 8, 4, 1 };
    xt::xarray<double> a(shape, strides);

However, this requires to carefully compute the strides to avoid buffer overflow when accessing elements of the array. `xtensor` provides
shortcuts that allow not to specify the strides when you instantiate a multi-dimensional array: ``layout::row_major`` and ``layout::column_major``
enum values, which are particular cases of strided schemes. The previous example is equivalent to the following one:

.. code::

    #include <vector>
    #include "xtensor/xarray.hpp"

    std::vector<size_t> shape = { 3, 2, 4 };
    xt::array<double> a(shape, xt::layout::row_major);


If neither strides nor layout is specified when instantiating an array, the ``row_major`` layout is used.

Runtime vs Compile-time dimensionality
--------------------------------------

Two container classes implementing multi-dimensional arrays are provided: ``xarray`` and ``xtensor``.

- ``xarray`` can be reshaped dynamically to any number of dimensions. It is the container that is the most similar to numpy arrays.
- ``xtensor`` has a dimension set at compilation time, which enables many optimizations. For example, shapes and strides
  of ``xtensor`` instances are allocated on the stack instead of the heap.

``xarray`` and ``xtensor`` containers are both ``xexpression`` s and can be involved and mixed in mathematical expressions, assigned to each
other etc... They provide an augmented interface compared to other ``xexpression`` types:

- Each method exposed in ``xexpression`` interface has its non-const counterpart exposed by both ``xarray`` and ``xtensor``.
- ``reshape()`` reshapes the container in place, that is, if the global size of the container doesn't change, no memory allocation occurs.
- ``transpose()`` transposes the container in place, that is, no memory allocation occurs.
- ``strides()`` returns the strides of the container, used to compute the position of an element in the underlying buffer.

Performance
-----------

The dynamic dimensionality of ``xarray`` comes at a cost. Since the dimension is unknown at build time, the sequences holding shape and strides of
``xarray`` instances are heap-allocated, which makes it significantly more expansive than ``xtensor``. Shape and strides of ``xtensor`` are
stack-allocated which makes them more efficient.

More generally, the library implements a ``promote_shape`` mechanism at build time to determine the optimal sequence type to hold the shape of an
expression. The shape type of a broadcasting expression whose members have a dimensionality determined at compile time will have a stack-allocated
shape. If a single member of a broadcasting expression has a dynamic dimension (for example an ``xarray``), it bubbles up to entire broadcasting
expression which will have a heap allocated shape. The same hold for views, broadcast expressions, etc...

Aliasing and temporaries
------------------------

In some cases, an expression should not be directly assigned to a container. Instead, it has to be assigned to a temporary variable before being copied
into the destination container. This occurs when the destination container is involved in the expression and has to be reshaped. This phenomenon is
known as *aliasing*.

To prevent this, `xtensor` assigns the expression to a temporary variable before copying it. In the case of ``xarray``, this results in an extra dynamic memory
allocation and copy.

However, if the left-hand side is not involved in the expression being assigned, no temporary variable should be required. `xtensor` cannot detect such cases
automatically and applies the "temporary variable rule" by default. A mechanism is provided to forcibly prevent usage of a temporary variable:

.. code::

    #include "xtensor/xarray.hpp"
    #include "xtensor/xnoalias.hpp"

    // a, b, and c are xt::arrays previously initialized
    xt::noalias(b) = a + c;
    // Even if b has to be reshaped, a+c will be assigned directly to it
    // No temporary variable will be involved

Example of aliasing
~~~~~~~~~~~~~~~~~~~

The aliasing phenomenon is illustrated in the following example:

.. code::

    #include <vector>
    #include "xtensor/xarray.hpp"

    std::vector<size_t> a_shape = {3, 2, 4};
    xt::xarray<double> a(a_shape);
    
    std::vector<size_t> b_shape = {2, 4};
    xt::xarray<double> b(b_shape);

    b = a + b;
    // b appears on both left-hand and right-hand sides of the statement

In the above example, the shape of ``a + b`` is ``{ 3, 2, 4 }``. Therefore, ``b`` must first be reshaped, which impacts how the right-hand side is computed.

If the values of ``b`` were copied into the new buffer directly without an intermediary variable, then we would have 
``new_b(0, i, j) == old_b(i, j) for (i,j) in [0,1] x [0, 3]``. After the reshape of ``bb``, ``a(0, i, j) + b(0, i, j)`` is assigned to ``b(0, i, j)``, then,
due to broadcasting rules, ``a(1, i, j) + b(0, i, j)`` is assigned to ``b(1, i, j)``. The issue is ``b(0, i, j)`` has been changed by the previous assignment.

