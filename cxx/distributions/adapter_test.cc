// Copyright 2024
// Refer to LICENSE.txt

#define BOOST_TEST_MODULE test Normal

#include "distributions/adapter.hh"

#include <boost/test/included/unit_test.hpp>

#include "distributions/normal.hh"
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(adapt_normal) {
  Normal* n = new Normal;
  DistributionAdapter<double> ad(n);

  ad.incorporate("5.0");
  ad.incorporate("-2.0");
  BOOST_TEST(n->N == ad.N);

  ad.unincorporate("5.0");
  ad.incorporate("7.0");
  BOOST_TEST(n->N == ad.N);

  ad.unincorporate("-2.0");
  BOOST_TEST(n->N == ad.N);

  BOOST_TEST(ad.logp("6.0") == n->logp(6.), tt::tolerance(1e-6));
  BOOST_TEST(ad.logp_score() == n->logp_score(), tt::tolerance(1e-6));

  std::mt19937 prng;
  std::string samp = ad.sample(&prng);
}

