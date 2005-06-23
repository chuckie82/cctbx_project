// Done by Jacob Smith
#include <cctbx/boost_python/flex_fwd.h>

#include <cctbx/maptbx/abstract_interpolator.h>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/args.hpp>

namespace cctbx { namespace maptbx { namespace boost_python {

namespace {

  struct abstract_interpolator_wrappers
  {

    typedef abstract::interpolator<double> interpolator;
    typedef abstract::get_non_crystallographic<double> get_non_crystallographic;
    typedef abstract::get_cartesian_crystallographic<double> get_cartesian_crystallographic;
    typedef abstract::get_fractional_crystallographic<double> get_fractional_crystallographic;
    typedef abstract::get_cartesian_asu<double> get_cartesian_asu;
    typedef abstract::get_fractional_asu<double> get_fractional_asu;

    BOOST_PYTHON_FUNCTION_OVERLOADS(
      get_non_crystallographic_interpolator_overloads,
      get_non_crystallographic::interpolator, 2, 4)

    static void
    wrap()
    {

      using namespace boost::python;

      class_<interpolator>("abstract_interpolator",no_init)
        .def("interpolate",
          (double(interpolator::*)( scitbx::vec3<double> const& ) const)
          &interpolator::interpolate,
          (arg_("site")))
        .def("interpolate",
          (af::shared<double>(interpolator::*)( af::const_ref<scitbx::vec3<double> > const& ) const)
          &interpolator::interpolate,
          (arg_("sites")));

      def("get_non_crystallographic_interpolator",
        (interpolator(*)
          (af::versa<double, af::flex_grid<> > const&,
          scitbx::mat3<double> const&,
          bool,
          double const&))
          get_non_crystallographic::interpolator,
        get_non_crystallographic_interpolator_overloads((
          arg_("map"),
          arg_("gridding_matrix"),
          arg_("allow_out_of_bounds")=true,
          arg_("out_of_bounds_substitute_value")=0.0)));

      def("get_fractional_crystallographic_interpolator",
        (interpolator(*)
          (af::versa<double, af::flex_grid<> > const&))
          get_fractional_crystallographic::interpolator,
        (arg_("map")));

      def("get_cartesian_crystallographic_interpolator",
        (interpolator(*)
          (af::versa<double, af::flex_grid<> > const&,
          scitbx::mat3<double> const&))
          get_cartesian_crystallographic::interpolator,
        (arg_("map"),arg_("fractionalization_matrix")));

      def("get_fractional_asu_interpolator",
        (interpolator(*)
          (af::versa<double, af::flex_grid<> > const&, crystal::direct_space_asu::asu_mappings<double> &))
          get_fractional_asu::interpolator,
        (arg_("map"),arg_("asu_mappings")));

      def("get_cartesian_asu_interpolator",
        (interpolator(*)
          (af::versa<double, af::flex_grid<> > const&,
          crystal::direct_space_asu::asu_mappings<double> &))
          get_cartesian_asu::interpolator,
        (arg_("map"),arg_("asu_mappings")));

    }
  };

} // namespace <anoymous>

  void wrap_abstract_interpolators()
  {
    abstract_interpolator_wrappers::wrap();
  }

}}} // namespace cctbx::maptbx::boost_python
