/* ------------------------------------------------------------------   */
/*      item            : LinearSystem.cxx
        made by         : Rene' van Paassen
        date            : 990415
        category        : body file
        description     :
        changes         : 990415 first version
                          010723 DUECA first version OS
                          020508 Move to mtl as matrix lib
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define LinearSystem_cxx
#include "LinearSystem.hxx"
#include <iostream>
//#define TEST
#ifdef TESTLINEARSYSTEM
#define DDEB(A) cout << A << endl
#else
#ifdef TEST
#define DDEB(A) cout << A << endl
#else
#define DDEB(A)
#endif
#endif
#include <cstdio>

#ifdef HAVE_EIGEN3_UNSUPPORTED_EIGEN_MATRIXFUNCTIONS
#include <unsupported/Eigen/MatrixFunctions>
#endif

using namespace std;

static const char *e_num_den_size =
"numerator > denominator, or den zero size";
static const char *state_space_dim =
"invalid dimensions of state-space system";

static char LSEbuff[256];
LinSysException::LinSysException(const char* file, int line, const char* r) :
  reason(LSEbuff)
{
  snprintf(LSEbuff, 256, "%s:%i %s", file, line, r);
}

LinearSystem::LinearSystem(const Vector& num, const Vector& den,
                           double dt) :
  n(den.size() - 1),
  m(1),
  Phi(n,n),
  Psi(n,m),
  C(m,n),
  D(m,m),
  x(n),
  y(1)
{
  createFromNumDen(num, den, dt);
}

LinearSystem::LinearSystem(const Matrix& A, const Matrix& B,
                           const Matrix& Cin, const Matrix& Din, double dt) :
  n(A.rows()),
  m(B.cols()),
  Phi(n,n),
  Psi(n,m),
  C(m,n),
  D(m,m),
  x(n),
  y(Cin.rows())
{
  createFromABCD(A, B, Cin, Din, dt);
}

LinearSystem::LinearSystem(const Matrix& Phi, const Matrix& Psi,
                           const Matrix& Cin, const Matrix& Din) :
  n(Phi.rows()),
  m(Psi.cols()),
  Phi(n,n),
  Psi(n,m),
  C(Cin.rows(), n),
  D(Cin.rows(), m),
  x(n),
  y(Cin.rows())
{
  this->Phi = Phi;
  this->Psi = Psi;
  this->C = Cin;
  this->D = Din;
  x.setZero(); y.setZero();
}

LinearSystem::LinearSystem(const LinearSystem& o) :
  n(o.n),
  m(o.m),
  Phi(o.Phi),
  Psi(o.Psi),
  C(o.C),
  D(o.D),
  x(o.x),
  y(o.y)
{
  //
}

LinearSystem::LinearSystem() :
  n(0),
  m(0),
  Phi(),
  Psi(),
  C(),
  D(),
  x(),
  y()
{
  //
}

LinearSystem::~LinearSystem()
{
  // matrixes take care of their own destruction
}

void LinearSystem::createFromNumDen(const Vector& num, const Vector& den, double dt)
{
  DDEB("creating linear system from num, den");
  n = den.size() - 1;   // order of the denominator
  while (n > 0 && den[n] == 0.0) n--;
  m = num.size() - 1;   // order of the numerator
  while (m > 0 && num[m] == 0.0) m--;
  if (m > n || (n == 0 && den[0] == 0.0)) {
    throw(LinSysException(__FILE__, __LINE__, e_num_den_size));
  }


  if (n == 0) {
    // special case, just a gain, but the calculations don't go well
    // with empty matrices ...
    Phi.resize(1,1); Phi.setZero();
    Psi.resize(1,1); Psi.setZero();
    C.resize(1,1); C.setZero();
    D.resize(1,1); D(0,0) = num[0]/den[0];
    x.resize(1); x.setZero();
    y.resize(1); y.setZero();
    return;
  }

  // if not needed, these are no-ops
  Phi.resize(n,n);
  Psi.resize(n,1);
  C.resize(1,n); C.setZero();
  D.resize(1,1); D.setZero();
  x.resize(n); x.setZero();
  y.resize(n); y.setZero();

  // to simplify, call coefficients a, b, and calculate feedthru d
  double d = (n == m) ? num[n]/den[n] : 0.0;
  Vector a = Vector::Zero(n+1);
  Vector b = Vector::Zero(n+1);
  for (int ii = n+1; ii--; ) {
    a[ii] = den[ii]/den[n];
  }
  for (int ii = m+1; ii--; ) {
    b[ii] = num[ii]/den[n];
  }
  DDEB("a = " << a);
  DDEB("b = " << b);
  DDEB("d = " << d);

  // A and B matrix represent the continuous state space form
  Matrix A = Matrix::Zero(n, n);
  Matrix B = Matrix::Zero(n, 1);// Initialize matrices to zero

  // http://www.engr.mun.ca/~millan/Eng6825/canonicals.pdf
  // fill A and C
  for (unsigned ii = 0; ii < n; ii++) {
    A(ii,n-1) = -a[ii];
    B(ii,0) =  b[ii] - a[ii]*d;
  }
  for (unsigned ii = 0; ii < n-1; ii++) A(ii+1,ii) = 1.0;

  // fill B and D
  C(0,n-1) = 1.0;
  D(0,0) = d;
#ifdef TEST
  DDEB("A = " << A);
  DDEB("B = " << B);
  DDEB("C = " << C);
  DDEB("D = " << D);
#endif

  // get a discrete form
  continuous2discrete(A, B, Phi, Psi, dt);
}

LinearSystem& LinearSystem::operator=(const LinearSystem & rhs)
{
  if (&rhs != this) {
    n = rhs.n;
    m = rhs.m;
    if (n && m) {
      Phi.resize(n, n); Phi = rhs.Phi;
      Psi.resize(n, m); Psi = rhs.Psi;
      C.resize(rhs.C.rows(), n); C   = rhs.C;
      D.resize(rhs.D.rows(), m); D   = rhs.D;
      x.resize(n); x   = rhs.x;
      y.resize(rhs.C.rows()); y   = rhs.y;
    }
    else {/*
      Phi = Matrix();
      Psi = Matrix();
      C = Matrix();
      D = Matrix();
      x = Vector();
      y = Vector(); */
    }
  }
  return *this;
}

void LinearSystem::createFromABCD(const Matrix& Ain, const Matrix& Bin,
                                  const Matrix& Cin, const Matrix& Din,
                                  double dt)
{
  if (Ain.cols() != Ain.rows() ||   // check A square
      Bin.rows() != Ain.rows() ||   // check no rows B = no states
      Cin.cols() != Ain.rows() ||   // check no cols C = no states
      Cin.rows() != Din.rows() ||   // check no outputs C and D equal
      Din.cols() != Bin.cols()) {   // check no inputs B and D equal
    throw(LinSysException(__FILE__, __LINE__, state_space_dim));
  }

  C = Cin;
  D = Din;
  n = Ain.rows();
  m = Cin.rows();
  x.resize(n); x.setZero();
  y.resize(m); y.setZero();
  continuous2discrete(Ain, Bin, Phi, Psi, dt);

#ifdef TEST
  DDEB("Phi = " << Phi);
  DDEB("Psi = " << Psi);
  DDEB("C = " << C);
  DDEB("D = " << D);
#endif
}

void LinearSystem::createFromPhiPsiCD(const Matrix& Ain, const Matrix& Bin,
                                      const Matrix& Cin, const Matrix& Din)
{
  if (Ain.cols() != Ain.rows() ||   // check A square
      Bin.rows() != Ain.rows() ||   // check no rows B = no states
      Cin.cols() != Ain.rows() ||   // check no cols C = no states
      Cin.rows() != Din.rows() ||   // check no outputs C and D equal
      Din.cols() != Bin.cols()) {   // check no inputs B and D equal
    throw(LinSysException(__FILE__, __LINE__, state_space_dim));
  }

  Phi = Ain;
  Psi = Bin;
  C = Cin;
  D = Din;
  n = Ain.rows();
  m = Cin.rows();
  x.resize(n); x.setZero();
  y.resize(m); y.setZero();
}

void LinearSystem::continuous2discrete(const Matrix& A, const Matrix& B,
                                       Matrix& Phi, Matrix& Psi, double dt)
{
  DDEB("continuous to discrete");
  if (A.rows() != Phi.rows()) {
    Phi = Matrix(A.rows(), A.rows());
    Psi = Matrix(A.rows(), B.cols());
  }
  assert(A.cols() == Phi.cols());
  int n = A.rows(), ns = A.cols() + B.cols();

  DDEB("A = " << A);
  DDEB("B = " << B);

  Matrix s = Matrix::Zero(ns, ns);// Initialize to zero

  //  copy(A, s.sub_matrix(0, n-1, 0, n-1));
  for (int ii = A.rows(); ii--; ) {
    for (int jj = A.cols(); jj--; ) s(ii, jj) = A(ii, jj);
    for (int jj = B.cols(); jj--; ) s(ii, jj+n) = B(ii, jj);
  }
  s *= dt;

#ifdef HAVE_EIGEN3_UNSUPPORTED_EIGEN_MATRIXFUNCTIONS
  Matrix g = Matrix::Zero(ns, ns);
  Eigen::MatrixExponential<Matrix> e(s);
  e.compute(g);
#else

  // working matrices
  Matrix g = Matrix::Identity(ns, ns) + s,
        s2 = s;

  for (int ii = 2; ii < 100; ii++) {
    s2 *= (s / double(ii));
    g  += s2;
  }
#endif

  //  copy(g.sub_matrix(0, n-1, 0, n-1), Phi);
  //  copy(g.sub_matrix(0, n-1, n, n+B.cols()), Psi);
  for (int ii = n; ii--; ) {
    for (int jj = n; jj--; ) Phi(ii, jj) = g(ii, jj);
    for (int jj = B.cols(); jj--; ) Psi(ii, jj) = g(ii, n+jj);
  }
}

void LinearSystem::reset()
{
  x.setZero();
  y.setZero();
}

void LinearSystem::acceptState(const Vector& x_new)
{
  // Output equation, will include input direct-feedthrough
  y += C * (x_new - x);

  // Copy state
  x = x_new;
}

const Vector& LinearSystem::step(const Vector& u)
{
  x = Phi * x + Psi * u;
  y = C * x + D * u;
  return y;
}

const Vector& LinearSystem::step(double u)
{
  VectorE vu(&u, 1); // vu << u;
  return step(vu);
}

#ifdef TESTLINEARSYSTEM

int main(int argc, const char* argv[])
{
  {
  Vector num(2), den(2);
  num << 1.0, 0.0;
  den << 1.0, 5.0;

  LinearSystem l(num, den, 0.01);

  //  l.createFromNumDen(num, den, 0.01);

  std::cout << "Phi = " << l.getPhi() << std::endl;
  std::cout << "Psi = " << l.getPsi() << std::endl;
  std::cout << "C = " << l.getC() << std::endl;
  std::cout << "D = " << l.getD() << std::endl;

  // try with a sine input
  for (int ii = 0; ii < 1000; ii++) {
    cout << l.getY()[0] << endl;
    l.step(sin(double(ii)/100.0));
  }
  }

  {
    Matrix A(3,3);
    A <<  -2.439,    2.337,   -1.776,
      -2.933,   -1.096,    4.221,
      0.09223,   -4.579,   -1.537;
    Matrix B(3,2);
    B <<   0.8886,        0,
      -0.7648,        0,
      -1.402,  -0.1774;
    Matrix C(2,3);
    C <<  -0.1961,   0.2916,    1.588,
      1.419,   0.1978,        0;
    Matrix D(2,2);
    D <<  0.6966,  -0.2437,
      0,        0;
    LinearSystem l(A, B, C, D, 0.1);

    /* result should resemble:
  a =
             x1        x2        x3
   x1    0.7524    0.2207  -0.09825
   x2   -0.2333    0.7807    0.3753
   x3   0.06268   -0.3829    0.7714

  b =
              u1         u2
   x1    0.07789   0.001097
   x2    -0.1083  -0.003491
   x3    -0.1081   -0.01591
    */
  std::cout << "Phi = " << l.getPhi() << std::endl;
  std::cout << "Psi = " << l.getPsi() << std::endl;
  std::cout << "C = " << l.getC() << std::endl;
  std::cout << "D = " << l.getD() << std::endl;

  LinearSystem l2;
  l2.createFromABCD(A, B, C, D, 0.1);
  std::cout << "Phi2 = " << l2.getPhi() << std::endl;
  std::cout << "Psi2 = " << l2.getPsi() << std::endl;
  std::cout << "C2 = " << l2.getC() << std::endl;
  std::cout << "D2 = " << l2.getD() << std::endl;
  assert(l2.getY().size() == 2);
  assert(l2.getX().size() == 3);

  Vector U(2); U.setZero();
  Vector x(3); x.setZero();
  x[0]=0.2;
  l.acceptState(x);
  // try with a sine input
  for (int ii = 0; ii < 1000; ii++) {
    cout << l.getY() << endl;
    U[0] = sin(double(ii)/100.0);
    l.step(U);
  }
  }
}




#endif
