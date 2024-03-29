> Level Intermediate
# Inspect a type hierarchy at compile-time
Most C++ programmers know something about **metaprogramming**. Some hate it, some love it, some fear it, but probably most of us just do not know enough. Like any other tool, metaprogramming can be either a benefit or a hazard, we just have to choose wisely the right tools for the right problems.

This article is a brief introduction to metaprogramming focused on a real-world example. Essentially, we will learn how to represent and manipulate lists of types, by using some well known c++ features like variadic templates.

However, It **is not** intended to be a comprehensive description of C++11 or metaprogramming, **it is just about overcoming the fear of metaprogramming**, so you can still hate it, love it or keep on fearing it, but with a bit more knowledge.

Our problem can be formulated as:

> Given a list of arbitrary types TL and an arbitrary type T, build a template meta-function that computes an ordered list of ancestors of T included in TL. Also, create a function that iterates over the types, performing an arbitrary operation.

One of the usages of this technique is **serialization**. By having access to the list of ordered ancestors, we can have access to all the members in a parent-to-child order. In addition, all of this is **without having to alter the original code**.

Throughout the article, we will be using the following two class hierarchies *registered* in a particular order:

```
                   F                                                    
      A           / \                                                        
     / \         H   \                                                        
    B   C       / \   \         Reg order: I, C, Z, G, D, F, L, C, I, A, T, B, J, K, H, E, E
   /   / \     I   J   G                                                       
  T   D   E     \ /   / \
                 K   L   Z
```

Finally, all the code presented in this article and the corresponding code file is C++11 compliant.

## Metaprogramming

Firstly, let us **define** what metaprogramming is. [Wikipedia](https://en.wikipedia.org/wiki/Metaprogramming) gives a very good definition of it:
> *"Metaprogramming is a programming technique in which computer programs have the ability to treat programs as their data. It means that a program can be designed to read, generate, analyze or transform other programs, and even modify itself while running"*

In our particular case, we will use template metaprogramming to generate code that otherwise we would have to create ourselves manually. By expressing our intention through templates, we rely on the compiler to generate the run-time code for us. The main benefit for us will be **automation**, and to some extent, **speed** (as we will see, the compiler unrolls recursive calls generated by template meta-functions).

## Type lists

In metaprogramming, there is a construct called **type list** that we will use to model our lists. In C++11 it is simple to define it by using variadic templates.

```cpp
namespace tmp {

    template<typename...>
    struct typelist {
    };

}
```

But, before we start, what is a **variadic template** in the first place?

> A variadic template is a template that has at least one parameter pack, which is a group of template parameters that accept zero or more parameters.

For instance, according to the previous definition, the type `typelist` accepts the following parameter possibilities:

```cpp
using all_types = typelist<I, C, Z, G, D, F, L, C, I, A, T, B, J, K, H, E, E>;
using an_empty_typelist = typelist<>;
```

Note: in this article, we will be using extensively the type alias [**using**](http://en.cppreference.com/w/cpp/language/type_alias).

`all_types` and `an_empty_typelist` are type aliases that represent the type `typelist` with 17 and 0 parameters respectively.

When we want a specialization of a variadic template, in that specialization we can mention again the parameter pack:

```c++
namespace tmp {

    template<typename...TS>
    struct typelist {
        // General implementation
    };

    template<typename... TS>
    struct typelist<int, TS...> {
        // Specific implementation
    };

}
```

In this particular example, we want a different implementation when the first type is just `int` regardless of the rest. We are going to take advantage of this feature during the whole article.

In the standard library, we can find types that *hold* other types, for instance, `std::tuple`, which can be considered practically a *type list*.

## Our goal

Going back to our original problem, the kind of interface we are looking for is this:

```cpp
namespace tmp {

    template<typename TL, typename T>
    struct find_ancestors;
  
}
```

So that we can use it as follows:

```cpp
using ancestors = typename tmp::find_ancestors<all_types, K>::type;
auto instance = K();
hierarchy_iterator<ancestors>(&instance); 
```

The call to `hierarchy_iterator` will make to recursively call the same for each ordered ancestor of `K`.

## Simple operations

In order to achieve our goal, we will need a few simple operations, like adding types, removing types and accessing types by index. Specifically, we need to implement the following meta-functions: ***push_back***, ***push_front***, ***pop_front*** and ***at***. 

The meta-function ***push_back*** is as follows:

```cpp
namespace tmp {

    template<typename T, typename TL> 
    struct push_back;

}
```

We need an specialization for *type lists*:

```cpp
namespace tmp {

    template<typename T, typename...TS> 
    struct push_back<T, typelist<TS...>> { 
        using type = typelist<TS..., T>; 
    };

}
```

where `TL` is `typename<TS...>` and `T` is the same. The resulting *type list* will be the original one plus `T` at the end. We will construct ***push_front*** and ***pop_back*** in the same way:

```cpp
namespace tmp {

    template<typename T, typename TL> 
    struct push_front;

    template<typename T, typename...TS> 
    struct push_front<T, typelist<TS...>> { 
        using type = typelist<T, TS...>; 
    };

    template<typename TL>
    struct pop_front;

    template<typename T, typename...TS> 
    struct pop_front<typelist<T, TS...>> { 
        using type = typelist<TS...>; 
    };

}
```

Notice that for ***pop_front*** we are not implementing the case of `TL` = `typelist<>`. That is a good exercise if you want to get more familiar with metaprogramming.

Finally, ***at*** metafunction which receives the *type list* and the index on that *type list* we want to retrieve.

```cpp
namespace tmp {

    template<typename TL, size_t I>
    struct at;

}
```

This time, we need two specializations, one for the base case (index == 0), which will be the most specialized one, and another for the general case (index != 0)

```cpp
namespace tmp {

    template <typename T, typename...TS>
    struct at<typelist<T, TS...>, 0> {
        using type = T;
    };

    template <typename T, typename...TS, size_t I>
    struct at<typelist<T, TS...>, I> {
        using type = typename at<typelist<TS...>, I - 1>::type;
    };

}
```

When `I` == 0 (the base case), the result is the first element of the *type list* `typelist<T, TS...>`, this is `T`. In the general case, we must move the index backwards until we force the base case. 

Let us simulate the series of templates the compiler instantiates when invoking the meta-function:

```cpp 
using result = typename at<typelist<A, B, C, D>, 2>::type;

struct at<typelist<A, B, C, D>, 2> {
    using type = typename at<typelist<B, C, D>, 1>::type;
};
struct at<typelist<B, C, D>, 1> {
    using type = typename at<typelist<C, D>, 0>::type;
};
struct at<typelist<C, D>, 0> {
    using type = C;
};
```

## Complex operations

For or solution, we require two more auxiliary meta-functions: ***filter*** and ***max***

```cpp
namespace tmp {

    template<typename TL, template<typename>class PRED>
    struct filter;

    template<typename TL, template<typename,typename>class PRED> 
    struct max;

}
```

Where `TL` is a *type list* and `PRED` is a unary predicate for the *filter* metafunction and a binary predicate for *max*. 

The metaclass ***filter*** can be described in a couple of sentences

* *If the type list is empty, the result is an empty type list*
* *If not, if the predicate validates T the result is T + filter of the remaining, or just without T otherwise.*

In code it is expressed like this:

```cpp
namespace tmp {

    template <template<typename>class PRED>
    struct filter<typelist<>, PRED> {
        using type = typelist<>;
    };

    template <template<typename>class PRED, typename T, typename...TS>
    struct filter<typelist<T, TS...>, PRED> {
        using filtered_remaining = typename filter<typelist<TS...>, PRED>::type;
        using type = typename conditional<
            PRED<T>::value,
            typename push_front<T, filtered_remaining>::type,
            filtered_remaining
        >::type;
    };

}
```

The metaclass ***max*** can also be described in a couple of sentences

* *If the list has exactly one element, the result is that element*
* *Otherwise, if the predicate on the first element and the max of the remaining elements validates, **first** is the result, if not it is the **max** of the remaining elements.*

Expressed in terms of code:

```cpp
namespace tmp {

    template <typename T, template<typename,typename>class PRED>
    struct max<typelist<T>, PRED> {
        using type = T;
    };

    template <typename ...TS, template<typename,typename>class PRED>
    struct max<typelist<TS...>, PRED> {
        using first = typename at<typelist<TS...>, 0>::type;
        using remaining_max = typename max<typename pop_front<typelist<TS...>>::type, PRED>::type;
        using type = typename std::conditional<PRED<first, remaining_max>::value,
            first, 
            remaining_max
        >::type;
    };

}
```

## Finally, *find_ancestors*

This is the implementation of the meta-function ***find_ancestors*** we wanted in the first place:

```cpp
namespace tmp {

    template<typename TL, typename T>
    struct find_ancestors {

        template<typename U>
        using base_of_T = typename std::is_base_of<U, T>::type;

        using type = typename impl::find_ancestor<
            typename filter<TL, base_of_T>::type,
            typelist<>
        >::type;

    };

}
```

Given a *type list* `TL` and a type `T`, this metafunction will define a *type list* alias `type` with the ordered list of ancestors of `T`. Notice that we define a predicate inside the meta-class body that is used first to filter the source *type list*.

In addition, as you probably have already spotted, the actual implementation lives in `impl::find_ancestor` which receives two *type lists*: the **source** and the **destination**:

```cpp
namespace tmp {

    namespace impl {

        template<typename SRCLIST, typename DESTLIST>
        struct find_ancestor {

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
        struct find_ancestor<typelist<>, DESTLIST> {
            using type = DESTLIST;
        };

    }

}
```

Again, the base case is straightforward: if it receives an empty *type list* as source, `type` is the destination list without any modification.

In the general case, there are a few preliminary computations and definitions. Firstly, a unary predicate that forms the negation of a type trait (`negation`) (*defined in the standard library starting from C++17*). Secondly, a binary predicate is used to generate a type alias with the most ancient type (`cmp` and `most_ancient` respectively). And finally, a unary predicate that given a type `T` evaluates to true if it is not the most ancient type (`not_most_ancient`).

With all this, `impl::find_ancestor` behaviour can be described in a few sentences:

* If the source *type list* is empty, the alias `type` is the destination *type list*
* If not, call recursively with the source list filtered by the predicate `not_most_ancient` and the destination as the concatenation if the destination list and the `most_ancient` type.

In fact, this is a variation of the [Selection Sort](https://en.wikipedia.org/wiki/Selection_sort#Implementation) algorithm. It is definitely not the best sorting algorithm, but it is fairly simple to implement with meta-functions. 

This is how it is used:

```cpp
using REGISTRY  = typelist<I, C, Z, G, D, F, L, C, I, A, T, B, J, K, H, E, E>;
using ANCESTORS = find_ancestors<REGISTRY, K>::type;
```

In this precise example, as `J` and `I` are both parents of `K` but not related to each other their order depends on the order of registry. In this case as `J` is defined after `I` it appears before it. The outcome of the meta-function invocation will be `typelist<F, H, J, I, K>`.

## Invoke inspection

It is easy to write down a template function that inspects the whole hierarchy given the *type list*

```cpp
template<typename TL>
struct hierarchy_iterator {
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
```

Again, we need a specialization for empty lists and a general case where we do something useful.

## Code generation

In the repository file *main.cpp* there is a full example with all the needed code to test our solution. If we copy paste the source code and paste it into [**https://godbolt.org/**](https://godbolt.org/) we get the following assembly code:

```asm
.LC0:
  .string "============================================================="
.LC1:
  .string "base = %s\n"
main:
  sub rsp, 8
  mov edi, OFFSET FLAT:.LC0
  call puts
  mov esi, OFFSET FLAT:typeinfo name for A
  mov edi, OFFSET FLAT:.LC1
  xor eax, eax
  call printf
  mov esi, OFFSET FLAT:typeinfo name for C
  mov edi, OFFSET FLAT:.LC1
  xor eax, eax
  call printf
  mov esi, OFFSET FLAT:typeinfo name for D
  mov edi, OFFSET FLAT:.LC1
  xor eax, eax
  call printf
  mov edi, OFFSET FLAT:.LC0
  call puts
  mov esi, OFFSET FLAT:typeinfo name for F
  mov edi, OFFSET FLAT:.LC1
  xor eax, eax
  call printf
  mov esi, OFFSET FLAT:typeinfo name for H
  mov edi, OFFSET FLAT:.LC1
  xor eax, eax
  call printf
  mov esi, OFFSET FLAT:typeinfo name for J
  mov edi, OFFSET FLAT:.LC1
  xor eax, eax
  call printf
  mov esi, OFFSET FLAT:typeinfo name for I
  mov edi, OFFSET FLAT:.LC1
  xor eax, eax
  call printf
  mov esi, OFFSET FLAT:typeinfo name for K
  mov edi, OFFSET FLAT:.LC1
  xor eax, eax
  call printf
  mov edi, OFFSET FLAT:.LC0
  call puts
  xor eax, eax
  add rsp, 8
  ret
typeinfo name for A:
  .string "1A"
typeinfo name for F:
  .string "1F"
typeinfo name for C:
  .string "1C"
typeinfo name for H:
  .string "1H"
typeinfo name for D:
  .string "1D"
typeinfo name for J:
  .string "1J"
typeinfo name for I:
  .string "1I"
typeinfo name for K:
  .string "1K"
```

As we can see, the compiler unrolls the recursive calls for ancestors of `D` (`typelist<A, C, D>`) and ancestors of `K` (`typelist<F, H, J, I, K>`). In addition to the benefit of the automation, an improvement in speed occurs as a consequence of the unrolling.

## Further reading

* [How to build and maintain a global type-list](https://github.com/galtza/global-typelist):
  In this article it is discussed the techniques to allow compile-time events to build and maintain a globally defined type list.

* References:
  * [Metaprogramming](https://en.wikipedia.org/wiki/Metaprogramming)
  * [Type alias](http://en.cppreference.com/w/cpp/language/type_alias)
  * [std::conditional](http://en.cppreference.com/w/cpp/types/conditional)
  * [std::is_base_of](http://en.cppreference.com/w/cpp/types/is_base_of)
  
#### About this document

March 28, 2018 &mdash; Raul Ramos

[LICENSE](https://github.com/galtza/hierarchy-inspector/blob/master/LICENSE)
