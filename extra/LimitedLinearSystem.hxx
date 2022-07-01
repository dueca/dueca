/* ------------------------------------------------------------------   */
/*      item            : LimitedLinearSystem.hxx
        made by         : Joost Ellerbroek
        date            : 090515
        category        : header file
        description     :
        changes         : 090515 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek/René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef LimitedLinearSystem_hxx
#define LimitedLinearSystem_hxx
#include "LinearSystem.hxx"

/** A specialization of LinearSystem that implements limits on the system state. */
class LimitedLinearSystem : public LinearSystem
{
public:
  /// Default constructor
  LimitedLinearSystem();

  /** This constructs a SISO system, on the basis of continuous time
      transfer function.
      \param num   Numerator of the transfer function. Elements are
      arranged as num[0]*s^0 + num[1]*s^1 etc.
      \param den   Denominator.
      \param dt    Size, in seconds, of the discrete time step. */
  LimitedLinearSystem(const Vector& num, const Vector& den, double dt);

  /** Copy constructor. */
  LimitedLinearSystem(const LimitedLinearSystem &);

  /** This constructs a system, on the basis of a continuous time
      state space system.
      \param A     A matrix
      \param B     B matrix
      \param C     C matrix
      \param D     D matrix
      \param dt    Size, in seconds, of the discrete time step. */
  LimitedLinearSystem(const Matrix& A, const Matrix& B,
                      const Matrix& C, const Matrix& D, double dt);

  /** Destructor. */
  virtual ~LimitedLinearSystem();

  /** Assignment of one system to another. */
  LimitedLinearSystem& operator = (const LimitedLinearSystem& o);

  /** Set the saturation limits for the states */
  void setSaturationLimits(const Vector& lower, const Vector& upper);

  /** Returns true if one or more of the states are saturated */
  inline bool isSaturated() const {return saturation;}

  /** Accept a new state
      \param x_new  Vector with the new state. */
  void acceptState(const Vector& x_new);

  /** Reset state, output, and saturation */
  void reset();

  /** Calculate a single time step, return the output vector.
      \param u      Input vector. */
  virtual const Vector& step(const Vector& u);

  /** Calculate a single time step, return the output vector.
      \param u      Input variable. Only valid for SI systems. */
  virtual const Vector& step(double u);

protected:
  /// Upper limit
  Vector ul;

  /// Lower limit
  Vector ll;

  /// Keep track of saturation
  bool saturation;
};
#endif
