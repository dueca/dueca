/* ------------------------------------------------------------------   */
/*      item            : IndexedContainer.hxx
        made by         : Rene van Paassen
        date            : 010822
        category        : header file
        description     :
        changes         : 010822 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef IndexedContainer_hxx
#define IndexedContainer_hxx

#include <vector>
#include <iostream>
using namespace std;

#include <dueca_ns.h>
PRINT_NS_START
ostream& operator << (ostream& os, const IndexedContainer<T>& o);
PRINT_NS_END

DUECA_NS_START
template<class T> class IndexedContainer;
template<class T>

/** This class keeps and organises several client's status objects. */
template<class T>
class IndexedContainer
{
  /** vector with client's objects. */
  vector<T*> client_data;

public:
  /** constructor. */
  IndexedContainer();

  /** Destructor. */
  ~IndexedContainer();

  /** Clear stuff in the container bins. */
  void clear();

  /** add client data at a location. */
  void addClientData(int idx, const T& cdata);

  /** update client data at a location.
      \param idx      The client data used
      \param cdata    New data for the client
      \returns        true if the data was different, false when no
                      change is detected. */
  bool updateClientData(unsigned int idx, const T &cdata);

  /** update client data completely
      \param o        New client data
      \returns        true if the data was different, false when no
                      change is detected. */
  bool updateClientData(const IndexedContainer<T> &o);

  /** Test for equality, on the basis of data in containers. */
  bool operator == (const IndexedContainer<T> &) const;

  /** Access one of the client statuses. */
  T& operator [] (int idx);

  /** Combine two status objects. */
  IndexedContainer<T>& operator &= (const IndexedContainer<T>&);

  /** Print to a stream. */
  friend ostream& operator << <> (ostream& os, const IndexedContainer<T>& o);
};

template<class T>
IndexedContainer<T>::IndexedContainer()
{
  //
}

template<class T>
IndexedContainer<T>::~IndexedContainer()
{
  //
}

template<class T>
void IndexedContainer<T>::addClientData(int idx, const T& cdata)
{
  if (idx >= int(client_data.size())) {
    client_data.resize(idx+1, NULL);
  }
  client_data[idx] = cdata.clone();
}

template<class T>
T& IndexedContainer<T>::operator [](int idx)
{
  if (idx < 0 || idx >= client_data.size())
    return T::null();
  return client_data[idx];
}

template<class T>
void IndexedContainer<T>::clear()
{
  for (vector<T*>::iterator ii = client_data.begin();
       ii != client_data.end(); ii++) {
    (*ii)->clear();
  }
}

template<class T> IndexedContainer<T>&
IndexedContainer<T>::operator &= (const IndexedContainer<T>& o)
{
  assert(client_data.size() == o.client_data.size());
  for (int ii = client_data.size(); ii--; ) {
    *(client_data[ii]) &= *(o.client_data[ii]);
  }
  return *this;
}

template<class T> bool
IndexedContainer<T>::updateClientData(const IndexedContainer<T> &o)
{
  assert(client_data.size() == o.client_data.size());
  if (*this == o) return false;
  for (int ii = client_data.size(); ii--; ) {
    *(client_data[ii]) = *(o.client_data[ii]);
  }
  return true;
}

template<class T> bool
IndexedContainer<T>::updateClientData(unsigned int idx, const T& cdata)
{
  assert(idx < client_data.size());
  if (*client_data[idx] == cdata) return false;
  *(client_data[idx]) = cdata;
  return true;
}

template<class T> bool
IndexedContainer<T>::operator == (const IndexedContainer<T>& o) const
{
  if (this == &o) return true;
  if (client_data.size() != o.client_data.size()) return false;
  for (int ii = client_data.size(); ii--; ) {
    if (*(client_data[ii]) != *(o.client_data[ii])) return false;
  }
  return true;
}
DUECA_NS_END

PRINT_NS_START
template<class T>
ostream& operator << (ostream& os, const IndexedContainer<T>& o)
{
  os << "IndexedContainer(";
  for (unsigned int ii = 0; ii < o.client_data.size(); ii++) {
    os << 'd' << ii << '=';
    if (o.client_data[ii] == NULL) os << "NULL";
    else                           os << *(o.client_data[ii]);
    if (ii < o.client_data.size() - 1) os << ", ";
  }
  return os << ')';
}
PRINT_NS_END
#endif
