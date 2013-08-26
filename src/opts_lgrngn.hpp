/** 
 * @file
 * @copyright University of Warsaw
 * @section LICENSE
 * GPLv3+ (see the COPYING file or http://www.gnu.org/licenses/)
 */

#include <boost/assign/ptr_map_inserter.hpp>  // for 'ptr_map_insert()'

#include "opts_common.hpp"
#include "kin_cloud_2d_lgrngn.hpp"

// string parsing
#include <boost/spirit/include/qi.hpp>    
#include <boost/fusion/adapted/std_pair.hpp> 
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

// simulation and output parameters for micro=lgrngn
template <class solver_t>
void setopts_micro(
  typename solver_t::params_t &params, 
  int nx, int nz, int nt,
  typename std::enable_if<std::is_same<
    decltype(solver_t::params_t::cloudph_opts),
    libcloudphxx::lgrngn::opts_t<typename solver_t::real_t>
  >::value>::type* = 0
)
{
std::cerr << "setopts_lgrngn" << std::endl;
  using thrust_real_t = setup::real_t; // TODO: make it a choice?

  po::options_description opts("Lagrangian microphysics options"); 
  opts.add_options()
    ("backend", po::value<std::string>()->required() , "one of: CUDA, OpenMP, serial")
    ("async", po::value<bool>()->default_value(true), "use CPU for advection while GPU does micro (ignored if backend != CUDA)")
    ("sd_conc_mean", po::value<thrust_real_t>()->required() , "mean super-droplet concentration per grid cell (int)")
    // processes
    ("adve", po::value<bool>()->default_value(true ) , "particle advection     (1=on, 0=off)")
    ("sedi", po::value<bool>()->default_value(true ) , "particle sedimentation (1=on, 0=off)")
    ("cond", po::value<bool>()->default_value(true ) , "condensational growth  (1=on, 0=off)")
    ("coal", po::value<bool>()->default_value(true ) , "collisional growth     (1=on, 0=off)")
    ("rcyc", po::value<bool>()->default_value(false) , "particle recycling     (1=on, 0=off)")
    ("chem", po::value<bool>()->default_value(false) , "aqueous chemistry      (1=on, 0=off)")
    // free parameters
    ("sstp_cond", po::value<int>()->default_value(100), "no. of substeps for condensation")
    ("sstp_coal", po::value<int>()->default_value(1), "no. of substeps for coalescence")
    ("RH_max", po::value<setup::real_t>()->default_value(1.01), "RH limit for drop growth equation")
    // 
    ("out_dry", po::value<std::string>()->default_value(".5e-6:25e-6|0"),       "dry radius ranges and moment numbers (r1:r2|n1,n2...;...)")
    ("out_wet", po::value<std::string>()->default_value(".5e-6:25e-6|0,1,2,3"),  "wet radius ranges and moment numbers (r1:r2|n1,n2...;...)")
    // TODO: MAC, HAC, vent_coef
  ;
  po::variables_map vm;
  handle_opts(opts, vm);
      
  thrust_real_t kappa = .5; // TODO!!!

  std::string backend_str = vm["backend"].as<std::string>();
  if (backend_str == "CUDA") params.backend = libcloudphxx::lgrngn::cuda;
  else if (backend_str == "OpenMP") params.backend = libcloudphxx::lgrngn::omp;
  else if (backend_str == "serial") params.backend = libcloudphxx::lgrngn::cpp;

  params.async = vm["async"].as<bool>();

  params.cloudph_opts.sd_conc_mean = vm["sd_conc_mean"].as<thrust_real_t>();;
  params.cloudph_opts.nx = nx;
  params.cloudph_opts.nz = nz;
  boost::assign::ptr_map_insert<
    setup::log_dry_radii<thrust_real_t> // value type
  >(
    params.cloudph_opts.dry_distros // map
  )(
    kappa // key
  );

  // output variables
  params.outvars = {
    // <TODO>: make it common among all three micro?
    {solver_t::ix::rhod_th, {"rhod_th", "[K kg m-3]"}},
    {solver_t::ix::rhod_rv, {"rhod_rv", "[kg m-3]"}}
    // </TODO>
  };

  // process toggling
  params.cloudph_opts.adve = vm["adve"].as<bool>();
  params.cloudph_opts.sedi = vm["sedi"].as<bool>();
  params.cloudph_opts.cond = vm["cond"].as<bool>();
  params.cloudph_opts.coal = vm["coal"].as<bool>();
  params.cloudph_opts.rcyc = vm["rcyc"].as<bool>();
  params.cloudph_opts.chem = vm["chem"].as<bool>();

  // free parameters
  params.cloudph_opts.sstp_cond = vm["sstp_cond"].as<int>();
  params.cloudph_opts.sstp_coal = vm["sstp_coal"].as<int>();
  params.cloudph_opts.RH_max = vm["RH_max"].as<thrust_real_t>();

  // parsing --out_dry and --out_wet options values
  // the format is: "rmin:rmax|0,1,2;rmin:rmax|3;..."
  for (auto &opt : std::set<std::string>({"out_dry", "out_wet"}))
  {
    namespace qi = boost::spirit::qi;
    namespace phoenix = boost::phoenix;

    std::string val = vm[opt].as<std::string>();
    auto first = val.begin();
    auto last  = val.end();

    std::vector<std::pair<std::string, std::string>> min_maxnum;
    outmom_t<thrust_real_t> &moms = 
      opt == "out_dry"
        ? params.out_dry
        : params.out_wet;

    const bool result = qi::phrase_parse(first, last, 
      *(
	*(qi::char_-":")  >>  qi::lit(":") >>  
	*(qi::char_-";")  >> -qi::lit(";") 
      ),
      boost::spirit::ascii::space, min_maxnum
    );    
    if (!result || first != last) BOOST_THROW_EXCEPTION(po::validation_error(
        po::validation_error::invalid_option_value, opt, val 
    ));  

    for (auto &ss : min_maxnum)
    {
      int sep = ss.second.find('|'); 

      auto iter_status = moms.insert(outmom_t<thrust_real_t>::value_type({outmom_t<thrust_real_t>::key_type(
        boost::lexical_cast<setup::real_t>(ss.first) * si::metres,
        boost::lexical_cast<setup::real_t>(ss.second.substr(0, sep)) * si::metres
      ), outmom_t<setup::real_t>::mapped_type()}));

      // TODO catch (boost::bad_lexical_cast &)

      assert(iter_status.second); // TODO: this does not seem to report anything, ranges should be unique!

      std::string nums = ss.second.substr(sep+1);;
      auto nums_first = nums.begin();
      auto nums_last  = nums.end();

      const bool result = qi::phrase_parse(nums_first, nums_last, 
	(
	  qi::int_[phoenix::push_back(phoenix::ref(iter_status.first->second), qi::_1)]
	      >> *(',' >> qi::int_[phoenix::push_back(phoenix::ref(iter_status.first->second), qi::_1)])
	),
	boost::spirit::ascii::space
      );    
      if (!result || nums_first != nums_last) BOOST_THROW_EXCEPTION(po::validation_error(
	  po::validation_error::invalid_option_value, opt, val // TODO: report only the relevant part?
      ));  
    }
  } 
}
