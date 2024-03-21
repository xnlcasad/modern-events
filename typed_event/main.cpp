#include <iostream>
#include <cassert>
#include <array>

enum class Signal {
    Invalid,
    ButtonPressed,
    TimerExpired,
    XY_RawData,
    HIDPP,
};

using ButtonMask = unsigned int;

struct xy_data {
    int x;
    int y;
};

struct h20payload
{
    static constexpr auto hidpp20_PAYLOAD_LONG_SIZE = 55U;
    uint8_t deviceIdx;
    uint8_t featureIdx;
    uint8_t funcIndex_swId;
    std::array<uint8_t, hidpp20_PAYLOAD_LONG_SIZE> methodParams;
};

enum class EventPool {
    Static,
    Small,
    Large
};


class MemoryPoolManager {
public:
    MemoryPoolManager() {
        // Initialize memory pools
    }

    ~MemoryPoolManager() {
        // Clean up memory pools
    }

    // Allocate memory block of specified size
    void* get(std::size_t blockSize) {
        return malloc(blockSize);
    }

    // Deallocate memory block of specified size
    void release(void* memory_block) {
        free(memory_block);
    }
};

// Global MemoryPoolManager instances
MemoryPoolManager global_small_pool_manager;
MemoryPoolManager global_large_pool_manager;

template <typename T>
constexpr EventPool event_pool_selector() {
    constexpr std::size_t threshold = 42; // Set an appropriate threshold value

    if constexpr (sizeof(T) <= threshold) {
        return EventPool::Small;
    } else {
        return EventPool::Large;
    }
}

template <typename T>
constexpr MemoryPoolManager& get_pool_manager() {

    if constexpr (event_pool_selector<T>() == EventPool::Small) {
        return global_small_pool_manager;
    } else {
        return global_large_pool_manager;
    }
}

// The Event class models an event that only has a signal as useful information
class Event {
public:
    // Event objects can not be allocated without a signal, and cannot be copied or moved
    Event() = delete;
    Event(const Event&) = delete;
    Event(Event&&) = delete;
    // The public constructor does not allow to specify the EventPool, it is
    // meant to be used for static events only.
    explicit Event(Signal sig) : signal_(sig), poolId_(EventPool::Static) {}
    [[nodiscard]] Signal getSignal() const { return signal_; }
    [[nodiscard]] EventPool getPoolId() const { return poolId_; }
protected:
    // A derived class can specify the EventPool
    explicit Event(Signal sig, EventPool poolId) : signal_(sig), poolId_(poolId) {}

private:
    // A factory function, allows to create pool events, the dummy parameter is used
    // to distinguish it from the protected constructor
    explicit Event(Signal sig, EventPool poolId, int) : signal_(sig), poolId_(poolId) {}

    // make_Event is used to create events based on the memory pool.
    friend const Event* make_Event(Signal sig);

    const Signal signal_;
    const EventPool poolId_;
    uint_fast32_t refCount_{0};
};


template <Signal Sig, typename EvType, EventPool Pool = EventPool::Static>
class TypedEvent : public Event {
public:
    // TypedEvent objects can not be allocated without data, and cannot be copied or moved
    TypedEvent() = delete;
    TypedEvent(const TypedEvent&) = delete;
    TypedEvent(TypedEvent&&) = delete;
    // The public constructor does not allow to specify the EventPool, it is
    // meant to be used for static events only.
    explicit TypedEvent(const EvType& data) : Event(Sig, EventPool::Static), data_(data) {
        static_assert(Pool == EventPool::Static);
    }

    [[nodiscard]] const EvType& getEventData() const {
        return data_;
    }

private:
    // The constructor that allows to specify an EventPool (i.e. no EventPool::Static)
    // is only accessible to the make_Event function.
    explicit TypedEvent(const EvType& data, int) : Event(Sig, Pool), data_(data) {
        static_assert(Pool != EventPool::Static);
    }

    // Allow make_Event to access the private constructor
    template <Signal EvTag, typename U, typename... Args>
    friend auto make_TypedEvent(Args&&... args) -> const TypedEvent<EvTag, U, event_pool_selector<U>()>*;

    const EvType data_;
};

// Factory function to create simple events from the memory pool.
const Event* make_Event(Signal sig) {
    void* mem_block = global_small_pool_manager.get(sizeof(Event));

    assert(mem_block != nullptr);
    return new (mem_block) Event(sig, EventPool::Small, 0);
}


// Factory function to create Typed events from the memory pool.
template <Signal EvTag, typename EvType, typename... Args>
const TypedEvent<EvTag, EvType, event_pool_selector<EvType>()>* make_TypedEvent(Args&&... args) // -> const TypedEvent<EvTag, EvType, event_pool_selector<EvType>()>*
{

    // Determine the required memory block size based on the event type.
    std::size_t blockSize = sizeof(TypedEvent<EvTag, EvType>);

    // Choose the appropriate global pool manager based on the event size.
    MemoryPoolManager& pool_manager = get_pool_manager<EvType>();

    // Request memory block from the selected pool manager.
    void* mem_block = pool_manager.get(blockSize);

    assert(mem_block != nullptr);

    // Determine the EventPool value based on the size of the event.
    constexpr EventPool poolId = event_pool_selector<EvType>();

    // Construct the event using the placement new operator in the allocated memory block
    // and call the new private constructor with the dummy parameter.
    auto const *event = new (mem_block) TypedEvent<EvTag, EvType, poolId>(std::forward<Args>(args)..., 0);
    // Return the pointer to the newly created event.
    return event;
}

void print_value(const Event& event) {
    std::cout << "Event pool:" << static_cast<int>(event.getPoolId()) << " signal:" << static_cast<int>(event.getSignal()) << " data:";

    switch (event.getSignal()) {
        case Signal::ButtonPressed: {
            const auto& e = static_cast<const TypedEvent<Signal::ButtonPressed, int>&>(event);
            std::cout << e.getEventData() << std::endl;
            break;
        }
        case Signal::XY_RawData: {
            const auto& e = static_cast<const TypedEvent<Signal::XY_RawData, xy_data>&>(event);
            std::cout << "x:" << e.getEventData().x << " y:" << e.getEventData().y << std::endl;
            break;
        }
        case Signal::HIDPP: {
            const auto& e = static_cast<const TypedEvent<Signal::HIDPP, h20payload>&>(event);
            std::cout << "deviceIdx:" << (int)e.getEventData().deviceIdx << std::endl;
            break;
        }
        default:
            std::cout << "N/A" << std::endl;
    }
}

const auto& get_static_event()
{
    static const Event e(Signal::TimerExpired);
    return e;
}

const auto& get_static_typed_event()
{
    static const TypedEvent<Signal::XY_RawData, xy_data> e({1, 2});
    return e;
}

template <Signal sig, typename T>
const auto& get_static_typed_event_generic(T data)
{
    static const TypedEvent<sig, T> e(data);
    return e;
}

const auto& get_dynamic_event()
{
    return *make_Event(Signal::TimerExpired);
}

const auto& get_dynamic_typed_event()
{
    return *make_TypedEvent<Signal::XY_RawData, xy_data>(xy_data { 1, 2 });
}

int main() {
    static const TypedEvent<Signal::ButtonPressed, ButtonMask> static_typed_event1(21);

    auto* dyn_typed_event1 = make_TypedEvent<Signal::ButtonPressed, ButtonMask>(42);
    auto* dyn_typed_event2 = make_TypedEvent<Signal::XY_RawData, xy_data>(xy_data { 1, 2 });

    auto* dyn_event2 = make_Event(Signal::TimerExpired);

    auto* dyn_hidpp_event = make_TypedEvent<Signal::HIDPP, h20payload>(h20payload {0, 0, 0, {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} });
    static const TypedEvent<Signal::HIDPP, h20payload> static_hidpp_event(h20payload{1, 2, 3, {2,2,2,2}});

    print_value(get_static_typed_event());
    print_value(static_typed_event1);
    print_value(get_static_event());
    print_value(get_dynamic_event());
    print_value(get_dynamic_typed_event());
    print_value(get_static_typed_event_generic<Signal::ButtonPressed>((ButtonMask)12));
    print_value(get_static_typed_event_generic<Signal::ButtonPressed>((ButtonMask)14));

    print_value(*dyn_typed_event1);
    print_value(*dyn_event2);
    print_value(*dyn_typed_event2);
    print_value(*dyn_hidpp_event);
    print_value(static_hidpp_event);

    return 0;
}

// event hold a signal and possibly data
// events hold a reference counter
// queues only hold a pointer to the event
// event memory is allocated from a memory pool or a heap, but can also be statically allocated.
//
