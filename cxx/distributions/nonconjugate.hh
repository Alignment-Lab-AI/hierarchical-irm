#pragma once

#include <map>
#include <random>
#include "distributions/base.hh"

template <typename T>
class NonconjugateDistribution : public Distribution<T> {
 public:
  // Abstract base class for Distributions that don't have conjugate priors.

  // The log probability of x given the current latent values.
  virtual double logp(const T& x) const = 0;

  // Sample a value from the distribution given the current latent values.
  virtual T sample(std::mt19937* prng) = 0;

  // Transition hyperparameters given the current latent values.
  virtual void transition_hyperparameters(std::mt19937* prng) = 0;

  // Set the current latent values to a sample from the parameter prior.
  virtual void init_theta(std::mt19937* prng) = 0;

  // Transition the current latent values.
  virtual void transition_theta(std::mt19937* prng) = 0;

  double cumulative_logp = 0.0;

  void incorporate(const T& x) {
    (this->N)++;
    cumulative_logp += logp(x);
  };

  void unincorporate(const T& x) {
    --(this->N);
    cumulative_logp -= logp(x);
  };

  double logp_score() const {
    return cumulative_logp;
  }
};