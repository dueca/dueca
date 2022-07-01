/* ------------------------------------------------------------------   */
/*      item            : SchemeClassData.cxx
        made by         : Rene' van Paassen
        date            : 990709
        category        : body file
        description     :
        changes         : 990709 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#include "SchemeClassData.hxx"
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include "debug.h"
#include "dueca-guile.h"
DUECA_NS_START

static const unsigned int INVALID_TAG = -1;

GenericSchemeClassData::GenericSchemeClassData
(const char* scheme_name, GenericSchemeClassData* parent) :
  scheme_tag(INVALID_TAG),
  parent(parent)
{
  this->scheme_name = new char[std::strlen(scheme_name) + 6];
  std::strcpy(this->scheme_name, "make-");
  std::strcpy(&(this->scheme_name[5]), scheme_name);

  /* DUECA scripting.

     Creation of a new scheme type */
  I_CNF("New scheme type " << scheme_name);
}

GenericSchemeClassData::~GenericSchemeClassData()
{
  delete[] scheme_name;
}

void GenericSchemeClassData::addTagValue(scm_t_bits t)
{
  scheme_tags.push_back(t);
  if (parent) parent->addTagValue(t);
}

void GenericSchemeClassData::setTag(scm_t_bits t)
{
  /* DUECA scripting.

     Assignment of a tag for a scheme type */
  I_CNF("Tag for " << scheme_name << " is " << t);
  if (scheme_tag != INVALID_TAG) {
    cerr << "re-defining scheme tag from " << scheme_tag << " to "
         <<  t << endl;
  }
  scheme_tag = t;

  // parent tag
  if (parent) parent->addTagValue(t);
}

int GenericSchemeClassData::validTag(SCM smob)
{
#if defined(SCM_USE_FOREIGN)
  if (SCM_IS_A_P(smob, SCM_PACK(scheme_tag))) return 1;
  for (list<scm_t_bits>::const_iterator ii = scheme_tags.begin();
       ii != scheme_tags.end(); ii++) {
    if (SCM_IS_A_P(smob, SCM_PACK(*ii))) return 1;
  }
#else
  if (SCM_SMOB_PREDICATE(scheme_tag, smob)) return 1;

  for (list<scm_t_bits>::const_iterator ii = scheme_tags.begin();
       ii != scheme_tags.end(); ii++) {
    if (SCM_SMOB_PREDICATE(*ii, smob)) return 1;
  }
#endif
  return 0;
}

char* GenericSchemeClassData::getMakeName()
{
  return scheme_name;
}

char* GenericSchemeClassData::getName()
{
  return &scheme_name[5];
}


DUECA_NS_END
