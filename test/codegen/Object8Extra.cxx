#define __CUSTOM_COMPATLEVEL_110

bool Object8::set_ax(const std::vector<double> &i)
{
  a.resize(i.size());
  std::copy(i.begin(), i.end(), a.begin());
  return true;
}
