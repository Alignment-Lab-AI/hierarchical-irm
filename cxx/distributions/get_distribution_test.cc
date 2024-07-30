// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test UtilDistributionVariant

#include "get_distribution.hh"

#include <string.h>
#include <typeinfo>
#include <boost/test/included/unit_test.hpp>

namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_distribution_spec) {
  std::mt19937 prng;

  DistributionSpec dbb = DistributionSpec("bernoulli");
  BOOST_TEST((dbb.distribution == DistributionEnum::bernoulli));
  BOOST_TEST(dbb.distribution_args.empty());

  DistributionSpec dbg = DistributionSpec("bigram");
  BOOST_TEST((dbg.distribution == DistributionEnum::bigram));
  BOOST_TEST(dbg.distribution_args.empty());

  DistributionSpec dn = DistributionSpec("normal");
  BOOST_TEST((dn.distribution == DistributionEnum::normal));
  BOOST_TEST(dn.distribution_args.empty());

  DistributionSpec ds = DistributionSpec("skellam");
  BOOST_TEST((ds.distribution == DistributionEnum::skellam));
  BOOST_TEST(ds.distribution_args.empty());

  DistributionSpec dc("categorical(k=6)");
  BOOST_TEST((dc.distribution == DistributionEnum::categorical));
  BOOST_TEST((dc.distribution_args.size() == 1));
  std::string expected = "6";
  BOOST_CHECK_EQUAL(dc.distribution_args.at("k"), expected);

  std::map<std::string, std::string> params = {{"k", "6"}};
  DistributionSpec dc2 = DistributionSpec("categorical", params);
  BOOST_TEST((dc2.distribution == DistributionEnum::categorical));
  BOOST_TEST((dc2.distribution_args.size() == 1));
  std::string expected2 = "6";
  BOOST_CHECK_EQUAL(dc2.distribution_args.at("k"), expected2);

  DistributionSpec dsc = DistributionSpec("stringcat(strings=a b c d)");
  BOOST_TEST((dsc.distribution == DistributionEnum::stringcat));
  BOOST_TEST((dsc.distribution_args.size() == 1));
  BOOST_CHECK_EQUAL(dsc.distribution_args.at("strings"), "a b c d");

  DistributionSpec dsc2 = DistributionSpec("stringcat(strings=yes:no,delim=:)");
  BOOST_TEST((dsc2.distribution == DistributionEnum::stringcat));
  BOOST_TEST((dsc2.distribution_args.size() == 2));
  BOOST_CHECK_EQUAL(dsc2.distribution_args.at("strings"), "yes:no");
}

BOOST_AUTO_TEST_CASE(test_get_prior_bernoulli) {
  std::mt19937 prng;

  DistributionVariant dv = get_prior(DistributionSpec("bernoulli"), &prng);
  Distribution<bool> *d = std::get<Distribution<bool>*>(dv);
  std::string name = typeid(*d).name();
  BOOST_TEST(name.find("BetaBernoulli") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_get_prior_bigram) {
  std::mt19937 prng;

  DistributionVariant dv = get_prior(DistributionSpec("bigram"), &prng);
  Distribution<std::string> *d = std::get<Distribution<std::string>*>(dv);
  std::string name = typeid(*d).name();
  BOOST_TEST(name.find("Bigram") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_get_prior_categorical) {
  std::mt19937 prng;

  DistributionSpec ds("categorical", {{"k", "5"}});
  DistributionVariant dv = get_prior(ds, &prng);
  Distribution<int> *d = std::get<Distribution<int>*>(dv);
  std::string name = typeid(*d).name();
  BOOST_TEST(name.find("DirichletCategorical") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_get_prior_normal) {
  std::mt19937 prng;

  DistributionSpec ds("normal");
  DistributionVariant dv = get_prior(ds, &prng);
  Distribution<double> *d = std::get<Distribution<double>*>(dv);
  std::string name = typeid(*d).name();
  BOOST_TEST(name.find("Normal") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_get_prior_skellam) {
  std::mt19937 prng;

  DistributionSpec ds("skellam");
  DistributionVariant dv = get_prior(ds, &prng);
  Distribution<int> *d = std::get<Distribution<int>*>(dv);
  std::string name = typeid(*d).name();
  BOOST_TEST(name.find("Skellam") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_get_prior_stringcat) {
  std::mt19937 prng;

  DistributionSpec ds("stringcat", {{"strings", "hello world"}});
  DistributionVariant dv = get_prior(ds, &prng);
  Distribution<std::string> *d = std::get<Distribution<std::string>*>(dv);
  std::string name = typeid(*d).name();
  BOOST_TEST(name.find("StringCat") != std::string::npos);
}