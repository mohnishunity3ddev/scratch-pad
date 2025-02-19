#ifndef __mat_h
#define __mat_h

#include <cstring>
#include <cassert>
#include <initializer_list>
#include <iostream>

template<typename E1, typename E2>
class Mat4Prod;
template<typename E1, typename E2>
class Mat4Add;

template <typename Derived>
class Mat4Expr {
public:
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

class Mat4 : public Mat4Expr<Mat4> {
  public:
    static constexpr size_t SIZE = 4;

    union {
        double data[16];
        double _data[4][4];
    };

    Mat4() { memset(data, 0, sizeof(data)); }

    Mat4(double m00, double m01, double m02, double m03,
         double m10, double m11, double m12, double m13,
         double m20, double m21, double m22, double m23,
         double m30, double m31, double m32, double m33) 
    {
        _data[0][0] = m00; _data[0][1] = m01; _data[0][2] = m02; _data[0][3] = m03;
        _data[1][0] = m10; _data[1][1] = m11; _data[1][2] = m12; _data[1][3] = m13;
        _data[2][0] = m20; _data[2][1] = m21; _data[2][2] = m22; _data[2][3] = m23;
        _data[3][0] = m30; _data[3][1] = m31; _data[3][2] = m32; _data[3][3] = m33;
    }

    Mat4(std::initializer_list<std::initializer_list<double>> list) {
        size_t i = 0;
        for (auto& row : list) {
            size_t j = 0;
            for (double value : row) {
                if (i < SIZE && j < SIZE) {
                    _data[i][j] = value;
                    ++j;
                }
            }
            ++i;
        }
    }

    template<typename E>
    Mat4(const Mat4Expr<E>& expr) {
        for (size_t i = 0; i < SIZE; ++i) {
            for (size_t j = 0; j < SIZE; ++j) {
                _data[i][j] = expr.derived()(i, j);
            }
        }
    }

    template<typename E>
    Mat4& operator=(const Mat4Expr<E>& expr) {
        for (size_t i = 0; i < SIZE; ++i) {
            for (size_t j = 0; j < SIZE; ++j) {
                _data[i][j] = expr.derived()(i, j);
            }
        }
        return *this;
    }

    double operator()(size_t i, size_t j) const {
        assert(i < SIZE && j < SIZE);
        return _data[i][j];
    }

    template <typename E>
    Mat4Prod<Mat4, E> operator*(const Mat4Expr<E>& rhs) const { return Mat4Prod<Mat4, E>(*this, rhs.derived()); }
    template <typename E>
    Mat4Add<Mat4, E> operator+(const Mat4Expr<E>& rhs) const { return Mat4Add<Mat4, E>(*this, rhs.derived()); }

    void print() const {
        for (size_t i = 0; i < SIZE; ++i) {
            for (size_t j = 0; j < SIZE; ++j) {
                std::cout << _data[i][j] << " ";
            }
            std::cout << "\n";
        }
        std::cout << "------------------\n";
    }
};

template<typename E1, typename E2>
class Mat4Prod : public Mat4Expr<Mat4Prod<E1, E2>> {
    const E1& lhs;
    const E2& rhs;

public:
    Mat4Prod(const E1& l, const E2& r) : lhs(l), rhs(r) {}

    double operator()(size_t i, size_t j) const {
        return lhs(i, 0) * rhs(0, j) +
               lhs(i, 1) * rhs(1, j) +
               lhs(i, 2) * rhs(2, j) +
               lhs(i, 3) * rhs(3, j);
    }

    // Support chaining: (A * B) * C
    template<typename E>
    Mat4Prod<Mat4Prod<E1, E2>, E> operator*(const Mat4Expr<E>& rhs) const { return Mat4Prod<Mat4Prod<E1, E2>, E>(*this, rhs.derived()); }
};

template<typename E1, typename E2>
class Mat4Add : public Mat4Expr<Mat4Add<E1, E2>> {
  public:
    const E1& lhs;
    const E2& rhs;

    Mat4Add(const E1& l, const E2& r) : lhs(l), rhs(r) {}

    double operator()(size_t i, size_t j) const { return lhs(i, j) + rhs(i, j); }
    
    template<typename E>
    Mat4Add<Mat4Add<E1, E2>, E> operator+(const Mat4Expr<E>& rhs) const { return Mat4Add<Mat4Add<E1, E2>, E>(*this, rhs.derived()); }
};

void mat4driver()
{
    Mat4 A = {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}, {13, 14, 15, 16}};
    Mat4 B = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
    Mat4 C = {{2, 3, 4, 5}, {6, 7, 8, 9}, {10, 11, 12, 13}, {14, 15, 16, 17}};
    Mat4 D = {{1, 1, 1, 1}, {2, 2, 2, 2}, {3, 3, 3, 3}, {4, 4, 4, 4}};

    std::cout << "mul result: \n";
    auto prod = A * B * C * D;
    Mat4 prodResult = prod;  
    prodResult.print();

    std::cout << "add result: \n";
    auto add = A + B + C + D;
    Mat4 addResult = add;
    addResult.print();
}

#endif