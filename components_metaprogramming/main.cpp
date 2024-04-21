#include <cstdint>
#include <tuple>
#include <iostream>

/**
* Inspired by a Component based application
*/

template<typename T>
concept ActivityConcept = requires (T t)
{
    { t.run() } -> std::same_as<void>; // run() function with void return type
};

template <typename T>
struct IsStdTuple { static constexpr bool value = false; };

template <typename ...Args>
struct IsStdTuple<std::tuple<Args...>> { static constexpr bool value = true; };

template <typename T>
concept ActivitiesTupleConcept = requires (T t)
{
    requires IsStdTuple<T>::value;          // It is a std::tuple
    {std::get<0>(t)} -> ActivityConcept;    // Activity concept enforced for first item TODO: extend this to all the classes
};


class ActivityButton
{
    public:

    void run()
    {
        //__asm("marker button");
    }
};

class ActivityMotion
{
    public:

    void run()
    {
        //__asm("marker motion");
    }
};

// Recursive case
template<   const unsigned N,
            const unsigned M = 0U>
struct TupleRunEach
{
    template<ActivitiesTupleConcept tuple_type>
    static void runAct(tuple_type& t)
    {
        // Run the M’th object and the next higher one.
        std::get<M>(t).run();
        TupleRunEach<N, M + 1U>::runAct(t);
    }
};

// Base case (end of recursion --> N == M)
template<const unsigned N>
struct TupleRunEach<N, N>
{
    template<typename tuple_type>
    static void runAct(tuple_type&) { }
};

namespace
{
    // Instantiate the activities
    using TupleActivitiesType = std::tuple<ActivityButton, ActivityMotion>; // NOTE: simplified case, no args to ctors
    TupleActivitiesType activities;

    // Get the size of the tuple
    constexpr unsigned tuple_size = std::tuple_size<TupleActivitiesType>::value;

} // anonymous namespace

int main()
{
    // Run all the activities
    TupleRunEach<tuple_size>::runAct(activities);
}
