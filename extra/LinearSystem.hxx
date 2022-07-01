/* ------------------------------------------------------------------   */
/*      item            : LinearSystem.hxx
        made by         : Rene' van Paassen
        date            : 990415
        category        : header file
        description     :
        changes         : 990415 first version
                          010723 DUECA first version OS
                          020508 Move to mtl as matrix lib
                          090515 Added state limiting, and Integrator class
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef LinearSystem_hxx
#define LinearSystem_hxx

#ifndef USING_EIGEN3
#define USING_EIGEN3
#endif

#include <Eigen/Dense>

// a normal matrix, allocates its own storage
typedef Eigen::MatrixXd Matrix;
// a matrix that takes external storage
typedef Eigen::Map<Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor> > MatrixE;
// a normal vector, allocates its own storage
typedef Eigen::VectorXd Vector;
// a vector that takes external storage
typedef Eigen::Map<Eigen::VectorXd> VectorE;

/** This is an exception that can be thrown by the LinearSystem class. */
class LinSysException: public std::exception
{
  /** Description of this exception. */
  const char* reason;

public:
  /** Copy constructor. */
  LinSysException(const LinSysException& e) : reason(e.reason) {}

  /** With line number and file name */
  LinSysException(const char* file, int line, const char* r);

  /** Destructor. */
  virtual ~LinSysException() throw() {}

  /** Print out the reason for throwing this. */
  const char* what() const throw() { return reason;}
};


/** This class creates a linear time-invariant (control) system. The
    system is transformed to state-space form if necessary, and
    discretized with the parameter dt. */
class LinearSystem
{
protected:
  /** Order of the system */
  unsigned n;

  /** Number of outputs */
  unsigned m;

  /** Transition matrix. */
  Matrix Phi;

  /** Input matrix. */
  Matrix Psi;

  /** Output matrix. */
  Matrix C;

  /** Feedthrough matrix. */
  Matrix D;

  /** The state vector. */
  Vector x;

  /** The output vector. */
  Vector y;

private:
  /** Helper function, makes a discrete system from a continuous one. */
  void continuous2discrete(const Matrix& A, const Matrix& B,
                           Matrix& Phi, Matrix& Psi, double dt);

public:
  /** Default constructor. */
  LinearSystem();

  /** This constructs a SISO system, on the basis of continuous time
      transfer function.
      \param num   Numerator of the transfer function. Elements are
      arranged as num[0]*s^0 + num[1]*s^1 etc.
      \param den   Denominator.
      \param dt    Size, in seconds, of the discrete time step. */
  LinearSystem(const Vector& num, const Vector& den, double dt);

  /** Copy constructor. */
  LinearSystem(const LinearSystem &);

  /** This constructs a system, on the basis of a continuous time
      state space system.
      \param A     A matrix
      \param B     B matrix
      \param C     C matrix
      \param D     D matrix
      \param dt    Size, in seconds, of the discrete time step. */
  LinearSystem(const Matrix& A, const Matrix& B,
               const Matrix& C, const Matrix& D, double dt);

  /** This constructs a system, on the basis of a discrete time
      state space system.
      \param phi     phi matrix
      \param psi     psi matrix
      \param C     C matrix
      \param D     D matrix
  */
  LinearSystem(const Matrix& phi, const Matrix& psi,
               const Matrix& C, const Matrix& D);

  /** Destructor. */
  virtual ~LinearSystem();

  /** Assignment of one system to another. */
  LinearSystem& operator = (const LinearSystem& o);

  /** Accept a new state
      \param x_new  Vector with the new state. */
  virtual void acceptState(const Vector& x_new);

  /** Calculate a single time step, return the output vector.
      \param u      Input vector. */
  virtual const Vector& step(const Vector& u);

  /** Calculate a single time step, return the output vector.
      \param u      Input variable. Only valid for SI systems. */
  virtual const Vector& step(double u);

  /** Reset the state to 0; */
  virtual void reset();

  /** Create a system from numerator, denominator input. */
  void createFromNumDen(const Vector& num, const Vector& den, double dt);

  /** Create from matrices. */
  void createFromABCD(const Matrix& A, const Matrix& B,
                      const Matrix& C, const Matrix& D, double dt);

  /** Create from matrices, discrete */
  void createFromPhiPsiCD(const Matrix& phi, const Matrix& psi,
                          const Matrix& C, const Matrix& D);

  /** Obtain the output vector. */
  inline const Vector& getY() const {return y;}

  /** Obtain the state vector. */
  inline const Vector& getX() const {return x;}

  /** Obtain the state vector for modification. */
  inline Vector& getX() {return x;}

  /** Additional output, transition. */
  inline const Matrix& getPhi() const {return Phi;}

  /** Input matrix discrete system. */
  inline const Matrix& getPsi() const {return Psi;}

  /** Output matrix. */
  inline const Matrix& getC() const {return C;}

  /** Feedthrough matrix. */
  inline const Matrix& getD() const {return D;}
};
#endif
