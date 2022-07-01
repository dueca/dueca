};

/** packing dueca amorphstore */
template <size_t N, typename D>
void packData(dueca::AmorphStore& s, const dueca::fixvector<N, D>& fv)
{
  for (auto const &val: fv) { packData(s, val); }
}

template <size_t N, typename D>
void unPackData(dueca::AmorphReStore& s, dueca::fixvector<N, D>& fv)
{
  for (auto &val: fv) { unPackData(s, val); }
}

namespace _dummy {
