
#include <array>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <utility>
#include <algorithm>

constexpr const unsigned BYTE_SIZE = 8;
// TODO: calculate logs using recursive constexpr log
constexpr const unsigned LOG_BYTE_SIZE = 3;
constexpr const unsigned BYTE_MASK = 0b111;
constexpr const unsigned WORD_SIZE = 32;
constexpr const unsigned LOG_WORD_SIZE = 5;
constexpr const unsigned WORD_MASK = 0b11111;
constexpr const unsigned WORD_REMAINDER_BITS = 0b11;
constexpr const unsigned BYTES_PER_WORD = WORD_SIZE/BYTE_SIZE;

using Word = long unsigned;
using byte = unsigned char;
using std::size_t;

/////////////// BitArray (similar to std::bitset) ////

constexpr Word word_ceil (Word n) {
    return (((n & WORD_REMAINDER_BITS) > 0)
                    ? BYTES_PER_WORD
                    : 0)
            + (n & ~WORD_REMAINDER_BITS);
}
static_assert(word_ceil(3) == 4, "word ceil");
static_assert(word_ceil(4) == 4, "word ceil");
static_assert(word_ceil(5) == 8, "word ceil");

template<size_t size>
class BitArray {
    class BitRef;
public:
    constexpr static const size_t len_in_bits = size;
    constexpr static const size_t len_in_bytes = ((size & BYTE_MASK) > 0) + (size >> LOG_BYTE_SIZE);
    constexpr static const size_t len_in_words = ((size & WORD_MASK) > 0) + (size >> LOG_WORD_SIZE);
//private:
    std::array<byte, BYTES_PER_WORD*len_in_words> data;
public:
    //// public methods
    void reset(size_t i) noexcept {
        (*this)[i] = 0;
    }
    void set(size_t i) noexcept {
        (*this)[i] = 1;
    }
    bool test(size_t i) const noexcept {
        return (*this)[i];
    }
    const BitRef operator[] (size_t i) const noexcept {
        return BitRef(*this, i);
    }
    BitRef operator[] (size_t i) noexcept {
        return BitRef(*this, i);
    }
    //// iteration
private:
    template<bool const_>
    class BaseBitIterator {
        using value_type = BitRef;
        //TODO using difference_type
        using iterator_category = std::forward_iterator_tag;
        using IterableRef = typename std::conditional_t<const_, const BitArray&, BitArray&>;
        using ValueRef = typename std::conditional_t<const_, const BitRef, BitRef>;
    public:
    protected:
        IterableRef bit_array;
        size_t index;
    public:
        BaseBitIterator(IterableRef in_bit_array, size_t in_idx)
            : bit_array(in_bit_array), index(in_idx) {}
        // XXX: should check if reference is the same?
        bool operator== (const BaseBitIterator& other) { return this->index == other.index; }
        bool operator!= (const BaseBitIterator& other) { return !(*this == other); }
        BaseBitIterator operator++ () { auto r = *this; ++index; return r; }
        BaseBitIterator& operator++ (int) { ++index; return *this; }
    private:
        ValueRef deref_operator () {
            return this->bit_array[this->index];
        }
    public:
        template <typename _ = ValueRef>
        std::enable_if_t<!const_, _> operator* () {
            return this->deref_operator();
        }
        template <typename _ = ValueRef>
        std::enable_if_t<const_, _> operator* () const {
            return const_cast<BaseBitIterator<const_>*>(this)->deref_operator();
        }
    };
    using BitIterator = BaseBitIterator<false>;
    using ConstBitIterator = BaseBitIterator<true>;
    using iterator = BitIterator;
    using const_iterator = ConstBitIterator;

public:
    BitIterator begin() { return BitIterator(*this, 0); }
    BitIterator end() { return BitIterator(*this, len_in_bits); }
    ConstBitIterator begin() const { return ConstBitIterator{*this, 0}; }
    ConstBitIterator end() const { return ConstBitIterator{*this, len_in_bits}; }

    auto beginBytes() { return data.begin(); }
    auto beginBytes() const { return data.begin(); }
    auto endBytes() { return data.end() - (len_in_words*BYTES_PER_WORD/len_in_bytes); }
    auto endBytes() const { return data.end() - (len_in_words*BYTES_PER_WORD/len_in_bytes); }

    // TODO: replace with an iter_words function returning a private type with the
    // ranged for loop interface implemented
public:
    Word* beginWords() { return reinterpret_cast<Word*>(&data[0]); }
    const Word* beginWords() const { return reinterpret_cast<const Word*>(&data[0]); }
    Word* endWords() {
        return reinterpret_cast<Word*>(&data[BYTES_PER_WORD*len_in_words]);
    }
    const Word* endWords() const {
        return reinterpret_cast<const Word*>(&data[BYTES_PER_WORD*len_in_words]);
    }

    //// non-bitset public methods

    // TODO: specialize std::find
    // find the first 1 in the collection of bits
    size_t findFirstSetBit() const {
        size_t result;
        const auto foundWordItr = std::find_if(beginWords(), endWords(),
                                              [](auto w){ return w != 0; });
        if (foundWordItr == endWords())
            return size;
        const Word& foundWord = *foundWordItr;
        // TODO: use macro for non-gcc/clang implementation
        const int num_leading_zeros = __builtin_clz(__builtin_bswap32(foundWord));
        result = num_leading_zeros;
        return result;
    }
    //// implement iterator in words, bytes, and the default, bits
    //// BitRef Impl
private:
    class BitRef {
        const size_t offset;
        BitArray& bit_array;
        const int byte_offset = offset >> LOG_BYTE_SIZE;
        const char bit_offset = offset & BYTE_MASK;
        byte& containing_byte = bit_array.data[byte_offset];
    public:
        BitRef(const BitArray& in_bit_array, size_t in_offset)
            // XXX: can an owner of a const BitRef modify the bit array?
            // people editing this can thanks to that dirty con_cast
            : bit_array(const_cast<BitArray&>(in_bit_array))
            , offset(in_offset) {}
        operator bool() const {
            return static_cast<bool>((containing_byte >> (BYTE_SIZE-bit_offset-1)) & 0b1);
        }
        void operator= (bool in) {
            if (in) containing_byte |= 0b1 << (BYTE_SIZE-bit_offset-1);
            else containing_byte &= ~(0b1 << (BYTE_SIZE-bit_offset-1));
        }
    };
    //// construction
public:
    BitArray() : data() {}

    template<typename T>
    BitArray(T&& t) : data(byte_array_from_obj(t)) {}

    template<typename T>
    BitArray& operator= (T&& t) {
        data = byte_array_from_obj(t);
        return *this;
    }

    template<typename T>
    static auto byte_array_from_obj(T&& obj) {
        std::array<byte,word_ceil(sizeof(obj))> result;
        auto raw = reinterpret_cast<const void*>(&obj);
        auto bytes = reinterpret_cast<const byte*>(raw);
        for (int i = 0; i < sizeof(obj); ++i)
            result[i] = bytes[i];
        return result;
    }
};

// implicit size deduction guideline
template<typename T>
BitArray(T&& t) -> BitArray<word_ceil(sizeof(t)<<LOG_BYTE_SIZE)>;

template<size_t size>
std::ostream& operator<< (std::ostream& os, const BitArray<size>& bitarr) {
    for (const auto& b: bitarr)
        os << (b ? "1" : "0");
    return os;
}


/////////////// Pooled Flyweight ////

enum class AllocState : int {
    ALLOCATED = 0,
    FREE = 1
};

//maybe one would inherit from this to gain the protected
//flyweight element?
template<typename FlyweightType, size_t pool_size>
class PooledFlyweightUser {
protected:
    class RefCountedFlyweightType;
private:
    //// Construction
    static BitArray<pool_size> init_alloc_states() {
        BitArray<pool_size> result;
        for (auto itr = result.beginWords(); itr != result.endWords(); ++itr)
            *itr = 0xffffffff;
        return result;
    }
    constexpr static const auto no_cell = pool_size;

    //// Class Variables
    static BitArray<pool_size> alloc_states;
    //TODO: prevent construction at allocation time
    static std::array<RefCountedFlyweightType, pool_size> pool;

    /// Internal routines
    static size_t tryFindFreeCell() {
        auto bit_index = alloc_states.findFirstSetBit();
        if (bit_index == alloc_states.len_in_bits)
            return no_cell;
        else
            return bit_index;
    }
    //// Types
protected:
    class RefCountedFlyweightType : public FlyweightType {
        unsigned refcount;
    public:
        // TODO: forward all constructor calls to base constructor
        RefCountedFlyweightType() : refcount(1), FlyweightType() {}

        static bool is_pooled(RefCountedFlyweightType* ptr) {
            return ptr >= &pool[0] && ptr < &pool[pool_size];
        }
        void incRef() { ++refcount; };
        void decRef() { --refcount; /* if (refcount==0) delete this*/ };
        void* operator new(size_t size) {
            // TODO: static_assert that the flyweight type is not a derived/extended type
            // TODO: use hash table to implement hash equality testing for flyweight interning
            auto cell = tryFindFreeCell();
            if (cell == no_cell)
                return ::new RefCountedFlyweightType;
            else {
                alloc_states[cell] = AllocState::ALLOCATED;
                return &pool[cell];
            }
        }
        void operator delete(void* ptr) {
            auto* _ptr = reinterpret_cast<RefCountedFlyweightType*>(ptr);
            if (!is_pooled(_ptr))
                ::delete _ptr;
            else
                alloc_states[(_ptr - &pool[0])/sizeof(_ptr)] = AllocState::FREE;
        }
        // TODO: add byte-wise equality operator implementation?
    };

    //// Data Members
//protected:
    const RefCountedFlyweightType& flyweight;

    //// Construction
    PooledFlyweightUser() : flyweight(*(new RefCountedFlyweightType)) {
    }
};

template<typename T, size_t sz>
BitArray<sz> PooledFlyweightUser<T,sz>::alloc_states =
    PooledFlyweightUser<T,sz>::init_alloc_states();

template<typename T, size_t sz>
std::array<typename PooledFlyweightUser<T,sz>::RefCountedFlyweightType, sz>
PooledFlyweightUser<T,sz>::pool;

/////////////// Example ////

enum TileType {forest, plains};

struct ExampleFlyweight {
    TileType tile_type = forest;
    char production = 2;
    char food = 3;
};

class ExampleUser : public PooledFlyweightUser<ExampleFlyweight, 100> {
    char test = 'h';
};

int main() {
    BitArray b = 0b1110100111;
}
