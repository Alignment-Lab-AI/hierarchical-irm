// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test HIRM

#include "hirm.hh"

#include <boost/test/included/unit_test.hpp>
#include <random>

#include "distributions/get_distribution.hh"

namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_hirm) {
  std::map<std::string, T_relation> schema1{
      {"R5",
       T_noisy_relation{
           {"D1", "D1", "D2"}, true, EmissionSpec("sometimes_bitflip"), "R1"}},
      {"R1",
       T_clean_relation{{"D1", "D1"}, false, DistributionSpec("bernoulli")}},
      {"R6",
       T_noisy_relation{
           {"D1", "D1"}, true, EmissionSpec("sometimes_bitflip"), "R1"}},
      {"R7",
       T_noisy_relation{
           {"D1", "D1", "D3"}, true, EmissionSpec("sometimes_bitflip"), "R6"}},
      {"R2", T_clean_relation{{"D1", "D2"}, false, DistributionSpec("normal")}},
      {"R3", T_clean_relation{{"D3", "D1"}, true, DistributionSpec("bigram")}},
      {"R4", T_noisy_relation{{"D1", "D2", "D3"},
                              true,
                              EmissionSpec("sometimes_gaussian"),
                              "R2"}}};

  std::mt19937 prng;
  HIRM hirm(schema1, &prng);

  hirm.transition_cluster_assignments_all(&prng);

  hirm.remove_relation("R3");

  double logp_x = hirm.logp({{"R1", {1, 2}, false}}, &prng);
  BOOST_TEST(logp_x < 0.0);

  hirm.incorporate(&prng, "R1", {1, 2}, false);
  hirm.incorporate(&prng, "R5", {1, 2, 0}, true);
  hirm.incorporate(&prng, "R5", {1, 2, 4}, false);
  hirm.incorporate(&prng, "R2", {0, 3}, 0.5);
  hirm.incorporate(&prng, "R4", {0, 3, 1}, 0.7);
  hirm.incorporate(&prng, "R4", {0, 3, 2}, 0.4);
  hirm.incorporate(&prng, "R4", {0, 3, 4}, 0.55);
  BOOST_TEST(hirm.logp_score() < 0.0);

  // Transitioning clusters should preserve the number of observations.
  hirm.transition_cluster_assignments_all(&prng);
  int r2_obs = std::visit([](auto r) { return r->get_data().size(); },
                          hirm.get_relation("R2"));
  int r5_obs = std::visit([](auto r) { return r->get_data().size(); },
                          hirm.get_relation("R5"));
  int r4_obs = std::visit([](auto r) { return r->get_data().size(); },
                          hirm.get_relation("R4"));
  BOOST_TEST(r2_obs == 1);
  BOOST_TEST(r4_obs == 3);
  BOOST_TEST(r5_obs == 2);

  hirm.set_cluster_assignment_gibbs(&prng, "R6", 4);
  BOOST_TEST(hirm.logp_score() < 0.0);

  // TODO: Enable this when NoisyRelation supports cluster_or_prior_logp.
  // double logp_x_r6 = hirm.logp({{"R6", {1, 2}, false}}, &prng);
  // BOOST_TEST(logp_x_r6 < 0.0);

  hirm.transition_latent_values_relation(&prng, "R2");
  Relation<double>* R2 = std::get<Relation<double>*>(hirm.get_relation("R2"));
  BOOST_TEST(R2->get_data().at({0, 3}) != 0.5);
}

BOOST_AUTO_TEST_CASE(test_hirm_sample) {
  std::map<std::string, T_relation> schema2{
      {"R2",
       T_noisy_relation{
           {"D1", "D1", "D2"}, true, EmissionSpec("sometimes_bitflip"), "R1"}},
      {"R1",
       T_clean_relation{{"D1", "D1"}, false, DistributionSpec("bernoulli")}},
      {"R3",
       T_noisy_relation{
           {"D1", "D1", "D5"}, true, EmissionSpec("sometimes_bitflip"), "R1"}},
      {"R4", T_clean_relation{{"D1", "D3"}, false, DistributionSpec("normal")}},
      {"R5", T_noisy_relation{{"D1", "D3", "D4"},
                              true,
                              EmissionSpec("sometimes_gaussian"),
                              "R4"}}};

  std::mt19937 prng;
  HIRM hirm(schema2, &prng);
  T_encoded_observations samples = hirm.sample_and_incorporate(&prng, 5);

  BOOST_TEST(
      std::get<Relation<bool>*>(hirm.get_relation("R2"))->get_data().size() ==
      5);
  BOOST_TEST(
      std::get<Relation<bool>*>(hirm.get_relation("R3"))->get_data().size() ==
      5);
  BOOST_TEST(
      std::get<Relation<double>*>(hirm.get_relation("R5"))->get_data().size() ==
      5);
  int nobs_R4 =
      std::get<Relation<double>*>(hirm.get_relation("R4"))->get_data().size();
  BOOST_TEST(nobs_R4 > 0);
  BOOST_TEST(nobs_R4 <= 5);
  BOOST_TEST(hirm.logp_score() < 0.0);

  BOOST_TEST(samples.at("R2").size() == 5);
  BOOST_TEST(samples.at("R3").size() == 5);
  BOOST_TEST(samples.at("R5").size() == 5);
}

BOOST_AUTO_TEST_CASE(test_hirm_relation_names) {
  std::mt19937 prng;
  std::map<std::string, T_relation> schema1{
      {"a", T_noisy_relation{{"domain1"}, true, EmissionSpec("bigram"), "b"}},
      {"b", T_clean_relation{{"domain1"}, false, DistributionSpec("bigram")}}};
  HIRM hirm(schema1, &prng);
  hirm.transition_cluster_assignments_all(&prng);
}
