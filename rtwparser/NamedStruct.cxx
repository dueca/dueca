#include "NamedStruct.hxx"
#include <iostream>

NamedStruct::NamedStruct(const char* name, ObjectList* objs, const char* comm) :
  BaseObject("struct", name),
  child_list(objs)
{
  if (string(comm) != "")
    addComment(comm);
}

NamedStruct::~NamedStruct()
{
}
