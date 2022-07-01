#if 0
#include <mtl/mtl.h>
#include <mtl/matrix.h>
#include <iostream>

using namespace mtl;
// a normal matrix, allocates its own storage
typedef matrix<double, rectangle<>, dense<>, row_major>::type Matrix;
// a matrix that takes external storage
typedef matrix<double, rectangle<>, dense<external>, row_major>::type MatrixE;
// a normal vector, allocates its own storage
typedef dense1D<double> Vector;
// a vector that takes external storage
typedef external_vec<double> VectorE;

int main()
{
  // shallow copy semantics
  // it is allowed to set an empty vector to a specific value; no
  // effect
  // an empty vector can be copied
  Vector v, v3, v4(5, 1.0);
  std::cout << v.size() << std::endl;
  Vector *v2;
  {
    Vector x(3, 1.0);
    v = x;
    x[1] = 2.0;
    v2 = new Vector(x);
    Vector x2;
    v3 = x2;
    v4 = x2;
  }
  mtl::set(v3, 1.0);
  print_vector(v);
  print_vector(*v2);
  print_vector(v3);
  print_vector(v4);
  delete v2;

  return 0;
}
#endif
