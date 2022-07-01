/* ------------------------------------------------------------------   */
/*      item            : StringUtils.cxx
        made by         : Joost Ellerbroek
        date            : 080314
        category        : body file
        description     :
        changes         : 080314 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek/René van Paassen
        license         : EUPL-1.2
*/

#include "StringUtils.hxx"

DUECA_NS_START;

string replaceAll(string s, const string & search, const string & repl)
{
  unsigned int found = s.find(search);
  while(found < s.size())
  {
    s.replace(found, search.size(), repl);
    found = s.find(search,found+repl.size());
  }
  return s;
}

unsigned int split(const string & input, vector<string> & output, const string & separators)
{
  unsigned int init_size = output.size();
  unsigned int start, stop;
  start = input.find_first_not_of(separators);
  while (start < input.size()) {
    stop = input.find_first_of(separators, start);
    if (stop > input.size())
      stop = input.size();
    output.push_back(input.substr(start, stop - start));
    start = input.find_first_not_of(separators, stop+1);
  }

  return (output.size()-init_size);
}

string trim(const string & s, const string & pattern) {
  if(s.size() == 0) return s;
  unsigned int b = s.find_first_not_of(pattern);
  if(b > s.size())  return "";
  unsigned int e = s.find_last_not_of(pattern);
  return string(s, b, e - b + 1);
}

DUECA_NS_END;
