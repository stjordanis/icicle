/** @file
 *  @author Sylwester Arabas <slayoo@igf.fuw.edu.pl>
 *  @author Anna Jaruga <ajaruga@igf.fuw.edu.pl>
 *  @copyright University of Warsaw
 *  @date November 2011
 *  @section LICENSE
 *    GPL v3 (see the COPYING file or http://www.gnu.org/licenses/)
 *  @section DESCRIPTION
 *    C++ implementation of the MPDATA scheme for the Arakawa-C grid
 *    (for solenoidal flows on a uniformly spaced grid)
 */
#ifndef ADV_MPDATA_HPP
#  define ADV_MPDATA_HPP

#  include "adv.hpp"
#  include "grd_arakawa-c-lorenz.hpp"

template <typename real_t> 
class adv_mpdata : public adv<real_t> 
{
  public: const int stencil_extent() { return 3; }
  public: const int time_levels() { return 2; }
  public: const int num_steps() { return iord; }

  private: int iord;
  private: bool cache;
  private: grd_arakawa_c_lorenz<real_t> *grid;
  private: auto_ptr<Array<real_t, 3> > caches[3]; // antidiffusive velocity caches

  public: adv_mpdata(grd_arakawa_c_lorenz<real_t> *grid, int iord, bool cache = false) 
    : iord(iord), cache(cache), grid(grid)
  {
    if (iord <= 0) error_macro("iord (the number of iterations) must be > 0")
  }

#    define mpdata_F(p1, p2, U) (.5 * (U + abs(U)) * p1 + .5 * (U - abs(U)) * p2)
  public: void op_helper(int dim, const real_t sign, const Range &il, const Range &ic, const Range &ir,
    const Range &i, const Range &j, const Range &k, Array<real_t, 3>* psi[], const int n,
    const Array<real_t, 3> &Cx, const Array<real_t, 3> &Cy, const Array<real_t, 3> &Cz
  )
  {
    if (cache)
    {
      if (caches[0].get() == NULL) // not allocated yet
      {
        int nx=Cx.cols(), ny=Cx.rows(), nz=Cx.depth();
        caches[0].reset(new Array<real_t, 3>(nx, ny, nz));
        caches[1].reset(new Array<real_t, 3>(nx, ny, nz));
        caches[2].reset(new Array<real_t, 3>(nx, ny, nz));
      }
    }
    // preprocessor macros are the only option as methods cannot return parts of Blitz expressions 
#    define mpdata_A(pr, pl) \
       where(pr + pl > 0, \
         (pr - pl) / (pr + pl), \
         real_t(0.) \
       )
#    define mpdata_B(pru, plu, prd, pld) \
       where(pru + plu + prd + pld > 0, \
         .5 * (pru + plu - prd - pld) / (pru + plu + prd + pld), \
         real_t(0.) \
       )
#    define mpdata_V(Vru, Vlu, Vrd, Vld) (.25 * (Vru + Vlu + Vrd + Vld))
#    define mpdata_CA(pr, pl, U) ((abs(U) - pow(U,2)) * mpdata_A(pr, pl))
#    define mpdata_CB(pru, plu, prd, pld, U, V) (U * V * mpdata_B(pru, plu, prd, pld)) 
    (*psi[n+1])(i,j,k) += sign * (
      mpdata_F(
        (*psi[n])(il, j, k), (*psi[n])(ir, j, k),
// TODO na jutro: 
// - 6 tablicy w cache'u
// - poniższy nawias -> makro preprocesora
// - zapisanie wyniku lub nie w zależności od wartości cache
// - metody getCache() do uzycia w klasie pochodnej (adv_mpdata_fct)
        (
          mpdata_CA( 
            (*psi[n])(ir, j, k), (*psi[n])(il, j, k), // pl, pr
            Cx(ic, j, k)
          ) - 
          mpdata_CB(
            (*psi[n])(ir, j+1, k), (*psi[n])(il, j+1, k), // pru, plu
            (*psi[n])(ir, j-1, k), (*psi[n])(il, j-1, k), // prd, pld
            Cx(ic, j, k), mpdata_V(
              Cy(ir, j + grid->p_half, k), Cy(il, j + grid->p_half, k), // Vru, Vlu
              Cy(ir, j - grid->m_half, k), Cy(il, j - grid->m_half, k)  // Vrd, Vld
            )
          ) - 
          mpdata_CB(
            (*psi[n])(ir, j, k+1), (*psi[n])(il, j, k+1), // pru, plu
            (*psi[n])(ir, j, k-1), (*psi[n])(il, j, k-1), // prd, pld
            Cx(ic, j, k), mpdata_V(
              Cz(ir, j, k + grid->p_half), Cz(il, j, k + grid->p_half), // Vru, Vlu
              Cz(ir, j, k - grid->m_half), Cz(il, j, k - grid->m_half)  // Vrd, Vld
            )
          ) 
        )
      )
    );
#    undef mpdata_A
#    undef mpdata_B
#    undef mpdata_V
#    undef mpdata_CA
#    undef mpdata_CB
  }

  public: void op(int dim, Array<real_t, 3>* psi[], 
    const Range &i, const Range &j, const Range &k, const int n, const int step,
    const Array<real_t, 3> &Cx, const Array<real_t, 3> &Cy, const Array<real_t, 3> &Cz
  )
  {
    if (step == 1)
      (*psi[n+1])(i,j,k) -= (
        mpdata_F((*psi[n])(i  ,j,k), (*psi[n])(i+1,j,k), Cx(i + grid->p_half,j,k)) - 
        mpdata_F((*psi[n])(i-1,j,k), (*psi[n])(i  ,j,k), Cx(i - grid->m_half,j,k))
      );
    else 
    {
      op_helper(dim, -1, i  , i+grid->p_half, i+1, i, j, k, psi, n, Cx, Cy, Cz);
      op_helper(dim, +1, i-1, i-grid->m_half, i  , i, j, k, psi, n, Cx, Cy, Cz);
    }
  }
#    undef mpdata_F
};
#endif
