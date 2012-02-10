/** @file
 *  @author Sylwester Arabas <slayoo@igf.fuw.edu.pl>
 *  @copyright University of Warsaw
 *  @date November 2011
 *  @section LICENSE
 *    GPLv3+ (see the COPYING file or http://www.gnu.org/licenses/)
 */
#ifndef ADV_LEAPFROG_HPP
#  define ADV_LEAPFROG_HPP

#  include "adv.hpp"
#  include "grd_arakawa-c-lorenz.hpp"

template <typename real_t> 
class adv_leapfrog : public adv<real_t> 
{
  adv_hack_macro // workaround for virtual template methods
  public: const int stencil_extent() { return 3; }
  public: const int time_levels() { return 3; }
 
  public: const real_t courant_max() { return 1.; }
  public: const real_t courant_min() { return .5; } ; //TODO Wicker Skamarock 2002 
 
  private: grd_arakawa_c_lorenz<real_t> *grid;

  public: adv_leapfrog(grd_arakawa_c_lorenz<real_t> *grid)
    : grid(grid)
  { }

  public: 
  template <class idx>
  void op(
    mtx::arr<real_t>* psi[], 
    mtx::arr<real_t>*[], 
    mtx::arr<real_t>*[], 
    const mtx::rng &i, const mtx::rng &j, const mtx::rng &k, 
    const int n, const int step,
    const mtx::arr<real_t> * const Cx, const mtx::arr<real_t> * const, const mtx::arr<real_t> * const 
  )
  {
    assert(step == 1);
    ///  \f$ \psi^{n+1}_i = \psi^{n-1}_i - C^{n}_{i} \cdot (\psi^{n}_{i+1} - \psi^{n}_{i-1}) \f$ \n
    ///  where C is the average Courant number \n
    ///  for Arakawa C grid: \f$ C^{n}_i=0.5\cdot(C^{n}_{i+1/2} + C^{n}_{i-1/2}) \f$

    (*psi[n+1])(idx(i,j,k)) -= 
      .5 * ((*Cx)(idx(i + grid->p_half,j,k)) + (*Cx)(idx(i - grid->m_half,j,k))) // average Courant number!
      * ( (*psi[n])(idx(i+1,j,k)) - (*psi[n])(idx(i-1,j,k)) );
  }
};
#endif
