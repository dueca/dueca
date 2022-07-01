/* ------------------------------------------------------------------   */
/*      item            : doublewrite.cxx
        made by         : Rene' van Paassen
        date            : 051220
        category        : body file
        description     : Test of writing double precision numbers in
                          binary files, to get big-endian output
        changes         : 051220 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2

        conclusion      : the long double format matches *exactly* the
                          format used in Matlab (Linux). No modification made
                          to AmorphStore methods.
                          To read out, use:
                          f = fopen('amstore.bin', 'r', 'ieee-be.l64')
                          r = fread(f, 10, 'double', 0, 'ieee-be.l64')
                          # etc
                          fclose(f)

*/

#include <iostream>
#include <fstream>
#include <cmath>
#include <inttypes.h>
#include <netinet/in.h>
#include <AmorphStore.hxx>

using namespace std;

int main()
{
  ofstream myfile, amfile;
  uint32_t i[2];
  char buffer[1680];
  dueca::AmorphStore s(buffer, 1680);

  myfile.open("double.bin", ios::out | ios::binary);
  amfile.open("amstore.bin", ios::out | ios::binary);

  for (int p = -10; p < 10; p++) {
    for (int num = 0; num < 10; num++) {
      double d = num * pow(10.0, double (p));
      i[0] = htonl(*reinterpret_cast<unsigned*>(&d));
      i[1] = htonl(*(reinterpret_cast<unsigned*>(&d)+1));
      myfile.write(reinterpret_cast<const char*>(&i[1]), 4);
      myfile.write(reinterpret_cast<const char*>(&i[0]), 4);
      cout << d << endl;
      packData(s, d);
    }
  }
  double testnums[10] = {
    M_PI,
    1.234356789012345,
    123456789012345.0,
    0.1234567890E-80,
    5, 6, 7, 8, 9, 0 };
  for (int ii = 0; ii < 10; ii++) {
    double d = testnums[ii];
    i[0] = htonl(*reinterpret_cast<unsigned*>(&d));
    i[1] = htonl(*(reinterpret_cast<unsigned*>(&d)+1));
    myfile.write(reinterpret_cast<const char*>(&i[1]), 4);
    myfile.write(reinterpret_cast<const char*>(&i[0]), 4);
    cout << d << endl;
    packData(s, d);
  }
  amfile.write(s.getToData(), s.getSize());
  amfile.close();
  myfile.close();
}
