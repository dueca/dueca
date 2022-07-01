/* ------------------------------------------------------------------   */
/*      item            : Interval.cxx
        made by         : Rene' van Paassen
        date            : 010402
        category        : body file
        description     :
        changes         : 010402 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define Interval_cxx
#include "Interval.hxx"

#ifdef BUILD_DMODULES

#define E_TRM
#define W_TRM
#define I_TRM
#define D_TRM
#include <debug.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <debprint.h>

PRINT_NS_START
ostream& operator << (ostream& os, const Matrix& x);
PRINT_NS_END

DUECA_NS_START

Interval::Interval(double lowx, double highx, int ny, int query_offset) :
  lowx(lowx),
  highx(highx),
  x(3),
  y(3, ny),
  query_offset(query_offset)
{
  DEB1("New interval, x0=" << lowx << ", x1=" << highx << ", qoff="
        << query_offset);
  x[0] = lowx;
  x[1] = 0.5 * (lowx + highx);
  x[2] = highx;
}

Interval::~Interval()
{
  //
}

void Interval::newRange(double range)
{
  // new side x-es
  x[0] = x[1] - range;
  x[2] = x[1] + range;
}

bool Interval::shrink(double sspeed, Vector& ymix)
{
  // calculate mixed y value at the three points
  Vector yc(y.rows());
  yc = y * ymix;
  //mult( y, ymix, yc);

  DEB1("for interval " << query_offset/3 << std::endl
        <<"at points    " << x[0] << ' ' << x[1] << ' ' << x[2] << std::endl
        << "    y values \n" << y << std::endl
        << "  y weighing " << std::setw(10) << ymix[0] << std::setw(10)
        << ymix[1] << std::setw(10) << ymix[2]
        << std::endl
        << "mix y values " << yc[0] << ' ' << yc[1]
        << ' ' << yc[2]);

  // keep the range we had now.
  double range = 0.5*(x[2] - x[0]);

  // re-center based on mixed y. The calculation prefers high values
  // for x
  for (int ii = 2; ii--;) {
    if (yc[ii+1] * yc[ii] < 0.0) {

      // new center x
      x[1] = x[ii] + (x[ii+1] - x[ii])*(0.0 - yc[ii])/(yc[ii+1] - yc[ii]);

      // success in shrinking
      newRange(range/sspeed);
      DEB1("for interval " << query_offset/3 << " looking at " <<
            ii << std::endl
            << "Range right, new range " << x[0] << ' ' << x[2]);
      return true;
    }
    else if (yc[ii+1] == 0.0) {
      x[1] = x[ii+1];

      // success in shrinking
      newRange(range/sspeed);
      DEB1("for interval " << query_offset/3 << std::endl
           << "Range at hi/mid side, new range "
           << x[0] << ' ' << x[2]);
      return true;
    }
    else if (yc[ii] == 0.0) {
      x[1] = x[ii];

      // success in shrinking
      newRange(range/sspeed);
      DEB1("for interval " << query_offset/3 << std::endl
            << "Range at low side, new range " << x[0] << ' ' << x[2]);
      return true;
    }
  }

  // could not shrink, apparently, try expanding
  range *= sspeed;
  x[0] = std::max(x[1] - range, lowx);
  x[2] = std::min(x[1] + range, highx);
  x[1] = 0.5 * (x[2] + x[0]);
  DEB1("for interval " << query_offset/3 << std::endl
        << "Expansion " << x[0] << ' ' << x[2]);
  return false;
}

double Interval::getX(int ii)
{
  if (ii < query_offset || ii > query_offset + 2) return x[1];
  return x[ii-query_offset];
}

bool Interval::mergeResult(int ii, Vector& y_in)
{
  // result 1, the "center", is always accepted
  if (ii == 1) {
    y.col(ii) = y_in;
    //copy(y_in, rows(y)[ii]);
    return true;
  }

  if (ii < query_offset || ii > query_offset + 2) return false;

  // for me
  //copy(y_in, rows(y)[ii-query_offset]);
  y.col(ii-query_offset) = y_in;
  DEB1("for interval " << query_offset/3 << std::endl
        << "inserting for x=" << x[ii-query_offset] << std::endl
        << std::setw(10) << y_in[0]
        << std::setw(10) << y_in[1] << std::setw(10) << y_in[2]);
  return true;
}

void Interval::getResult(Vector& yr)
{
  //copy(rows(y)[1], yr);
  yr = y.col(1);
}

void Interval::getRange(Vector& yr)
{
  //add(rows(y)[2], scaled(rows(y)[0], -1.0), yr);
  yr = y.col(2) - y.col(0);
  DEB1("for interval " << query_offset/3 << std::endl
        << "returning range " << std::setw(10) << yr[0]
        << std::setw(10) << yr[1] << std::setw(10) << yr[2]);
}
DUECA_NS_END

#endif
