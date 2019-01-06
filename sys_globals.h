// Elmo Trolla, 2018
// License: pick one - UNLICENSE (www.unlicense.org) / MIT (opensource.org/licenses/MIT). EXCEPT
//          for the class Array, which is derived from imgui and is under the MIT license.

#pragma once

#include <stdlib.h> // malloc, free
#include <stdint.h>
#include <string.h> // memcpy

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef float    f32;
typedef double   f64;

inline i32 clamp_i32(i32 v, i32 hi, i32 lo) { if (hi < lo) { i32 d = hi; hi = lo; lo = d; } if (v < lo) v = lo; if (v > hi) v = hi; return v; }
inline i32 clamp_u32(u32 v, u32 hi, u32 lo) { if (hi < lo) { u32 d = hi; hi = lo; lo = d; } if (v < lo) v = lo; if (v > hi) v = hi; return v; }
inline i32 clamp_f32(f32 v, f32 hi, f32 lo) { if (hi < lo) { f32 d = hi; hi = lo; lo = d; } if (v < lo) v = lo; if (v > hi) v = hi; return v; }

#define ASSERT IM_ASSERT
#define ALWAYS_ASSERT IM_ASSERT // this is NOT removed for release builds. good to test for things like if malloc succeeded.

#ifndef IM_ASSERT
	#include <assert.h>
	#define IM_ASSERT(_EXPR) assert(_EXPR)
#endif

#define ELEMENTS_IN_ARRAY(array) (sizeof(array) / sizeof(array[0]))

#ifndef IMGUI_VERSION
	// Helpers macros to generate 32-bits encoded colors
	#ifdef IMGUI_USE_BGRA_PACKED_COLOR
		#define IM_COL32_R_SHIFT    16
		#define IM_COL32_G_SHIFT    8
		#define IM_COL32_B_SHIFT    0
		#define IM_COL32_A_SHIFT    24
		#define IM_COL32_A_MASK     0xFF000000
	#else
		#define IM_COL32_R_SHIFT    0
		#define IM_COL32_G_SHIFT    8
		#define IM_COL32_B_SHIFT    16
		#define IM_COL32_A_SHIFT    24
		#define IM_COL32_A_MASK     0xFF000000
	#endif
	#define IM_COL32(R,G,B,A)    (((u32)(A)<<IM_COL32_A_SHIFT) | ((u32)(B)<<IM_COL32_B_SHIFT) | ((u32)(G)<<IM_COL32_G_SHIFT) | ((u32)(R)<<IM_COL32_R_SHIFT))
	#define IM_COL32_WHITE       IM_COL32(255,255,255,255)  // Opaque white = 0xFFFFFFFF
	#define IM_COL32_BLACK       IM_COL32(0,0,0,255)        // Opaque black
	#define IM_COL32_BLACK_TRANS IM_COL32(0,0,0,0)          // Transparent black = 0x00000000
#endif

// Code originally taken from imgui.h ImVector, with modifications:
//     Added push_back_notconstructed() that allocates memory for a new element and returns a pointer to it. Up to the user to construct the object into the returned slot.
//     Disable copy constructor.
//     On destruction, call destructors of all allocated slots.
//     Remove methods I don't use yet.
//
// Original comment below:
//
// Lightweight std::vector<> like class to avoid dragging dependencies (also: windows implementation of STL with debug enabled is absurdly slow, so let's bypass it so our code runs fast in debug).
// Our implementation does NOT call C++ constructors/destructors. This is intentional and we do not require it. Do not use this class as a straight std::vector replacement in your code!
template<typename T>
class Array
{
public:
	int                         Size;
	int                         Capacity;
	T*                          Data;

	typedef T                   value_type;
	typedef value_type*         iterator;
	typedef const value_type*   const_iterator;

	Array(const Array&) = delete; // disable copy constructor.
	Array& operator=(Array const&) = delete; // prevent assigning

	inline Array()           { Size = Capacity = 0; Data = NULL; }
	inline ~Array()          { clear(); } // this calls all destructors and frees allocated memory

	inline bool                 empty() const                   { return Size == 0; }
	inline int                  size() const                    { return Size; }
	inline int                  capacity() const                { return Capacity; }

	inline value_type&          operator[](int i)               { IM_ASSERT(i < Size); return Data[i]; }
	inline const value_type&    operator[](int i) const         { IM_ASSERT(i < Size); return Data[i]; }

	inline void                 clear()                         { if (Data) { Size = Capacity = 0; for (int i = Size - 1; i >= 0; i--) (*this)[i].~T(); free(Data); Data = NULL; } }
	inline iterator             begin()                         { return Data; }
	inline const_iterator       begin() const                   { return Data; }
	inline iterator             end()                           { return Data + Size; }
	inline const_iterator       end() const                     { return Data + Size; }
	inline value_type&          front()                         { IM_ASSERT(Size > 0); return Data[0]; }
	inline const value_type&    front() const                   { IM_ASSERT(Size > 0); return Data[0]; }
	inline value_type&          back()                          { IM_ASSERT(Size > 0); return Data[Size - 1]; }
	inline const value_type&    back() const                    { IM_ASSERT(Size > 0); return Data[Size - 1]; }
	//inline void                 swap(ImVector<T>& rhs)          { int rhs_size = rhs.Size; rhs.Size = Size; Size = rhs_size; int rhs_cap = rhs.Capacity; rhs.Capacity = Capacity; Capacity = rhs_cap; value_type* rhs_data = rhs.Data; rhs.Data = Data; Data = rhs_data; }

	inline int                  _grow_capacity(int size) const  { int new_capacity = Capacity ? (Capacity + Capacity/2) : 8; return new_capacity > size ? new_capacity : size; }

	inline void                 resize(int new_size)            { if (new_size > Capacity) reserve(_grow_capacity(new_size)); Size = new_size; }
	inline void                 resize(int new_size, const T& v){ if (new_size > Capacity) reserve(_grow_capacity(new_size)); if (new_size > Size) for (int n = Size; n < new_size; n++) Data[n] = v; Size = new_size; }
	inline void                 reserve(int new_capacity) {
		if (new_capacity <= Capacity)
			return;
		T* new_data = (value_type*)malloc((size_t)new_capacity * sizeof(T));
		ALWAYS_ASSERT(new_data);
		if (Data) {
			memcpy(new_data, Data, (size_t)Size * sizeof(T));
			free(Data);
		}
		Data = new_data;
		Capacity = new_capacity;
	}

	inline void                 push_back(const value_type& v)  { if (Size == Capacity) reserve(_grow_capacity(Size + 1)); Data[Size++] = v; }

	inline T*                   push_back_notconstructed()      { if (Size == Capacity) reserve(_grow_capacity(Size + 1)); return &Data[Size++]; }

	inline void                 pop_back()                      { IM_ASSERT(Size > 0); Size--; Data[Size].~T(); }
//	inline void                 push_front(const value_type& v) { if (Size == 0) push_back(v); else insert(Data, v); }

//	inline iterator             erase(const_iterator it)        { IM_ASSERT(it >= Data && it < Data+Size); it->~T(); const ptrdiff_t off = it - Data; memmove(Data + off, Data + off + 1, ((size_t)Size - (size_t)off - 1) * sizeof(value_type)); Size--; return Data + off; }
//	inline iterator             insert(const_iterator it, const value_type& v)  { IM_ASSERT(it >= Data && it <= Data+Size); const ptrdiff_t off = it - Data; if (Size == Capacity) reserve(_grow_capacity(Size + 1)); if (off < (int)Size) memmove(Data + off + 1, Data + off, ((size_t)Size - (size_t)off) * sizeof(value_type)); Data[off] = v; Size++; return Data + off; }
//	inline bool                 contains(const value_type& v) const             { const T* data = Data;  const T* data_end = Data + Size; while (data < data_end) if (*data++ == v) return true; return false; }
};

