/* ------------------------------------------------------------------   */
/*      item            : CyclicInt.cxx
        made by         : Rene' van Paassen
        date            : 990817
        category        : body file
        description     :
        changes         : 990817 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define CyclicInt_cc
#include "CyclicInt.hxx"
#include <iostream>
DUECA_NS_START

CyclicInt::CyclicInt(int value, int limit) :
  value(value % limit), limit(limit)
{
  if (this->value < 0) this->value += limit;
}

CyclicInt::~CyclicInt()
{
  //
}

CyclicInt::CyclicInt(const CyclicInt& o) :
  value(o.value),
  limit(o.limit)
{
  //
}

void CyclicInt::setLimit(int newlimit)
{
  limit = newlimit;
  if (value >= limit) value = value % limit;
}

CyclicInt& CyclicInt::operator=(int d)
{
  value = d % limit;
  if (value < 0) value += limit;
  return *this;
}

CyclicInt& CyclicInt::operator++()
{
  if (++value == limit) value = 0;
  return *this;
}

CyclicInt& CyclicInt::operator--()
{
  if (--value == -1) value += limit;
  return *this;
}

CyclicInt& CyclicInt::operator+=(int d)
{
  value += d;
  while (value < 0) value += limit;
  while (value >= limit) value -= limit;
  return *this;
}

CyclicInt CyclicInt::operator+(int d)
{
  return CyclicInt(value + d, limit);
}

CyclicInt CyclicInt::operator-(int d)
{
  return CyclicInt(value - d, limit);
}

bool CyclicInt::operator == (int d) const
{
  return value == d;
}

bool operator == (int d, const CyclicInt& a)
{
  return a.value == d;
}

bool CyclicInt::operator != (int d) const
{
  return value != d;
}

bool operator != (int d, const CyclicInt& a)
{
  return a.value != d;
}

std::ostream& CyclicInt::print (std::ostream& o) const
{
  return o << "CyclicInt(" << value << ',' << limit << ')';
}
DUECA_NS_END
