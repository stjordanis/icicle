/** @file
 *  @author Sylwester Arabas <slayoo@igf.fuw.edu.pl>
 *  @copyright University of Warsaw
 *  @date November 2011
 *  @section LICENSE
 *    GPL v3 (see the COPYING file or http://www.gnu.org/licenses/)
 */
#ifndef ADV_LEAPFROG_HPP
#  define ADV_LEAPFROG_HPP

#  include "adv.hpp"
#  include "grd_arakawa-c-lorenz.hpp"

template <typename real_t> 
class adv_leapfrog : public adv<real_t> 
{
  public: const int stencil_extent() { return 3; }
  public: const int time_levels() { return 3; }
  public: const int num_steps() { return 1; }
 
  private: grd_arakawa_c_lorenz<real_t> *grid;

  public: adv_leapfrog(grd_arakawa_c_lorenz<real_t> *grid)
    : grid(grid)
  { }

  public: void op(int, Array<real_t, 3>* psi[], 
    const Range &i, 
    const Range &j, 
    const Range &k, 
    const int n, const int step,
    const Array<real_t, 3> &Cx, 
    const Array<real_t, 3> &, 
    const Array<real_t, 3> &
  )
  {
    assert(step == 1);
    (*psi[n+1])(i,j,k) -= 
      .5 * (Cx(i + grid->p_half,j,k) + Cx(i - grid->m_half,j,k)) // average Courant number!
      * ( (*psi[n])(i+1,j,k) - (*psi[n])(i-1,j,k) );
  }
};
#endif
