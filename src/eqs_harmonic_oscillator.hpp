/** @file
 *  @author Anna Jaruga <ajaruga@igf.fuw.edu.pl>
 *  @author Sylwester Arabas <slayoo@igf.fuw.edu.pl>
 *  @copyright University of Warsaw
 *  @date March 2012
 *  @section LICENSE
 *    GPLv3+ (see the COPYING file or http://www.gnu.org/licenses/)
 @  @brief contains definition of eqs_harmonic_oscillator
 */
#ifndef EQS_HARMONIC_OSCILLATOR_HPP
#  define EQS_HARMONIC_OSCILLATOR_HPP

#  include "cmn.hpp"
#  include "eqs.hpp"

/// @brief a minimalistic model of a harmonic oscillator 
///   (consult eq. 28 in Smolarkiewicz 2006, IJNMF)
template <typename real_t>
class eqs_harmonic_oscillator : public eqs<real_t> 
{
  private: ptr_vector<struct eqs<real_t>::gte> sys;
  public: ptr_vector<struct eqs<real_t>::gte> &system() { return sys; }

  // nested class
  private: class restoring_force : public rhs<real_t>
  { 
    // private members
    private: real_t omega_signed;
    private: int eqid;

    // ctor
    public: restoring_force(
      quantity<si::frequency, real_t> omega, 
      real_t sign,
      int eqid
    ) 
      : omega_signed(sign * omega * si::seconds), eqid(eqid)
    {} 

    // public methods
    public: void explicit_part(mtx::arr<real_t> &R, mtx::arr<real_t> **psi) 
    { 
      R(R.ijk) += omega_signed * (*psi[eqid])(R.ijk);
    };

    public: real_t implicit_part(quantity<si::time, real_t> dt)
    {
      return -(dt / si::seconds) * pow(omega_signed, 2);
    }
  };

  // ctor
  public: eqs_harmonic_oscillator(quantity<si::frequency, real_t> omega)
  {
    sys.push_back(new struct eqs<real_t>::gte({ "psi", "1st variable", "dimensionless" }));
    sys.back().rhs_terms.push_back(new restoring_force(omega, +1, 1)); 

    sys.push_back(new struct eqs<real_t>::gte({ "phi", "2nd variable", "dimensionless" }));
    sys.back().rhs_terms.push_back(new restoring_force(omega, -1, 0)); 
  }
};
#endif
