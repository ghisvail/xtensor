/***************************************************************************
* Copyright (c) 2016, Johan Mabille and Sylvain Corlay                     *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XITERATOR_HPP
#define XITERATOR_HPP

#include <iterator>

#include "xutils.hpp"
#include "xexception.hpp"

namespace xt
{

    /***********************
     * iterator meta utils *
     ***********************/

    namespace detail
    {
        template <class C>
        struct get_iterator_impl
        {
            using type = typename C::iterator;
        };

        template <class C>
        struct get_iterator_impl<const C>
        {
            using type = typename C::const_iterator;
        };
    }

    template <class C>
    using get_iterator = typename detail::get_iterator_impl<C>::type;
 
    namespace detail
    {
        template <class ST>
        struct index_type_impl
        {
            using type = std::vector<typename ST::value_type>;
        };

        template <class V, std::size_t L>
        struct index_type_impl<std::array<V, L>>
        {
            using type = std::array<V, L>;
        };
    }

    template <class C>
    using xindex_type_t = typename detail::index_type_impl<C>::type;
 
    /************
     * xstepper *
     ************/

    template <class C>
    class xstepper
    {

    public:

        using container_type = C;
        using subiterator_type = get_iterator<C>;
        using subiterator_traits = std::iterator_traits<subiterator_type>;
        using value_type = typename subiterator_traits::value_type;
        using reference = typename subiterator_traits::reference;
        using pointer = typename subiterator_traits::pointer;
        using difference_type = typename subiterator_traits::difference_type;
        using size_type = typename container_type::size_type;
        using shape_type = typename container_type::shape_type;

        xstepper() = default;
        xstepper(container_type* c, subiterator_type it, size_type offset) noexcept;

        reference operator*() const;

        void step(size_type dim, size_type n = 1);
        void step_back(size_type dim, size_type n = 1);
        void reset(size_type dim);

        void to_end();

        bool equal(const xstepper& rhs) const;

    private:

        container_type* p_c;
        subiterator_type m_it;
        size_type m_offset;
    };

    template <class C>
    bool operator==(const xstepper<C>& lhs,
                    const xstepper<C>& rhs);

    template <class C>
    bool operator!=(const xstepper<C>& lhs,
                    const xstepper<C>& rhs);

    template <class S, class IT, class ST>
    void increment_stepper(S& stepper,
                           IT& index,
                           const ST& shape);

    /********************
     * xindexed_stepper *
     ********************/

    template <class E, bool is_const = true>
    class xindexed_stepper
    {

    public:

        using self_type = xindexed_stepper<E, is_const>;
        using xexpression_type = std::conditional_t<is_const, const E, E>;

        using value_type = typename xexpression_type::value_type;
        using reference = std::conditional_t<is_const,
                                             typename xexpression_type::const_reference,
                                             typename xexpression_type::reference>;
        using pointer = std::conditional_t<is_const,
                                           typename xexpression_type::const_pointer,
                                           typename xexpression_type::pointer>;
        using size_type = typename xexpression_type::size_type;
        using difference_type = typename xexpression_type::difference_type;

        using shape_type = typename xexpression_type::shape_type;
        using index_type = xindex_type_t<shape_type>;

        xindexed_stepper() = default;
        xindexed_stepper(xexpression_type* e, size_type offset, bool end = false) noexcept;

        reference operator*() const;

        void step(size_type dim, size_type n = 1);
        void step_back(size_type dim, size_type n = 1);
        void reset(size_type dim);

        void to_end();

        bool equal(const self_type& rhs) const;

    private:

        xexpression_type* p_e;
        index_type m_index;
        size_type m_offset;
    };

    template <class C, bool is_const>
    bool operator==(const xindexed_stepper<C, is_const>& lhs,
                    const xindexed_stepper<C, is_const>& rhs);

    template <class C, bool is_const>
    bool operator!=(const xindexed_stepper<C, is_const>& lhs,
                    const xindexed_stepper<C, is_const>& rhs);

    /*************
     * xiterator *
     *************/

    namespace detail
    {
        template <class S>
        class shape_storage
        {

        public:

            using shape_type = S;
            using param_type = const S&;

            shape_storage() = default;
            shape_storage(param_type shape);
            const S& shape() const;

        private:

            S m_shape;
        };

        template <class S>
        class shape_storage<S*>
        {

        public:

            using shape_type = S;
            using param_type = const S*;

            shape_storage(param_type shape = 0);
            const S& shape() const;

        private:

            const S* p_shape;
        };
    }

    template <class It, class S>
    class xiterator : detail::shape_storage<S>
    {

    public:

        using self_type = xiterator<It, S>;

        using subiterator_type = It;
        using value_type = typename subiterator_type::value_type;
        using reference = typename subiterator_type::reference;
        using pointer = typename subiterator_type::pointer;
        using difference_type = typename subiterator_type::difference_type;
        using size_type = typename subiterator_type::size_type;
        using iterator_category = std::forward_iterator_tag;

        using private_base = detail::shape_storage<S>;
        using shape_type = typename private_base::shape_type;
        using shape_param_type = typename private_base::param_type;
        using index_type = xindex_type_t<shape_type>;

        xiterator() = default;
        xiterator(It it, shape_param_type shape);

        self_type& operator++();
        self_type operator++(int);

        reference operator*() const;
        pointer operator->() const;

        bool equal(const xiterator& rhs) const;

    private:

        subiterator_type m_it;
        index_type m_index;
    };

    template <class It, class S>
    bool operator==(const xiterator<It, S>& lhs,
                    const xiterator<It, S>& rhs);

    template <class It, class S>
    bool operator!=(const xiterator<It, S>& lhs,
                    const xiterator<It, S>& rhs);

    /***************************
     * xstepper implementation *
     ***************************/

    template <class C>
    inline xstepper<C>::xstepper(container_type* c, subiterator_type it, size_type offset) noexcept
        : p_c(c), m_it(it), m_offset(offset)
    {
    }

    template <class C>
    inline auto xstepper<C>::operator*() const -> reference
    {
        return *m_it;
    }

    template <class C>
    inline void xstepper<C>::step(size_type dim, size_type n)
    {
        if(dim >= m_offset)
            m_it += n * p_c->strides()[dim - m_offset];
    }

    template <class C>
    inline void xstepper<C>::step_back(size_type dim, size_type n)
    {
        if(dim >= m_offset)
            m_it -= n * p_c->strides()[dim - m_offset];
    }

    template <class C>
    inline void xstepper<C>::reset(size_type dim)
    {
        if(dim >= m_offset)
            m_it -= p_c->backstrides()[dim - m_offset];
    }

    template <class C>
    inline void xstepper<C>::to_end()
    {
        m_it = p_c->end();
    }

    template <class C>
    inline bool xstepper<C>::equal(const xstepper& rhs) const
    {
        return p_c == rhs.p_c && m_it == rhs.m_it && m_offset == rhs.m_offset;
    }

    template <class C>
    inline bool operator==(const xstepper<C>& lhs,
                           const xstepper<C>& rhs)
    {
        return lhs.equal(rhs);
    }

    template <class C>
    inline bool operator!=(const xstepper<C>& lhs,
                           const xstepper<C>& rhs)
    {
        return !(lhs.equal(rhs));
    }

    template <class S, class IT, class ST>
    void increment_stepper(S& stepper,
                           IT& index,
                           const ST& shape)
    {
        using size_type = typename S::size_type;
        size_type i = index.size();
        while(i != 0)
        {
            --i;
            if(++index[i] != shape[i])
            {
                stepper.step(i);
                return;
            }
            else if(i != 0)
            {
                index[i] = 0;
                stepper.reset(i);
            }
        }
        if(i == 0)
        {
            stepper.to_end();
        }
    }

    /***********************************
     * xindexed_stepper implementation *
     ***********************************/
    
    template <class C, bool is_const>
    inline xindexed_stepper<C, is_const>::xindexed_stepper(xexpression_type* e, size_type offset, bool end) noexcept
        : p_e(e), m_index(make_sequence<index_type>(e->shape().size(), size_type(0))), m_offset(offset)
    {
        if (end)
            to_end();
    }

    template <class C, bool is_const>
    inline auto xindexed_stepper<C, is_const>::operator*() const -> reference
    {
        return p_e->element(m_index.cbegin(), m_index.cend());
    }

    template <class C, bool is_const>
    inline void xindexed_stepper<C, is_const>::step(size_type dim, size_type n)
    {
        if (dim >= m_offset)
            m_index[dim - m_offset] += n;
    }

    template <class C, bool is_const>
    inline void xindexed_stepper<C, is_const>::step_back(size_type dim, size_type n)
    {
        if (dim >= m_offset)
            m_index[dim - m_offset] -= n;
    }

    template <class C, bool is_const>
    inline void xindexed_stepper<C, is_const>::reset(size_type dim)
    {
        if (dim >= m_offset)
            m_index[dim - m_offset] = 0;
    }

    template <class C, bool is_const>
    inline void xindexed_stepper<C, is_const>::to_end()
    {
        m_index = p_e->shape();
    }

    template <class C, bool is_const>
    inline bool xindexed_stepper<C, is_const>::equal(const self_type& rhs) const
    {
        return p_e == rhs.p_e && m_index == rhs.m_index && m_offset == rhs.m_offset;
    }

    template <class C, bool is_const>
    inline bool operator==(const xindexed_stepper<C, is_const>& lhs,
                           const xindexed_stepper<C, is_const>& rhs)
    {
        return lhs.equal(rhs);
    }

    template <class C, bool is_const>
    inline bool operator!=(const xindexed_stepper<C, is_const>& lhs,
                           const xindexed_stepper<C, is_const>& rhs)
    {
        return !lhs.equal(rhs);
    }

    /****************************
     * xiterator implementation *
     ****************************/

    namespace detail
    {
        template <class S>
        inline shape_storage<S>::shape_storage(param_type shape)
            : m_shape(shape)
        {
        }

        template <class S>
        inline const S& shape_storage<S>::shape() const
        {
            return m_shape;
        }

        template <class S>
        inline shape_storage<S*>::shape_storage(param_type shape)
            : p_shape(shape)
        {
        }

        template <class S>
        inline const S& shape_storage<S*>::shape() const
        {
            return *p_shape;
        }
    }

    template <class It, class S>
    inline xiterator<It, S>::xiterator(It it, shape_param_type shape)
        : private_base(shape), m_it(it),
          m_index(make_sequence<index_type>(this->shape().size(), size_type(0)))
    {
    }

    template <class It, class S>
    inline auto xiterator<It, S>::operator++() -> self_type&
    {
        increment_stepper(m_it, m_index, this->shape());
        return *this;
    }

    template <class It, class S>
    inline auto xiterator<It, S>::operator++(int) -> self_type
    {
        self_type tmp(*this);
        ++(*this);
        return tmp;
    }

    template <class It, class S>
    inline auto xiterator<It, S>::operator*() const -> reference
    {
        return *m_it;
    }

    template <class It, class S>
    inline auto xiterator<It, S>::operator->() const -> pointer
    {
        return &(*m_it);
    }

    template <class It, class S>
    inline bool xiterator<It, S>::equal(const xiterator& rhs) const
    {
        return m_it == rhs.m_it && this->shape() == rhs.shape();
    }

    template <class It, class S>
    inline bool operator==(const xiterator<It, S>& lhs,
                           const xiterator<It, S>& rhs)
    {
        return lhs.equal(rhs);
    }

    template <class It, class S>
    inline bool operator!=(const xiterator<It, S>& lhs,
                           const xiterator<It, S>& rhs)
    {
        return !(lhs.equal(rhs));
    }
}

#endif
