/* ------------------------------------------------------------------   */
/*      item            : ReflectoryCopier.hxx
        made by         : Rene van Paassen
        date            : 160801
        category        : header file
        description     :
        changes         : 160801 first version
        language        : C++
*/

#ifndef ReflectoryCopier_hxx
#define ReflectoryCopier_hxx

/** An object that accepts the stream of object and configation
    updates of a reflectory, and packs these for transport if needed,
    and buffers and plays back data from remote reflectory copies.

    Keeps a tab on the status of each remote reflectory copy.
*/


class ReflectoryCopier
{

public:
  /** Constructor */
  ReflectoryCopier();

  /** Destructor */
  ~ReflectoryCopier();
};

#endif
