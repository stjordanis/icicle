/** @file
 *  @author Sylwester Arabas <slayoo@igf.fuw.edu.pl>
 *  @copyright University of Warsaw
 *  @date November 2011
 *  @section LICENSE
 *    GPL v3 (see the COPYING file or http://www.gnu.org/licenses/)
 */
#ifndef DOM_SERIAL_HPP
#  define DOM_SERIAL_HPP

#  include "slv.hpp"
#  include "stp.hpp"

template <typename real_t>
class slv_serial : public slv<real_t>
{
  private: adv<real_t> *fllbck, *advsch;
  private: auto_ptr<Range> i, j, k;
  private: out<real_t> *output;
  private: int nx, ny, nz, halo;//, halo, zhalo;

  private: auto_ptr<Array<real_t, 3> > Cx, Cy, Cz;

  private: auto_ptr<Array<real_t, 3> > *psi_guard;
  private: Array<real_t, 3> **psi;

  private: auto_ptr<Array<real_t, 3> > *tmp_s_guard;
  private: Array<real_t, 3> **tmp_s;
  private: auto_ptr<Array<real_t, 3> > *tmp_v_guard;
  private: Array<real_t, 3> **tmp_v;

  public: slv_serial(stp<real_t> *setup,
    int i_min, int i_max, int nx,
    int j_min, int j_max, int ny,
    int k_min, int k_max, int nz, 
    quantity<si::time, real_t> dt // TODO: dt should not be needed here!
  )
    : fllbck(setup->fllbck), advsch(setup->advsch), output(setup->output), nx(nx), ny(ny), nz(nz)
  {
    // memory allocation
    halo = (advsch->stencil_extent() - 1) / 2;
    psi_guard = new auto_ptr<Array<real_t, 3> >[advsch->time_levels()];
    psi = new Array<real_t, 3>*[advsch->time_levels()];
    for (int n=0; n < advsch->time_levels(); ++n) 
    {
      psi_guard[n].reset(new Array<real_t, 3>(
        setup->grid->rng_sclr(i_min, i_max, halo),
        setup->grid->rng_sclr(j_min, j_max, halo),
        setup->grid->rng_sclr(k_min, k_max, halo)
      ));
      psi[n] = psi_guard[n].get();
    }

    // caches
    assert(fllbck == NULL || fllbck->num_sclr_caches() == 0);
    assert(fllbck == NULL || fllbck->num_vctr_caches() == 0);

    {
      int cnt = advsch->num_vctr_caches();
      tmp_v_guard = new auto_ptr<Array<real_t, 3> >[cnt];
      tmp_v = new Array<real_t, 3>*[cnt];
      for (int n=0; n < cnt; ++n)
      {
        tmp_v_guard[n].reset(new Array<real_t, 3>(
          setup->grid->rng_vctr(i_min, i_max),
          setup->grid->rng_vctr(j_min, j_max),
          setup->grid->rng_vctr(k_min, k_max)
        ));
        tmp_v[n] = tmp_v_guard[n].get();
      }
    }
    {
      int cnt = advsch->num_sclr_caches();
      tmp_s_guard = new auto_ptr<Array<real_t, 3> >[cnt];
      tmp_s = new Array<real_t, 3>*[cnt];
      for (int n=0; n < cnt; ++n)
      {
        tmp_s_guard[n].reset(new Array<real_t, 3>(
          setup->grid->rng_vctr(i_min, i_max),
          setup->grid->rng_vctr(j_min, j_max),
          setup->grid->rng_vctr(k_min, k_max)
        ));
        tmp_s[n] = tmp_s_guard[n].get();
      }
    }

 
    // indices
    i.reset(new Range(i_min, i_max));
    j.reset(new Range(j_min, j_max));
    k.reset(new Range(k_min, k_max));

    // initial condition
    setup->grid->populate_scalar_field(*i, *j, *k, psi[0], setup->intcond);

    // periodic boundary in all directions
    for (int s=this->first; s <= this->last; ++s) 
      this->hook_neighbour(s, this);
 
    // velocity fields
    {
      Range 
        ii = setup->grid->rng_vctr(i->first(), i->last()),
        jj = setup->grid->rng_vctr(j->first(), j->last()),
        kk = setup->grid->rng_vctr(k->first(), k->last());
      //TODO dla nx=3 ny=3 Cx powinno być 4x3 a nie 4x4
      Cx.reset(new Array<real_t, 3>(ii, jj, kk));
      Cy.reset(new Array<real_t, 3>(ii, jj, kk));
      Cz.reset(new Array<real_t, 3>(ii, jj, kk));

      setup->grid->populate_courant_fields(ii, jj, kk, Cx.get(), Cy.get(), Cz.get(), setup->velocity, dt);
    }
  }

  public: ~slv_serial()
  {
    delete[] psi;
    delete[] psi_guard;
    //delete[] tmp_{s,v}; TODO!!!
    //delete[] tmp_{s,v}_guard; TODO!!!
  }

  public: void record(const int n, const unsigned long t)
  {
    output->record(psi[n], *i, *j, *k, t);
  }

  public: Array<real_t, 3> data(int n, 
    const Range &i, const Range &j, const Range &k) 
  { 
    return (*psi[n])(i, j, k);
  }

  public: void fill_halos(int n)
  { // TODO: make it much shorter!!!
    { // left halo
      int i_min = i->first() - halo, i_max = i->first() - 1;
      (*psi[n])(Range(i_min, i_max), *j, *k) = 
        this->nghbr_data(slv<real_t>::left, n, Range((i_min + nx) % nx, (i_max + nx) % nx), *j, *k);
    }
    { // rght halo
      int i_min = i->last() + 1, i_max = i->last() + halo;
      (*psi[n])(Range(i_min, i_max), *j, *k) =
        this->nghbr_data(slv<real_t>::rght, n, Range((i_min + nx) % nx, (i_max + nx) % nx), *j, *k);
    } 
    { // fore halo
      int j_min = j->first() - halo, j_max = j->first() - 1;
      (*psi[n])(*i, Range(j_min, j_max), *k) = 
        this->nghbr_data(slv<real_t>::fore, n, *i, Range((j_min + ny) % ny, (j_max + ny) % ny), *k);
    }
    { // hind halo
      int j_min = j->last() + 1, j_max = j->last() + halo;
      (*psi[n])(*i, Range(j_min, j_max), *k) =
        this->nghbr_data(slv<real_t>::hind, n, *i, Range((j_min + ny) % ny, (j_max + ny) % ny), *k);
    } 
    { // base halo
      int k_min = k->first() - halo, k_max = k->first() - 1;
      (*psi[n])(*i, *j, Range(k_min, k_max)) = 
        this->nghbr_data(slv<real_t>::base, n, *i, *j, Range((k_min + nz) % nz, (k_max + nz) % nz));
    }
    { // apex halo
      int k_min = k->last() + 1, k_max = k->last() + halo;
      (*psi[n])(*i, *j, Range(k_min, k_max)) =
        this->nghbr_data(slv<real_t>::apex, n, *i, *j, Range((k_min + nz) % nz, (k_max + nz) % nz));
    } 
  }

  public: void advect(adv<real_t> *a, int n, int s, quantity<si::time, real_t> dt)
  {
    a->op3D(psi, tmp_s, tmp_v, *i, *j, *k, n, s, *Cx, *Cy, *Cz);
  }

  public: void cycle_arrays(const int n)
  {
    switch (advsch->time_levels())
    {
      case 2: // e.g. upstream, mpdata
        cycleArrays(*psi[n], *psi[n+1]); 
        break;
      case 3: // e.g. leapfrog
        cycleArrays(*psi[n-1], *psi[n], *psi[n+1]); 
        break;
      default: assert(false);
    }
  }

  public: void integ_loop(unsigned long nt, quantity<si::time, real_t> dt) 
  {
    assert(false); // TODO: this should not be needed with proper class hierarchy
  }
};
#endif
