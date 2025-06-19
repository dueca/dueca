/* ------------------------------------------------------------------   */
/*      item            : linearsystem.cxx
        made by         : Rene' van Paassen
        date            : 160529
        category        : body file
        description     :
        changes         : 160529 first version
        language        : C++
*/

#define linearsystem_cxx
#include <LinearSystem.hxx>
#include <iostream>
using namespace std;
using namespace dueca;

int main()
{
  double dt = 0.02;
  double w_notch =0.06666667;
  double z_notch = 0.01;
  Vector numnf(3);
  Vector dennf(3);
  numnf[0] = 1.0;
  numnf[1] = 2.0*z_notch/w_notch;
  numnf[2] = 1.0/(w_notch*w_notch);
  dennf[0] = 1.0;
  dennf[1] = 2.0/w_notch;
  dennf[2] = 1.0/(w_notch*w_notch);
  LinearSystem* notch_filter1 = new LinearSystem(numnf, dennf, dt);

  cout << "phi " << notch_filter1->getPhi() << endl;
  cout << "psi " << notch_filter1->getPsi() << endl;
  cout << "  C " << notch_filter1->getC() << endl;
  cout << "  D " << notch_filter1->getD() << endl;


#if 1

  double Phinotch[2][2]={{0.5186,-0.2084},
                         {0.2371, 0.9631}};
  double Bnotch[2]={0.05927,0.01051};
  double Cnotch[2]={-6.75,0};
  double Dnotch=1;
  Matrix Phin(2,2);
  Matrix Bn(2,1);
  Matrix Cn(1,2);
  Matrix Dn(1,1);
  for(int ii=0;ii<2;ii++) {
    for (int jj = 0; jj < 2; jj++) {
      Phin(ii,jj)=Phinotch[ii][jj];
    }
  }
  Bn(0,0)=Bnotch[0];
  Bn(1,0)=Bnotch[1];
  Cn(0,0)=Cnotch[0];
  Cn(0,1)=Cnotch[1];
  Dn(0,0)=Dnotch;
  LinearSystem* notch_filter2= new LinearSystem(Phin, Bn, Cn, Dn);
  cout << "phi " << notch_filter2->getPhi() << endl;
  cout << "psi " << notch_filter2->getPsi() << endl;
  cout << "  C " << notch_filter2->getC() << endl;
  cout << "  D " << notch_filter2->getD() << endl;
 #endif
  return 0;
}
