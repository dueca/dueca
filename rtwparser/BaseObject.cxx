#include "BaseObject.hxx"

string BaseObject::modelname = "";

BaseObject::BaseObject(const char* type, const char* name) :
    type(type), name(name)
{
}

BaseObject::~BaseObject()
{
}
