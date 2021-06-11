/**
Distributed under MIT style license:
-------------------------------------
Copyright (c) 2021 Janaka Subhawickrama

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

#include <array>
#include <assert.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <span>

#include "gtest/gtest.h"
#include <stdint.h>

/**
 * |----========---------|
 *      ^       ^
 *      Back    Front
 */

/**
 * DataType needs to be a POD type or class.
 * Warning: RingBuffer of complex classes with Vtables may cause unintended
 * results when memset is used to clear values.  Use std::fill instead for
 * compilers that supports C++20
 */
template <typename DataType>
class RingBuffer
{
  const uint16_t m_elements;
  std::span<DataType> m_ringbuffer;
  uint16_t m_front; // Where next new element will be stored
  uint16_t m_back; // Where last element will be deleted from
  uint16_t m_count;

  void AdvanceIndex(uint16_t& ix)
  {
    if(++ix >= m_elements)
    {
      ix = 0;
    }
  }

  public:
  RingBuffer(const uint16_t size)
      : m_elements(size)
      , m_front(0)
      , m_back(0)
      , m_count(0)
  {
    m_ringbuffer = {new DataType[m_elements], m_elements};
  }

  ~RingBuffer()
  {
    delete[] m_ringbuffer.data();
  }

  uint16_t GetFrontIx()
  {
    return m_front;
  }

  uint16_t GetBackIx()
  {
    return m_back;
  }

  uint16_t GetCount()
  {
    return m_count;
  }

  uint16_t GetMaxElements()
  {
    return m_elements;
  }

  void Print(const char* comment)
  {
    for(auto& ptr : m_ringbuffer)
    {
      std::cout << "[" << ptr << "]";
    }
    std::cout << " F=" << m_front << ",B=" << m_back;
    std::cout << " " << comment << std::endl;
  }

  std::span<DataType> GetBuffer()
  {
    return m_ringbuffer;
  }

  void PopulateAll()
  {
    for(int i = 1; i <= m_elements; i++)
    {
      Add(i);
    }
  }

  bool Add(DataType val)
  {
    m_ringbuffer[m_front] = val;
    AdvanceIndex(m_front);
    m_count++;
    if(m_count > m_elements)
    {
      // Next insertion position is the back and contains data
      AdvanceIndex(m_back);
      m_count--;
      return false;
    }
    else
    {
      return true;
    }
  }

  bool Remove(void)
  {
    DataType tmpVal;
    return Remove(tmpVal);
  }

  bool Remove(DataType& outVal)
  {
    if(m_count > 0)
    {
      outVal = m_ringbuffer[m_back];
      std::memset(
          static_cast<void*>(&m_ringbuffer[m_back]), 0, sizeof(DataType));
      AdvanceIndex(m_back);
      m_count--;
      return true;
    }
    else
    {
      std::memset(static_cast<void*>(&outVal), 0, sizeof(DataType));
      return false;
    }
  }
};

int main()
{
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}

class ringbuffer_test_cases : public ::testing::Test
{
  public:
  RingBuffer<int> ringbuf;

  ringbuffer_test_cases()
      : ringbuf(10)
  {
    // initialization code here
  }

  void SetUp()
  {
    // code here will execute just before the test ensues
    ringbuf.PopulateAll();
  }

  void TearDown()
  {
    // code here will be called just after the test completes
    // ok to through exceptions from here if need be
  }

  ~ringbuffer_test_cases()
  {
    // cleanup any pending stuff, but no exceptions allowed
  }

  // put in any custom data members that you need
};

TEST_F(ringbuffer_test_cases, testPopulationTen)
{
  ringbuf.Print("Full");
  int a[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  ASSERT_FALSE(std::memcmp(a, ringbuf.GetBuffer().data(), 10));
  ASSERT_EQ(ringbuf.GetFrontIx(), 0);
  ASSERT_EQ(ringbuf.GetBackIx(), 0);
  ASSERT_EQ(ringbuf.GetCount(), 10);
}

TEST_F(ringbuffer_test_cases, testWrap1)
{
  ringbuf.Add(11);
  ringbuf.Print("wrap1");
  int a[] = {11, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  ASSERT_FALSE(std::memcmp(a, ringbuf.GetBuffer().data(), 10));
  ASSERT_EQ(ringbuf.GetFrontIx(), 1);
  ASSERT_EQ(ringbuf.GetBackIx(), 1);
  ASSERT_EQ(ringbuf.GetCount(), 10);
}

TEST_F(ringbuffer_test_cases, testWrap2)
{
  ringbuf.Add(11);
  ringbuf.Add(12);
  ringbuf.Print("wrap2");
  int a[] = {11, 12, 3, 4, 5, 6, 7, 8, 9, 10};
  ASSERT_FALSE(std::memcmp(a, ringbuf.GetBuffer().data(), 10));
  ASSERT_EQ(ringbuf.GetFrontIx(), 2);
  ASSERT_EQ(ringbuf.GetBackIx(), 2);
  ASSERT_EQ(ringbuf.GetCount(), 10);
}

TEST_F(ringbuffer_test_cases, testRemove1)
{
  ringbuf.Add(11);
  ringbuf.Add(12);
  ringbuf.Remove();
  ringbuf.Print("Remove1");
  int a[] = {11, 12, 0, 4, 5, 6, 7, 8, 9, 10};
  ASSERT_FALSE(std::memcmp(a, ringbuf.GetBuffer().data(), 10));
  ASSERT_EQ(ringbuf.GetFrontIx(), 2);
  ASSERT_EQ(ringbuf.GetBackIx(), 3);
  ASSERT_EQ(ringbuf.GetCount(), 9);
}

TEST_F(ringbuffer_test_cases, testRemove2)
{
  ringbuf.Add(11);
  ringbuf.Add(12);
  ringbuf.Remove();
  ringbuf.Remove();
  ringbuf.Print("Remove2");
  int a[] = {11, 12, 0, 0, 5, 6, 7, 8, 9, 10};
  ASSERT_FALSE(std::memcmp(a, ringbuf.GetBuffer().data(), 10));
  ASSERT_EQ(ringbuf.GetFrontIx(), 2);
  ASSERT_EQ(ringbuf.GetBackIx(), 4);
  ASSERT_EQ(ringbuf.GetCount(), 8);
}

TEST_F(ringbuffer_test_cases, testFillRemoved)
{
  ringbuf.Add(11);
  ringbuf.Add(12);
  ringbuf.Remove();
  ringbuf.Remove();
  ringbuf.Add(13);
  ringbuf.Print("FillRemoved");
  int a[] = {11, 12, 13, 0, 5, 6, 7, 8, 9, 10};
  ASSERT_FALSE(std::memcmp(a, ringbuf.GetBuffer().data(), 10));
  ASSERT_EQ(ringbuf.GetFrontIx(), 3);
  ASSERT_EQ(ringbuf.GetBackIx(), 4);
  ASSERT_EQ(ringbuf.GetCount(), 9);
}

TEST_F(ringbuffer_test_cases, B4RemoveWrap)
{
  for(int i = 1; i < ringbuf.GetBuffer().size(); i++)
  {
    ringbuf.Remove();
  }
  ringbuf.Print("B4RemoveWrap");
  int a[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 10};
  ASSERT_FALSE(std::memcmp(a, ringbuf.GetBuffer().data(), 10));
  ASSERT_EQ(ringbuf.GetFrontIx(), 0);
  ASSERT_EQ(ringbuf.GetBackIx(), 9);
  ASSERT_EQ(ringbuf.GetCount(), 1);
}

TEST_F(ringbuffer_test_cases, RemoveWrap1)
{
  for(int i = 0; i < ringbuf.GetBuffer().size(); i++)
  {
    ringbuf.Remove();
  }
  ringbuf.Print("RemoveWrap1");
  int a[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  ASSERT_FALSE(std::memcmp(a, ringbuf.GetBuffer().data(), 10));
  ASSERT_EQ(ringbuf.GetFrontIx(), 0);
  ASSERT_EQ(ringbuf.GetBackIx(), 0);
  ASSERT_EQ(ringbuf.GetCount(), 0);
}

TEST_F(ringbuffer_test_cases, RemovePastFront)
{
  ringbuf.Add(11);
  for(int i = 0; i <= ringbuf.GetBuffer().size(); i++)
  {
    ringbuf.Remove();
  }
  ringbuf.Print("RemovePastFront");
  int a[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  ASSERT_FALSE(std::memcmp(a, ringbuf.GetBuffer().data(), 10));
  ASSERT_EQ(ringbuf.GetFrontIx(), 1);
  ASSERT_EQ(ringbuf.GetBackIx(), 1);
  ASSERT_EQ(ringbuf.GetCount(), 0);
}

TEST_F(ringbuffer_test_cases, InsertLargeAmount)
{
  for(int i = 0; i < 999; i++)
  {
    ringbuf.Add(i);
  }
  ringbuf.Print("RemovePastFront");
  int a[] = {990, 991, 992, 993, 994, 995, 996, 997, 998, 989};
  ASSERT_FALSE(std::memcmp(a, ringbuf.GetBuffer().data(), 10));
  ASSERT_EQ(ringbuf.GetFrontIx(), 9);
  ASSERT_EQ(ringbuf.GetBackIx(), 9);
  ASSERT_EQ(ringbuf.GetCount(), 10);
}

template <typename T>
class tmpClass2
{
  public:
  T three;
  int four;
};

TEST_F(ringbuffer_test_cases, testComplexClass)
{
  struct tmpClass
  {
    int one;
    int two;

    tmpClass()
        : two(0)
        , one(0){};
  };

  RingBuffer<tmpClass2<tmpClass>> ringbuf2(10);

  tmpClass2<tmpClass> element, element2;

  element.three.one = 7;
  element.three.two = 8;
  ringbuf2.Add(element);

  std::cout << "TestComplex.One:" << ringbuf2.GetBuffer().data()[0].three.one
            << std::endl;
  std::cout << "TestComplex.Two:" << ringbuf2.GetBuffer().data()[0].three.two
            << std::endl;

  ringbuf2.Remove(element2);
}