// Copyright 2024
// See LICENSE.txt

// A class for turning Distribution<SampleType>'s into
// Distribution<string>'s.

#pragma once
#include <iostream>
#include <sstream>
#include <string>

#include "distributions/base.hh"

template <typename S = double>
class DistributionAdapter : public Distribution<std::string> {
 public:
  // The underlying distribution that is being adapted.  We own the
  // underlying Distribution.
  Distribution<S>* d;

  DistributionAdapter(Distribution<S>* dd) : d(dd) {};

  S from_string(const std::string& x) const {
    S s;
    std::istringstream(x) >> s;
    return s;
  }

  std::string to_string(const S& s) const {
    std::ostringstream os;
    os << s;
    return os.str();
  }

  void incorporate(const std::string& x, double weight = 1.0) {
    S s = from_string(x);
    N += weight;
    d->incorporate(s, weight);
  }

  double logp(const std::string& x) const {
    S s = from_string(x);
    return d->logp(s);
  }

  double logp_score() const { return d->logp_score(); }

  std::string sample(std::mt19937* prng) {
    S s = d->sample(prng);
    return to_string(s);
  }

  void transition_hyperparameters(std::mt19937* prng) {
    d->transition_hyperparameters(prng);
  }

  void init_theta(std::mt19937* prng) { d->init_theta(prng); }
  void transition_theta(std::mt19937* prng) { d->transition_theta(prng); }

  // TODO(thomaswc): Define nearest methods for the DistributionAdapter
  // instantiations we use.

  ~DistributionAdapter() { delete d; }
};
