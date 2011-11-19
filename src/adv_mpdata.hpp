/** @file
 *  @author Sylwester Arabas (University of Warsaw) <slayoo@igf.fuw.edu.pl>
 *  @date November 2011
 *  @section LICENSE
 *    GPL v3 (see the COPYING file or http://www.gnu.org/licenses/)
 */
#ifndef ADV_MPDATA_HPP
#  define ADV_MPDATA_HPP

#  include "adv.hpp"

template <class unit, typename real_t> 
class adv_mpdata : public adv<unit, real_t> 
{
  private: int iord;
  public: const int stencil_extent() { return 3; }
  public: const int time_levels() { return 2; }
  public: const int num_steps() { return iord; }

  public: adv_mpdata(int iord = 2) 
    : iord(iord)
  {
    assert(iord > 0);
  }

  public: void op_1D(Array<quantity<unit, real_t>, 3>* psi[], const Range &i,
    const int n, const quantity<si::dimensionless, real_t> &courant, const int step) 
  {
#    define F(psi_l,psi_r,U) (.5 * (U + sqrt(U*U)) * psi_l + .5 * (U - sqrt(U*U)) * psi_r)
    switch (step)
    {
      case 1:
      { 
        (*psi[n+1])(i) -= (F((*psi[n])(i), (*psi[n])(i+1), courant) - F((*psi[n])(i-1), (*psi[n])(i), courant));
        break;
      }
      default:
      {
        (*psi[n+1])(i) -= (
          F((*psi[n])(i), (*psi[n])(i+1), 
            where(
              (*psi[n])(i+1) + (*psi[n])(i) > 0,
              (abs(courant) - pow(courant, 2)) * ((*psi[n])(i+1) - (*psi[n])(i)) / ((*psi[n])(i+1) + (*psi[n])(i)),
              quantity<unit, real_t>(0.)
            )
          ) -
          F((*psi[n])(i-1), (*psi[n])(i), 
            where(
              (*psi[n])(i) + (*psi[n])(i-1) > 0,
              (abs(courant) - pow(courant, 2)) * ((*psi[n])(i) - (*psi[n])(i-1)) / ((*psi[n])(i) + (*psi[n])(i-1)),
              quantity<unit, real_t>(0.)
            )
          )
        );
      }
    }
#    undef F
  }
};
#endif
