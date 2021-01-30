/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License"),
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Tests/Data/Types/VectorTest.cpp
Unit tests of the RawVector data type

******************************************************************************/

#include <catch2/catch.hpp>

#include <Methane/Data/Vector.hpp>

#include <sstream>

using namespace Methane::Data;

template<typename T, size_t size, typename = std::enable_if_t<2 <= size && size <= 4>>
void CheckRawVector(const RawVector<T, size>& vec, const std::array<T, size>& components)
{
    CHECK(vec[0] == components[0]);
    CHECK(vec[1] == components[1]);
    if constexpr (size > 2)
        CHECK(vec[2] == components[2]);
    if constexpr (size > 3)
        CHECK(vec[3] == components[3]);
}

template<typename T, size_t size>
std::array<T, size> CreateComponents(T first_value = T(1), T step_value = T(1))
{
    std::array<T, size> values{ first_value };
    for(size_t i = 1; i < size; ++i)
    {
        values[i] = first_value + step_value * T(i);
    }
    return values;
}

template<typename T, size_t size>
HlslVector<T, size> CreateHlslVector(const std::array<T, size>& components)
{
    if constexpr (size == 2)
        return HlslVector<T, 2>(components[0], components[1]);
    if constexpr (size == 3)
        return HlslVector<T, 3>(components[0], components[1], components[2]);
    if constexpr (size == 4)
        return HlslVector<T, 4>(components[0], components[1], components[2], components[3]);
}

#define VECTOR_TYPES_MATRIX \
    ((typename T, size_t size), T, size),        \
    (int32_t,  2), (int32_t,  3), (int32_t,  4), \
    (uint32_t, 2), (uint32_t, 3), (uint32_t, 4), \
    (float,    2), (float,    3), (float,    4), \
    (double,   2), (double,   3), (double,   4)  \

TEMPLATE_TEST_CASE_SIG("Raw Vector Initialization and Comparison", "[vector][init]", VECTOR_TYPES_MATRIX)
{
    const std::array<T, size> raw_arr = CreateComponents<T, size>();

    SECTION("Vector size equals sum of its component sizes")
    {
        CHECK(sizeof(RawVector<T, size>) == sizeof(T) * size);
    }

    SECTION("Default initialization with zeros")
    {
        CheckRawVector(RawVector<T, size>(), CreateComponents<T, size>(T(0), T(0)));
    }

    SECTION("Initialization with component values")
    {
        if constexpr (size == 2)
            CheckRawVector(RawVector<T, 2>(T(1), T(2)), { T(1), T(2) });
        if constexpr (size == 3)
            CheckRawVector(RawVector<T, 3>(T(1), T(2), T(3)), { T(1), T(2), T(3) });
        if constexpr (size == 4)
            CheckRawVector(RawVector<T, 4>(T(1), T(2), T(3), T(4)), { T(1), T(2), T(3), T(4) });
    }

    SECTION("Initialization with array")
    {
        CheckRawVector(RawVector<T, size>(raw_arr), raw_arr);
    }

    SECTION("Initialization with array pointer")
    {
        CheckRawVector(RawVector<T, size>(raw_arr.data()), raw_arr);
    }

    SECTION("Initialization with moved array")
    {
        CheckRawVector(RawVector<T, size>(CreateComponents<T, size>()), CreateComponents<T, size>());
    }

    SECTION("Initialization with HLSL vector")
    {

        const HlslVector<T, size> hlsl_vec = CreateHlslVector(raw_arr);
        CheckRawVector(RawVector<T, size>(hlsl_vec), raw_arr);
    }

    SECTION("Copy initialization from the same vector type")
    {
        RawVector<T, size> vec(raw_arr);
        CheckRawVector(RawVector<T, size>(vec), raw_arr);
    }

    if constexpr (size > 2)
    {
        SECTION("Copy initialization from smaller vector size")
        {
            const std::array<T, size - 1> small_arr = CreateComponents<T, size - 1>();
            RawVector<T, size - 1>        small_vec(small_arr);
            CheckRawVector(RawVector<T, size>(small_vec, raw_arr.back()), raw_arr);
        }
    }
    if constexpr (size > 3)
    {
        SECTION("Copy initialization from much smaller vector size")
        {
            const std::array<T, size - 2> small_arr = CreateComponents<T, size - 2>();
            RawVector<T, size - 2>        small_vec(small_arr);
            CheckRawVector(RawVector<T, size>(small_vec, raw_arr[2], raw_arr[3]), raw_arr);
        }
    }

    SECTION("Vectors equality comparison")
    {
        CHECK(RawVector<T, size>(raw_arr) == RawVector<T, size>(raw_arr));
        CHECK_FALSE(RawVector<T, size>(raw_arr) == RawVector<T, size>(CreateComponents<T, size>(T(1), T(2))));
    }

    SECTION("Vectors non-equality comparison")
    {
        CHECK_FALSE(RawVector<T, size>(raw_arr) != RawVector<T, size>(raw_arr));
        CHECK(RawVector<T, size>(raw_arr) != RawVector<T, size>(CreateComponents<T, size>(T(1), T(2))));
    }
}

TEMPLATE_TEST_CASE_SIG("Raw Vector Conversions to Other Types", "[vector][convert]", VECTOR_TYPES_MATRIX)
{
    const std::array<T, size> raw_arr = CreateComponents<T, size>();
    const RawVector<T, size>  raw_vec(raw_arr);

    if constexpr (!std::is_same_v<T, int32_t>)
    {
        SECTION("Cast to vector of integers")
        {
            CheckRawVector(static_cast<RawVector<int32_t, size>>(raw_vec), CreateComponents<int32_t, size>());
        }
    }
    if constexpr (!std::is_same_v<T, uint32_t>)
    {
        SECTION("Cast to vector of unsigned integers")
        {
            CheckRawVector(static_cast<RawVector<uint32_t, size>>(raw_vec), CreateComponents<uint32_t, size>());
        }
    }
    if constexpr (!std::is_same_v<T, float>)
    {
        SECTION("Cast to vector of floats")
        {
            CheckRawVector(static_cast<RawVector<float, size>>(raw_vec), CreateComponents<float, size>());
        }
    }
    if constexpr (!std::is_same_v<T, double>)
    {
        SECTION("Cast to vector of doubles")
        {
            CheckRawVector(static_cast<RawVector<double, size>>(raw_vec), CreateComponents<double, size>());
        }
    }

    SECTION("Cast to string")
    {
        std::stringstream ss;
        ss << "V(" << raw_arr[0];
        for(size_t i = 1; i < size; ++i)
            ss << ", " << raw_arr[i];
        ss << ")";
        CHECK(static_cast<std::string>(raw_vec) == ss.str());
    }

    SECTION("Convert to HLSL vector")
    {
        const HlslVector<T, size> hlsl_vec = raw_vec.AsHlsl();
        CHECK(hlslpp::all(hlsl_vec == CreateHlslVector(raw_arr)));
    }
}

TEMPLATE_TEST_CASE_SIG("Raw Vector Component Accessors and Property Getters", "[vector][accessors]", VECTOR_TYPES_MATRIX)
{
    const std::array<T, size> raw_arr = CreateComponents<T, size>();
    const RawVector<T, size>  raw_vec(raw_arr);
    const T                   new_value(123);

    SECTION("Unsafe component getters by index")
    {
        for(size_t i = 0; i < size; ++i)
        {
            CHECK(raw_vec[i] == raw_arr[i]);
        }
    }

    SECTION("Unsafe component setters by index")
    {
        RawVector<T, size> raw_vec_mutable(raw_arr);
        const std::array<T, size> other_arr = CreateComponents<T, size>(T(5), T(2));
        for(size_t i = 0; i < size; ++i)
        {
            raw_vec_mutable[i] = other_arr[i];
        }
        CheckRawVector(raw_vec_mutable, other_arr);
    }

    SECTION("Safe component getters by index")
    {
        for(size_t i = 0; i < size; ++i)
        {
            CHECK(raw_vec.Get(i) == raw_arr[i]);
        }
        CHECK_THROWS_AS(raw_vec.Get(size + 1), Methane::ArgumentExceptionBase<std::out_of_range>);
    }

    SECTION("Safe component setters by index")
    {
        RawVector<T, size> raw_vec_mutable(raw_arr);
        const std::array<T, size> other_arr = CreateComponents<T, size>(T(5), T(2));
        for(size_t i = 0; i < size; ++i)
        {
            raw_vec_mutable.Set(i, other_arr[i]);
        }
        CHECK_THROWS_AS(raw_vec_mutable.Set(size + 1, T(0)), Methane::ArgumentExceptionBase<std::out_of_range>);
        CheckRawVector(raw_vec_mutable, other_arr);
    }

    SECTION("X-coordinate getter")
    {
        CHECK(raw_vec.GetX() == raw_arr[0]);
    }

    SECTION("X-coordinate setter")
    {
        auto new_arr = raw_arr; new_arr[0] = new_value;
        CheckRawVector(RawVector<T, size>(raw_arr).SetX(new_value), new_arr);
    }

    SECTION("Y-coordinate getter")
    {
        CHECK(raw_vec.GetY() == raw_arr[1]);
    }

    SECTION("Y-coordinate setter")
    {
        auto new_arr = raw_arr; new_arr[1] = new_value;
        CheckRawVector(RawVector<T, size>(raw_arr).SetY(new_value), new_arr);
    }

    if constexpr (size > 2)
    {
        SECTION("Z-coordinate getter")
        {
            CHECK(raw_vec.GetZ() == raw_arr[2]);
        }

        SECTION("Z-coordinate setter")
        {
            auto new_arr = raw_arr; new_arr[2] = new_value;
            CheckRawVector(RawVector<T, size>(raw_arr).SetZ(new_value), new_arr);
        }
    }

    if constexpr (size > 3)
    {
        SECTION("W-coordinate getter")
        {
            CHECK(raw_vec.GetW() == raw_arr[3]);
        }

        SECTION("W-coordinate setter")
        {
            auto new_arr = raw_arr; new_arr[3] = new_value;
            CheckRawVector(RawVector<T, size>(raw_arr).SetW(new_value), new_arr);
        }
    }

    SECTION("Length getter")
    {
        T length = 0;
        for(T component : raw_arr)
            length += component * component;
        length = static_cast<T>(std::sqrt(length));
        CHECK(raw_vec.GetLength() == length);
    }
}