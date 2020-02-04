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
#include <iomanip>
#include <stdio.h>
#include "tmp.h"

using namespace tmp;

template<typename TL>
struct hierarchy_iterator {
    static_assert(is_typelist<TL>::value, "Not a typelist");
    inline static void exec(void* _p) {
        using target_t = typename pop_front<TL>::type;
        if (auto ptr = static_cast<target_t*>(_p)) {
            printf("%s\n", typeid(typename at<TL, 0>::type).name());
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
    using namespace std;

    using REGISTRY    = typelist<I, C, Z, G, D, F, L, C, I, A, T, B, J, K, H, E, E>;
    using D_ANCESTORS = find_ancestors<REGISTRY, D>::type;
    using K_ANCESTORS = find_ancestors<REGISTRY, K>::type;
    using D_EXPECTED = typelist<A, C, D>;
    using K_EXPECTED = typelist<F, H, J, I, K>;

    static_assert(is_same<D_ANCESTORS, D_EXPECTED>::value, "Ancestor of D test failed");
    static_assert(is_same<K_ANCESTORS, K_EXPECTED>::value, "Ancestor of K test failed");

    D d_instance;
    printf("The hierarchy tree of class D is:\n");
    hierarchy_iterator<D_ANCESTORS>::exec(&d_instance);
    printf("\n\n");

    K k_instance;
    printf("The hierarchy tree of class K is:\n");
    hierarchy_iterator<K_ANCESTORS>::exec(&k_instance);
    printf("\n\n");

    return 0;
}
