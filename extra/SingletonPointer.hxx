/* ------------------------------------------------------------------   */
/*      item            : SingletonPointer.hxx
        made by         : Rene van Paassen
        date            : 020506
        category        : header file
        description     :
        changes         : 020506 first version
        documentation   : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef SingletonPointer_hxx
#define SingletonPointer_hxx

#ifdef SingletonPointer_cxx
#endif

/** "Smart" pointer class for singletons. Each of the singleton
    references will point to one and the same object. This object
    should have a private default constructor, and a private
    destructor, and list SingletonPointer<MyClass> as its friend. This
    class is useful for controlling access to hardware devices.

    In your code body, add the static objects needed for this
    construction by using the following macro:
    \code
      SINGLETON_POINTER_STATIC(MyClass);
    \endcode
    Of course, replace "MyClass" by the name of your class.

    Note that this is not multi-thread proof. However, since any sane
    person will create and delete the singleton pointers in single
    thread mode (DUECA Module constructors and destructors), this
    should be no problem.

    You should not use the SingletonPointer constructor and destructor
    in functions, as this might lead to repeated construction and
    destruction of the singleton class.
*/
template<class T>
class SingletonPointer
{
private:
  /** Pointer to the object this smart pointer refers to. */
  static T* referred;

  /** Number of accesses by SingletonPointers */
  static int count;

public:
  /** Constructor. */
  SingletonPointer();

  /** Destructor. */
  ~SingletonPointer();

  /** Obtaining a normal pointer. */
  inline T* operator -> () {return referred;}

  /** Obtaining a normal pointer for a const object. */
  inline const T* operator -> () const {return referred;}
};

template<class T>
SingletonPointer<T>::SingletonPointer()
{
  if (!(count++)) {
    referred = new T();
  }
}

template<class T>
SingletonPointer<T>::~SingletonPointer()
{
  if (!(--count)) {
    delete referred;
  }
}

#define SINGLETON_POINTER_STATIC(C) \
template<> C* SingletonPointer<C>::referred = NULL; \
template<> int SingletonPointer<C>::count = 0

#endif


#ifdef TEST

#include <iostream>

class Tested;

SINGLETON_POINTER_STATIC(Tested);

class Tested
{
  friend SingletonPointer<Tested>;
  static int count;
  Tested() :counter(count) { cout << "made " << ++count << endl; }
  ~Tested() { cout << "deleted " << count-- << endl; }
public:
  int counter;
};

int Tested::count;

main()
{
  cout << 1 << endl;
  SingletonPointer<Tested> sp1;

  cout << "exists" << sp1->counter << endl;

  cout << 2 << endl;
  SingletonPointer<Tested> sp2;

}
#endif
