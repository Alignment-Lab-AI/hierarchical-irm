// Copyright 2024
// Apache License, Version 2.0, refer to LICENSE.txt

#include <cctype>
#include <cstdlib>

#include "irm.hh"
#include "pclean/csv.hh"
#include "pclean/pclean_lib.hh"
#include "pclean/schema.hh"

void incorporate_observations(std::mt19937* prng,
                              GenDB *gendb,
                              const DataFrame& df) {
  int num_rows = df.data.begin()->second.size();
  for (int i = 0; i < num_rows; i++) {
    std::map<std::string, ObservationVariant> row_values;
    for (const auto& col : df.data) {
      const std::string& col_name = col.first;
      if (!gendb->schema.query.fields.contains(col_name)) {
        if (i == 0) {
          printf("Schema does not contain %s, skipping ...\n", col_name.c_str());
        }
        continue;
      }
      const std::string& val = col.second[i];
      if (val.empty()) {
        // Don't incorporate missing values.
        // TODO(thomaswc): Allow the user to specify other values that mean
        // missing data.  ("missing", "NA", "nan", etc.).
        continue;
      }

      // Don't allow non-printable characters in val.
      for (const char c: val) {
        if (!std::isprint(c)) {
          printf("Found non-printable character with ascii value %d on line "
                 "%d of column %s in value `%s`.\n",
                 (int) c, i + 2, col_name.c_str(), val.c_str());
          std::exit(1);
        }
      }

      const RelationVariant& rv = gendb->hirm->get_relation(col_name);
      ObservationVariant ov;
      std::visit([&](const auto &r) { ov = r->from_string(val); }, rv);
      row_values[col_name] = ov;
    }
    gendb->incorporate(prng, std::make_pair(i, row_values));
  }
}

// Sample a single "row" into *query_values.  A value is sampled into
// (*query_values)[f] for every query field in the schema.
void make_pclean_sample(
    std::mt19937* prng, GenDB* gendb,
    std::map<std::string, std::string> *query_values) {
  const std::string& record_class = gendb->schema.query.record_class;
  int class_item = gendb->domain_crps[record_class].sample(prng);
  for (const auto& [name, query_field] : gendb->schema.query.fields) {
    T_items entities = gendb->sample_class_ancestors(
        prng, gendb->schema.query.record_class, class_item);

    (*query_values)[query_field.name] = gendb->hirm->sample_and_incorporate_relation(
        prng, query_field.name, entities);
  }
}

DataFrame make_pclean_samples(int num_samples, GenDB *gendb,
                              std::mt19937* prng) {
  DataFrame df;
  for (int i = 0; i < num_samples; i++) {
     std::map<std::string, std::string> query_values;
     make_pclean_sample(prng, gendb, &query_values);
     for (const auto& [column, val] : query_values) {
       df.data[column].push_back(val);
     }

  }
  return df;
}

