// Copyright 2024
// See LICENSE.txt

#pragma once
#include <map>
#include <random>
#include <unordered_map>
#include <unordered_set>

typedef int T_item;

// TODO(emilyaf): Make this a distribution subclass.
class CRP {
 public:
  double alpha = 1.;  // concentration parameter
  int N = 0;          // number of customers
  std::map<int, std::unordered_set<T_item>>
      tables;  // map from table id to set of customers
  std::unordered_map<T_item, int> assignments;  // map from customer to table id

  CRP() {}

  void incorporate(const T_item& item, int table);

  void unincorporate(const T_item& item);

  int sample(std::mt19937* prng);

  double logp_new_table() const;

  double logp(int table) const;

  double logp_score() const;

  // Returns the highest table entry in tables, or -1 if tables is empty.
  int max_table() const;

  std::map<int, double> tables_weights() const;

  std::map<int, double> tables_weights_gibbs(int table) const;

  void transition_alpha(std::mt19937* prng);
};
