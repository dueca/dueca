/* ------------------------------------------------------------------   */
/*      item            : GtkCaller.hxx
        made by         : Rene van Paassen
        date            : 051017
        category        : header file
        description     : This file provides template classes and
                          functions for interfacing with Gtk libglade
                          windows.
        api             :
        changes         : 051017 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef GtkCaller_hxx
#define GtkCaller_hxx

#include <gtk/gtk.h>
#if GTK_MAJOR_VERSION >= 2
#ifndef GtkSignalFunc
#define GtkSignalFunc GCallback
#endif
#endif

#include <dueca_ns.h>

DUECA_NS_START;

/** Base class for the callback pointers. Objects of this class will be
    automatically generated from the gtk_callback templated
    functions. */
class GtkCaller
{
protected:
  gpointer gp;
public:
  /** Constructor. */
  GtkCaller();

  /** Destructor. */
  virtual ~GtkCaller();

  /** Clone call, creates a copied caller that carries a reference to
      an object. */
  virtual GtkCaller* clone(void* obj) const = 0;

  /** Return a pointer to the GtkCallback function. */
  virtual GtkSignalFunc callback() = 0;

  /** Pass this pointer as user data to gtk signal */
  virtual gpointer user_data() {return reinterpret_cast<const gpointer>(this);}

  /** Set the value of the gpointer member. */
  void setGPointer(gpointer g);
};


/** 11111111111111111111111111111111111111111111111111111111111  */

/** Callback implementation class, for calls with one parameter. */
template<class T, typename RET, typename P1>
class GtkCallerImp1: public GtkCaller
{
  /** pointer to the member function. */
  RET (T:: *call) (P1, gpointer);

  /** pointer to the instantiation of the class. Is NULL when acting
      as a generic object in the table. */
  T* obj;

  /** pointer to the c-linkage callback used. */
  static RET (*base) (P1, gpointer);

public:
  /** Constructor.
      \param  call pointer to the callback function. */
  GtkCallerImp1(RET (T:: *call) (P1, gpointer), gpointer obj=NULL) :
    call(call), obj(reinterpret_cast<T*>(obj)) { }

  /** Implementing constructor. */
  GtkCallerImp1(T* obj, const GtkCallerImp1& o) : call(o.call), obj(obj) { }

  /** Full constructor */
  GtkCallerImp1(T* obj, RET (T:: *call) (P1, gpointer)) :
  call(call), obj(obj) { }

  /** Clone with object specialization. */
  GtkCaller* clone(void* obj) const override
  { return new GtkCallerImp1(reinterpret_cast<T*>(obj), *this); }

  /** Execute call. */
  RET operator () (P1 a) const
  { return (obj ->* call)(a, gp); }

  /** Return a pointer to the GtkCallback function. */
  GtkSignalFunc callback()
  { return reinterpret_cast<GtkSignalFunc>(base); }
};

/** c-linkage call with one parameter. */
template <class T, typename RET, typename P1>
static RET GtkCaller_callback(P1 g1, gpointer gp)
{
  return (*reinterpret_cast<GtkCallerImp1<T,RET,P1>*>(gp))(g1);
}

/** static member */
template<class T, typename RET, typename P1>
RET (* GtkCallerImp1<T,RET,P1>::base) (P1, gpointer) =
  &GtkCaller_callback<T,RET,P1>;


/** 22222222222222222222222222222222222222222222222222222222222  */

/** Callback implementation class, for calls with one parameter. */
template<class T, typename RET, typename P1, typename P2>
class GtkCallerImp2: public GtkCaller
{
  /** pointer to the member function. */
  RET (T:: *call) (P1, P2, gpointer);

  /** pointer to the instantiation of the class. Is NULL when acting
      as a generic object in the table. */
  T* obj;

  /** pointer to the c-linkage callback used. */
  static RET (*base) (P1, P2, gpointer);

public:
  /** Constructor.
      \param  call pointer to the callback function. */
  GtkCallerImp2(RET (T:: *call) (P1, P2, gpointer), gpointer obj=NULL) :
    call(call), obj(reinterpret_cast<T*>(obj)) { }

  /** Implementing constructor. */
  GtkCallerImp2(T* obj, const GtkCallerImp2& o) : call(o.call), obj(obj) { }

  /** Full constructor */
  GtkCallerImp2(T* obj, RET (T:: *call) (P1, P2, gpointer)) :
    call(call), obj(obj) { }

  /** Clone with object specialization. */
  GtkCaller* clone(void* obj) const override
  { return new GtkCallerImp2(reinterpret_cast<T*>(obj), *this); }

  /** Execute call. */
  RET operator () (P1 a, P2 b)
  { return (obj ->* call)(a, b, gp); }

  /** Return a pointer to the GtkCallback function. */
  GtkSignalFunc callback()
  { return reinterpret_cast<GtkSignalFunc>(base); }
};

/** c-linkage call. */
template<class T, typename RET, typename P1, typename P2>
static RET GtkCaller_callback(P1 g1, P2 g2, gpointer gp)
{
  return (*reinterpret_cast<GtkCallerImp2<T,RET,P1,P2>*>(gp))(g1, g2);
}

/** static member */
template<class T, typename RET, typename P1, typename P2>
RET (* GtkCallerImp2<T,RET,P1,P2>::base) (P1, P2, gpointer) =
  &GtkCaller_callback<T,RET,P1,P2>;


/** 33333333333333333333333333333333333333333333333333333333333  */

/** Callback implementation class, for calls with one parameter. */
template<class T, typename RET, typename P1, typename P2, typename P3>
class GtkCallerImp3: public GtkCaller
{
  /** pointer to the member function. */
  RET (T:: *call) (P1, P2, P3, gpointer);

  /** pointer to the instantiation of the class. Is NULL when acting
      as a generic object in the table. */
  T* obj;

  /** pointer to the c-linkage callback used. */
  static RET (*base) (P1, P2, P3, gpointer);

public:
  /** Constructor.
      \param  call pointer to the callback function. */
  GtkCallerImp3(RET (T:: *call) (P1, P2, P3, gpointer), gpointer obj=NULL) :
    call(call), obj(reinterpret_cast<T*>(obj)) { }

  /** Implementing constructor. */
  GtkCallerImp3(T* obj, const GtkCallerImp3& o) : call(o.call), obj(obj) { }

  /** Full constructor */
  GtkCallerImp3(T* obj, RET (T:: *call) (P1, P2, P3, gpointer)) :
    call(call), obj(obj) { }

  /** Clone with object specialization. */
  GtkCaller* clone(void* obj) const override
  { return new GtkCallerImp3(reinterpret_cast<T*>(obj), *this); }

  /** Execute call. */
  RET operator () (P1 a, P2 b, P3 c)
  { return (obj ->* call)(a, b, c, gp); }

  /** Return a pointer to the GtkCallback function. */
  GtkSignalFunc callback()
  { return reinterpret_cast<GtkSignalFunc>(base); }
};

/** c-linkage call with three parameters. */
template<class T, typename RET, typename P1, typename P2, typename P3>
static RET GtkCaller_callback(P1 g1, P2 g2, P3 g3, gpointer gp)
{
  return (*reinterpret_cast<GtkCallerImp3<T,RET,P1,P2,P3>*>(gp))
    (g1, g2, g3);
}

/** static member */
template<class T, typename RET, typename P1, typename P2, typename P3>
RET (* GtkCallerImp3<T,RET,P1,P2,P3>::base)
  (P1, P2, P3, gpointer) = &GtkCaller_callback<T,RET,P1,P2,P3>;


/** 44444444444444444444444444444444444444444444444444444444444  */

/** Callback implementation class, for calls with one parameter. */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4>
class GtkCallerImp4: public GtkCaller
{
  /** pointer to the member function. */
  RET (T:: *call) (P1, P2, P3, P4, gpointer);

  /** pointer to the instantiation of the class. Is NULL when acting
      as a generic object in the table. */
  T* obj;

  /** pointer to the c-linkage callback used. */
  static RET (*base) (P1, P2, P3, P4, gpointer);

public:
  /** Constructor.
      \param  call pointer to the callback function. */
  GtkCallerImp4(RET (T:: *call) (P1, P2, P3, P4, gpointer), gpointer obj=NULL) :
    call(call), obj(reinterpret_cast<T*>(obj)) { }

  /** Implementing constructor. */
  GtkCallerImp4(T* obj, const GtkCallerImp4& o) : call(o.call), obj(obj) { }

  /** Full constructor */
  GtkCallerImp4(T* obj, RET (T:: *call) (P1, P2, P3, P4, gpointer)) :
    call(call), obj(obj) { }

  /** Clone with object specialization. */
  GtkCaller* clone(void* obj) const override
  { return new GtkCallerImp4(reinterpret_cast<T*>(obj), *this); }

  /** Execute call. */
  RET operator () (P1 a, P2 b, P3 c, P4 d)
  { return (obj ->* call)(a, b, c, d, gp); }

  /** Return a pointer to the GtkCallback function. */
  GtkSignalFunc callback()
  { return reinterpret_cast<GtkSignalFunc>(base); }
};

/** c-linkage call. */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4>
static RET GtkCaller_callback(P1 g1, P2 g2, P3 g3, P4 g4, gpointer gp)
{
  return (*reinterpret_cast<GtkCallerImp4<T,RET,P1,P2,P3,P4>*>(gp))
    (g1, g2, g3, g4);
}

/** static member pointing to c-linkage call */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4>
RET (* GtkCallerImp4<T,RET,P1,P2,P3,P4>::base)
  (P1, P2, P3, P4, gpointer) = &GtkCaller_callback<T,RET,P1,P2,P3,P4>;


/** 55555555555555555555555555555555555555555555555555555555555  */

/** Callback implementation class, for calls with one parameter. */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4, typename P5>
class GtkCallerImp5: public GtkCaller
{
  /** pointer to the member function. */
  RET (T:: *call) (P1, P2, P3, P4, P5, gpointer);

  /** pointer to the instantiation of the class. Is NULL when acting
      as a generic object in the table. */
  T* obj;

  /** pointer to the c-linkage callback used. */
  static RET (*base) (P1, P2, P3, P4, P5, gpointer);

public:
  /** Constructor.
      \param  call pointer to the callback function. */
  GtkCallerImp5(RET (T:: *call) (P1, P2, P3, P4, P5, gpointer), gpointer obj=NULL) :
    call(call), obj(reinterpret_cast<T*>(obj)) { }

  /** Implementing constructor. */
  GtkCallerImp5(T* obj, const GtkCallerImp5& o) : call(o.call), obj(obj) { }

  /** Full constructor */
  GtkCallerImp5(T* obj, RET (T:: *call) (P1, P2, P3, P4, P5, gpointer)) :
    call(call), obj(obj) { }

  /** Clone with object specialization. */
  GtkCaller* clone(void* obj) const override
  { return new GtkCallerImp5(reinterpret_cast<T*>(obj), *this); }

  /** Execute call. */
  RET operator () (P1 a, P2 b, P3 c, P4 d, P5 e)
  { return (obj ->* call)(a, b, c, d, e, gp); }

  /** Return a pointer to the GtkCallback function. */
  GtkSignalFunc callback()
  { return reinterpret_cast<GtkSignalFunc>(base); }
};

/** c-linkage call. */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4, typename P5>
static RET GtkCaller_callback(P1 g1, P2 g2, P3 g3, P4 g4,
                              P5 g5, gpointer gp)
{
  return (*reinterpret_cast<GtkCallerImp5<T,RET,P1,P2,P3,P4,P5>*>(gp))
    (g1, g2, g3, g4, g5);
}

/** static member */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4, typename P5>
RET (* GtkCallerImp5<T,RET,P1,P2,P3,P4,P5>::base)
  (P1, P2, P3, P4, P5, gpointer) = &GtkCaller_callback<T,RET,P1,P2,P3,P4,P5>;


/** 66666666666666666666666666666666666666666666666666666666666  */

/** Callback implementation class, for calls with one parameter. */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4, typename P5, typename P6>
class GtkCallerImp6: public GtkCaller
{
  /** pointer to the member function. */
  RET (T:: *call) (P1, P2, P3, P4, P5, P6, gpointer);

  /** pointer to the instantiation of the class. Is NULL when acting
      as a generic object in the table. */
  T* obj;

  /** pointer to the c-linkage callback used. */
  static RET (*base) (P1, P2, P3, P4, P5, P6, gpointer);

public:
  /** Constructor.
      \param  call pointer to the callback function. */
  GtkCallerImp6(RET (T:: *call) (P1, P2, P3, P4, P5, P6, gpointer), gpointer obj=NULL) :
    call(call), obj(reinterpret_cast<T*>(obj)) { }

  /** Implementing constructor. */
  GtkCallerImp6(T* obj, const GtkCallerImp6& o) : call(o.call), obj(obj) { }

  /** Full constructor */
  GtkCallerImp6(T* obj, RET (T:: *call) (P1, P2, P3, P4, P5, P6, gpointer)) :
    call(call), obj(obj) { }

  /** Clone with object specialization. */
  GtkCaller* clone(void* obj) const override
  { return new GtkCallerImp6(reinterpret_cast<T*>(obj), *this); }

  /** Execute call. */
  RET operator () (P1 a, P2 b, P3 c, P4 d, P5 e, P6 f)
  { return (obj ->* call)(a, b, c, d, e, f, gp); }

  /** Return a pointer to the GtkCallback function. */
  GtkSignalFunc callback()
  { return reinterpret_cast<GtkSignalFunc>(base); }
};

/** c-linkage call. */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4, typename P5, typename P6>
static RET GtkCaller_callback(P1 g1, P2 g2, P3 g3, P4 g4,
                              P5 g5, P6 g6, gpointer gp)
{
  return (*reinterpret_cast<GtkCallerImp6<T,RET,P1,P2,P3,P4,P5,P6>*>(gp))
    (g1, g2, g3, g4, g5, g6);
}

/** static member */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4, typename P5, typename P6>
RET (* GtkCallerImp6<T,RET,P1,P2,P3,P4,P5,P6>::base)
  (P1, P2, P3, P4, P5, P6, gpointer) =
  &GtkCaller_callback<T,RET,P1,P2,P3,P4,P5,P6>;


/** 77777777777777777777777777777777777777777777777777777777777  */

/** Callback implementation class, for calls with seven parameters. */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4, typename P5, typename P6, typename P7>
class GtkCallerImp7: public GtkCaller
{
  /** pointer to the member function. */
  RET (T:: *call) (P1, P2, P3, P4, P5, P6, P7, gpointer);

  /** pointer to the instantiation of the class. Is NULL when acting
      as a generic object in the table. */
  T* obj;

  /** pointer to the c-linkage callback used. */
  static RET (*base) (P1, P2, P3, P4, P5, P6, P7, gpointer);

public:
  /** Constructor.
      \param  call pointer to the callback function. */
  GtkCallerImp7(RET (T:: *call) (P1, P2, P3, P4, P5, P6, P7, gpointer), gpointer obj=NULL) :
    call(call), obj(reinterpret_cast<T*>(obj)) { }

  /** Implementing constructor. */
  GtkCallerImp7(T* obj, const GtkCallerImp7& o) : call(o.call), obj(obj) { }

  /** Full constructor */
  GtkCallerImp7(T* obj, RET (T:: *call) (P1, P2, P3, P4, P5, P6, P7, gpointer)) :
    call(call), obj(obj) { }

  /** Clone with object specialization. */
  GtkCaller* clone(void* obj) const override
  { return new GtkCallerImp7(reinterpret_cast<T*>(obj), *this); }

  /** Execute call. */
  RET operator () (P1 a, P2 b, P3 c, P4 d, P5 e, P6 f, P7 g)
  { return (obj ->* call)(a, b, c, d, e, f, g, gp); }

  /** Return a pointer to the GtkCallback function. */
  GtkSignalFunc callback()
  { return reinterpret_cast<GtkSignalFunc>(base); }
};

/** c-linkage call. */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4, typename P5, typename P6, typename P7>
static RET GtkCaller_callback(P1 g1, P2 g2, P3 g3, P4 g4,
                              P5 g5, P6 g6, P7 g7, gpointer gp)
{
  return (*reinterpret_cast<GtkCallerImp7<T,RET,P1,P2,P3,P4,P5,P6,P7>*>(gp))
    (g1, g2, g3, g4, g5, g6, g7);
}

/** static member */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4, typename P5, typename P6, typename P7>
RET (* GtkCallerImp7<T,RET,P1,P2,P3,P4,P5,P6,P7>::base)
  (P1, P2, P3, P4, P5, P6, P7, gpointer) =
  &GtkCaller_callback<T,RET,P1,P2,P3,P4,P5,P6,P7>;


/** 88888888888888888888888888888888888888888888888888888888888  */

/** Callback implementation class, for calls with seven parameters. */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4, typename P5, typename P6, typename P7, typename P8>
class GtkCallerImp8: public GtkCaller
{
  /** pointer to the member function. */
  RET (T:: *call) (P1, P2, P3, P4, P5, P6, P7, P8, gpointer);

  /** pointer to the instantiation of the class. Is NULL when acting
      as a generic object in the table. */
  T* obj;

  /** pointer to the c-linkage callback used. */
  static RET (*base) (P1, P2, P3, P4, P5, P6, P7, P8, gpointer);

public:
  /** Constructor.
      \param  call pointer to the callback function. */
  GtkCallerImp8(RET (T:: *call) (P1, P2, P3, P4, P5, P6, P7, P8, gpointer), gpointer obj=NULL) :
    call(call), obj(reinterpret_cast<T*>(obj)) { }

  /** Implementing constructor. */
  GtkCallerImp8(T* obj, const GtkCallerImp8& o) : call(o.call), obj(obj) { }

  /** Full constructor */
  GtkCallerImp8(T* obj, RET (T:: *call) (P1, P2, P3, P4, P5, P6, P7, P8, gpointer)) :
    call(call), obj(obj) { }

  /** Clone with object specialization. */
  GtkCaller* clone(void* obj) const override
  { return new GtkCallerImp8(reinterpret_cast<T*>(obj), *this); }

  /** Execute call. */
  RET operator () (P1 a, P2 b, P3 c, P4 d, P5 e, P6 f, P7 g, P8 h)
  { return (obj ->* call)(a, b, c, d, e, f, g, h, gp); }

  /** Return a pointer to the GtkCallback function. */
  GtkSignalFunc callback()
  { return reinterpret_cast<GtkSignalFunc>(base); }
};

/** c-linkage call. */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4, typename P5, typename P6, typename P7, typename P8>
static RET GtkCaller_callback(P1 g1, P2 g2, P3 g3, P4 g4,
                              P5 g5, P6 g6, P7 g7, P8 g8, gpointer gp)
{
  return (*reinterpret_cast<GtkCallerImp8<T,RET,P1,P2,P3,P4,P5,P6,P7,P8>*>(gp))
    (g1, g2, g3, g4, g5, g6, g7, g8);
}

/** static member */
template<class T, typename RET, typename P1, typename P2, typename P3,
         typename P4, typename P5, typename P6, typename P7, typename P8>
RET (* GtkCallerImp8<T,RET,P1,P2,P3,P4,P5,P6,P7,P8>::base)
  (P1, P2, P3, P4, P5, P6, P7, P8, gpointer) =
  &GtkCaller_callback<T,RET,P1,P2,P3,P4,P5,P6,P7,P8>;

DUECA_NS_END;

#endif

