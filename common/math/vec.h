#ifndef __vec_h
#define __vec_h

#include <cstddef>
#include <assert.h>
#include <initializer_list>


/*
This code implements an expression template pattern for vector operations to optimize 
performance by eliminating temporary object creation. Here's how it works step by step:

1. Expression Template Base Classes:
   - VecExpr<Derived> is the base class for all vector expressions
   - Each vector expression type inherits from VecExpr with itself as the template parameter
   - This uses the Curiously Recurring Template Pattern (CRTP) to enable static polymorphism

2. Vector Operations (e.g., a + b + c):
   When we write an expression like a + b + c, here's what happens:

   a) First Addition (a + b)[sum1]:
      - a is a Vec3, b is a Vec3
      - Vec3::operator+ creates a VecSum<Vec3, Vec3>
      - This VecSum stores references to a and b (into it's lhs and rhs member variables).
      - No computation happens yet

   b) Second Addition ((a + b) + c)[sum2]:
      - Left operand is VecSum<Vec3, Vec3> from step a
      - Right operand is Vec3 (c)
      - Creates VecSum<VecSum<Vec3, Vec3>, Vec3>
      - Stores references to the previous VecSum and c (lhs gets VecSum<Vec3, Vec3>(a+b) and rhs gets Vec3(c))
      - Still no computation

3. Memory Structure:
   For expression VecSum<VecSum<Vec3, Vec3>, Vec3> sum2:
   sum2
   ├── lhs → VecSum<Vec3, Vec3> => [ a+b ]
   │         ├── lhs → Vec3 (a)
   │         └── rhs → Vec3 (b)
   └── rhs → Vec3 (c)

4. Evaluation:
   Only happens when we assign to a Vec3 or access elements. For sum2[0]:
   sum2[0] = lhs[0] + rhs[0]; the code here accesses using .derived()[0]
                            ; derived() literally gives me the original template parameter that it was called with.
           = (VecSum<Vec3, Vec3>::operator[](0)) + c[0]
           = (a[0] + b[0]) + c[0]
           = (1 + 4) + 7
           = 12

Benefits:
- No temporary Vec3 objects are created during operations
- Computations are performed only when results are actually needed
- The compiler can optimize the entire expression tree
- Multiple operations can be fused into a single loop

Example Usage:
    Vec3 a = {1, 2, 3};
    Vec3 b = {4, 5, 6};
    Vec3 c = {7, 8, 9};
    Vec3 result = a + b + c;  // No temporaries created!
    // result[0] = 12 (1+4+7)
    // result[1] = 15 (2+5+8)
    // result[2] = 18 (3+6+9)

This pattern is particularly efficient for large vectors and complex expressions
where avoiding temporary objects can significantly improve performance.
*/

template<typename E1, typename E2>
class VecSum;

template <typename Derived> 
class VecExpr
{
  public:
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

class Vec3 : public VecExpr<Vec3>
{
    public:
    static constexpr size_t SIZE = 3;

    union {
        struct {double x, y, z;};
        double data[3];
    };

    Vec3() { x = 0.0; y = 0.0; z = 0.0; }
    Vec3(double x, double y, double z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }
    Vec3(std::initializer_list<double> list) {
        size_t i = 0;
        for (double value : list) {
            if (i < SIZE) {
                data[i] = value;
                ++i;
            } else break;
        }
    }

    template<typename E>
    Vec3 (const VecExpr<E>& expr) {
        for (size_t i = 0; i < Vec3::SIZE; ++i) {
            data[i] = expr.derived()[i];
        }
    }
    template<typename E>
    Vec3& operator=(const VecExpr<E>& expr) {
        for (size_t i = 0; i < Vec3::SIZE; ++i) {
            data[i] = expr.derived()[i];
        }
        return *this;
    }

    double operator[](size_t i) const {
        assert(i < SIZE);
        return data[i];
    }

    double& operator[](size_t i) {
        assert(i < SIZE);
        return data[i];
    }

    /* expression template */
    template <typename E>
    VecSum<Vec3, E> operator+(const VecExpr<E>& rhs) const {
        return VecSum<Vec3, E>(*this, rhs.derived());
    }
};

template<typename E1, typename E2>
class VecSum : public VecExpr<VecSum<E1, E2>> {
    /* 
     * the fact that these are references to the original vector positions, makes sure we don't create temporaries.
     */
    const E1& lhs;
    const E2& rhs;

  public:
    VecSum(const E1& l, const E2& r) : lhs(l), rhs(r) {}
    double operator[](size_t i) const { return lhs[i] + rhs[i]; }

    template <typename E>
    VecSum<VecSum<E1, E2>, E> operator+(const VecExpr<E>& rhs) const {
        return VecSum<VecSum<E1, E2>, E>(*this, rhs.derived());
    }
};

#include <iostream>
inline void driver()
{
    Vec3 a = Vec3({1, 2, 3});
    Vec3 b = {4,  5, 6 };
    Vec3 c = {7, 8, 9};
    Vec3 d = {10, 11, 12};

    auto expr = a+b+c+d;
    Vec3 e = expr;
    std::cout << "vector add result: {" << e.x << ", " << e.y << ", " << e.z << "}\n";
}
#endif