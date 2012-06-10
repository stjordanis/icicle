/** @file
 *  @author Sylwester Arabas <slayoo@igf.fuw.edu.pl>
 *  @copyright University of Warsaw
 *  @date November 2011
 *  @section LICENSE
 *    GPLv3+ (see the COPYING file or http://www.gnu.org/licenses/)
 @  @brief contains the @ref main() function in which the floating point precision choice takes place
 */

#include "cfg.hpp"
#include "mdl.hpp"

int main(int ac, char* av[])
{
  cerr << "-- init: icicle starting (built on " << __DATE__ << ")" << endl;
#if defined(__GNUC__) && !defined(__FAST_MATH__)
  warning_macro("GCC was used without the -ffast-math flag!")
#endif
  try
  {
    // options list
    po::options_description desc("options");
    desc.add_options()
      ("help", "print this message")
      ("bits", po::value<int>()->default_value(32), "floating point bits: sizeof(float), sizeof(double), sizeof(long double)");
    opt_stp_desc(desc);
    opt_adv_desc(desc);
    opt_slv_desc(desc);
    opt_out_desc(desc);
    opt_grd_desc(desc);
    opt_vel_desc(desc);
    opt_ini_desc(desc);
    opt_eqs_desc(desc);
    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    // --help or no argument case
    if (vm.count("help") || ac == 1)
    {
      cerr << desc << endl;
      exit(EXIT_SUCCESS); // this is what GNU coding standards suggest
    }

    // --slv list
    if (vm.count("slv") && vm["slv"].as<string>() == "list")
    {
      cout << "serial fork";
#ifdef _OPENMP
      cout << " openmp fork+openmp";
#endif
#ifdef USE_BOOST_THREAD
      cout << " threads fork+threads";
#endif
#ifdef USE_BOOST_MPI
      cout << " mpi";
#  ifdef USE_BOOST_THREAD
      cout << " mpi+threads";
#  endif
#  ifdef _OPENMP
      cout << " mpi+openmp";
#  endif
#endif
      cout << endl;
      exit(EXIT_FAILURE);
    }

    // string containing all passed options (e.g. for archiving in a netCDF file)
    ostringstream options;
    options << string(av[0]);
    for (int i = 1; i < ac; ++i) options << string(" ") << string(av[i]);

    // --bits (floating point precision choice)
    int bits = vm["bits"].as<int>();
#ifdef USE_FLOAT
    if (sizeof(float) * 8 == bits) 
      mdl<float>(vm, options.str());
    else 
#endif
#ifdef USE_DOUBLE
    if (sizeof(double) * 8 == bits) 
      mdl<double>(vm, options.str());
    else 
#endif
#ifdef USE_LDOUBLE
    if (sizeof(long double) * 8 == bits) 
      mdl<long double>(vm, options.str());
    else 
#endif
#ifdef USE_FLOAT128 // TODO: only if GNU compiler?
    if (sizeof(__float128) * 8 == bits) 
      mdl<__float_128>(vm, options.str());
    else 
#endif
    error_macro("unsupported number of bits (" << bits << ")")
  }
  catch (exception &e)
  {
    cerr << "-- exception cought: " << e.what() << endl;
    cerr << "-- exit: KO" << endl;
    exit(EXIT_FAILURE);
  }
  cerr << "-- exit: OK" << endl;
  exit(EXIT_SUCCESS);
}
