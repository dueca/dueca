/* ------------------------------------------------------------------   */
/*      item            : TypeNameDef.hxx
        made by         : Joost Ellerbroek
        date            : 080128
        category        : header file
        description     :
        changes         : 080129 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek/René van Paassen
        license         : EUPL-1.2
*/

#ifndef TYPE_NAME_DEF_HXX
#define TYPE_NAME_DEF_HXX

#include "BaseObject.hxx"

using namespace std;

class TypeNameDef: public BaseObject
{
  public:
    /** Constructor.
        \param type     character string of the type
        \param name     name of the member variable
        \param size     dimension, a positive size means that a C-style array
                        of fixed length is generated.
        \param comm     Optional comment for this variable */
    TypeNameDef(const char* type, const char* name, const int size, const char* comm = "");

    /** Constructor.
    \param type     character string of the type
    \param name     name of the member variable
    \param comm     Optional comment for this variable */
    TypeNameDef(const char* type, const char* name, const char* comm = "");

    /// Destructor.
    ~TypeNameDef();

    /// Return the size of the array. returns 1 if this is not an array
    int     getSize() const {return size;}

  public: // Matlab RTW specific methods and data for formatted comments
    enum { ERROR_NO_COMMENT=0, HAS_COMPUTED_PARAM=1, HAS_EXPRESSION=2, IS_NOT_PARAM=3 };
    /// Simulink subsystem depth of this item
    int     syslevel;
    /// Simulink blockname, block internal variable name, or MATLAB expression
    string  block, var, expr;

    /// Parse the attached comment
    int     parseComment();

  private:
    /// Array size
    int     size;
};

#endif
