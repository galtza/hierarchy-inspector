/*
    MIT License

    Copyright (c) 2019-2020 Raúl Ramos

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#pragma once

#define VERSION "1.0"

namespace tmp {

    // type list
    template<typename...TS>
    struct typelist {
        static constexpr auto size = sizeof...(TS);
    };

    template<typename T>
    struct is_typelist : std::false_type { };

    template<typename...TS>
    struct is_typelist<typelist<TS...>> : std::true_type { };

    // basic operations
    template<typename T, typename TL> struct push_back;
    template<typename T, typename TL> struct push_front;
    template<typename TL>             struct pop_front;
    template<typename TL, size_t I>   struct at;

    template<typename T, typename...TS>
    struct push_back<T, typelist<TS...>> {
        using type = typelist<TS..., T>;
    };

    template<typename T, typename...TS>
    struct push_front<T, typelist<TS...>> {
        using type = typelist<T, TS...>;
    };

    template<typename T, typename...TS>
    struct pop_front<typelist<T, TS...>> {
        using type = typelist<TS...>;
    };

    template <typename T, typename...TS>
    struct at<typelist<T, TS...>, 0> {
        using type = T;
    };

    template <typename T, typename...TS, size_t I>
    struct at<typelist<T, TS...>, I> {
        static_assert(I < (1 + sizeof...(TS)), "Out of bounds access");
        using type = typename at<typelist<TS...>, I - 1>::type;
    };

    // 'filter'
    template<typename TL, template<typename>class PRED>
    struct filter;

    template <template<typename>class PRED>
    struct filter<typelist<>, PRED> {
        using type = typelist<>;
    };

    template <typename T, typename...TS, template<typename>class PRED>
    struct filter<typelist<T, TS...>, PRED> {
        using remaining = typename filter<typelist<TS...>, PRED>::type;
        using type = typename std::conditional<
            PRED<T>::value,
            typename push_front<T, remaining>::type,
            remaining
        >::type;
    };

    // 'max' given a template binary predicate
    template<typename TL, template<typename,typename>class PRED>
    struct max;

    template <typename T, template<typename,typename>class PRED>
    struct max<typelist<T>, PRED> {
        using type = T;
    };

    template <typename ...TS, template<typename,typename>class PRED>
    struct max<typelist<TS...>, PRED> {
        using first = typename at<typelist<TS...>, 0>::type;
        using remaining_max = typename max<typename pop_front<typelist<TS...>>::type, PRED>::type;
        using type  = typename std::conditional<
            PRED<first, remaining_max>::value,
            first, remaining_max
        >::type;
    };

    // 'find_ancestors'
    namespace impl {

        template<typename SRCLIST, typename DESTLIST>
        struct find_ancestors {

            template<typename B>
            using negation = typename std::integral_constant<bool, !bool(B::value)>::type;

            template<typename T, typename U>
            using cmp = typename std::is_base_of<T, U>::type;
            using most_ancient = typename max<SRCLIST, cmp>::type;

            template<typename T>
            using not_most_ancient = typename negation<std::is_same<most_ancient, T>>::type;

            using type = typename find_ancestors<
                typename filter<SRCLIST, not_most_ancient>::type,
                typename push_back<most_ancient, DESTLIST>::type
            >::type;

        };

        template<typename DESTLIST>
        struct find_ancestors<typelist<>, DESTLIST> {
            using type = DESTLIST;
        };

    }

    template<typename TL, typename T>
    struct find_ancestors {
        static_assert(is_typelist<TL>::value, "The first parameter is not a typelist");

        template<typename U>
        using base_of_T = typename std::is_base_of<U, T>::type;
        using src_list = typename filter<TL, base_of_T>::type;
        using type = typename impl::find_ancestors<
            src_list,
            typelist<>
        >::type;
    };

}

