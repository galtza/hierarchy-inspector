#include <iostream>
#include <iomanip>

namespace tmp {

    // type list
    template<typename...>
    struct typelist {
    };

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

        template<typename U>
        using base_of_T = typename std::is_base_of<U, T>::type;
        using src_list = typename filter<TL, base_of_T>::type;
        using type = typename impl::find_ancestors<
            src_list,
            typelist<>
        >::type;
    };

}

using namespace tmp;

template<typename TL>
struct hierarchy_iterator {
    inline static void exec(void* _p) {
        using target_t = typename pop_front<TL>::type;
        if (auto ptr = static_cast<target_t*>(_p)) {
            std::cout << "base = " << typeid(typename at<TL, 0>::type).name() << "\n";
            hierarchy_iterator<target_t>::exec(_p);
        }
    }
};

template<>
struct hierarchy_iterator<typelist<>> {
    inline static void exec(void*) {
    }
};

/*
=================================================================
                                    F
                                   / \
     A                            H   \
    / \                          / \   \
   B   C                        I   J   G
  /   / \                        \ /   / \
 T   D   E                        K   L   Z
================================================================= */
class A { };                   class F { };
class B : public A { };        class G : public F { };
class C : public A { };        class L : public G { };
class T : public B { };        class Z : public G { };
class D : public C { };        class H : public F { };
class E : public C { };        class I : public H { };
                               class J : public H { };
                               class K : public I, public J { };
// ==============================================================

int main()
{
    using REGISTRY                = typelist<I, C, Z, G, D, F, L, C, I, A, T, B, J, K, H, E, E>;
    using ANCESTORS_OF_D          = find_ancestors<REGISTRY, D>::type;
    using ANCESTORS_OF_K          = find_ancestors<REGISTRY, K>::type;
    using EXPECTED_ANCESTORS_OF_D = typelist<A, C, D>;
    using EXPECTED_ANCESTORS_OF_K = typelist<F, H, J, I, K>;

    static_assert(std::is_same<ANCESTORS_OF_D, EXPECTED_ANCESTORS_OF_D>::value, "Ancestor of D test failed");
    static_assert(std::is_same<ANCESTORS_OF_K, EXPECTED_ANCESTORS_OF_K>::value, "Ancestor of K test failed");

    auto banner = [] {
        std::cout << std::setfill('=') << std::setw(80) << "\n";
    };

    banner();

    auto d_instance = D();
    hierarchy_iterator<ANCESTORS_OF_D>::exec(&d_instance);

    banner();

    auto k_instance = K();
    hierarchy_iterator<ANCESTORS_OF_K>::exec(&k_instance);

    banner();

    return 0;
}
