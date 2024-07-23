// Copyright 2021 MIT Probabilistic Computing Project
// Apache License, Version 2.0, refer to LICENSE.txt

#include "util_io.hh"
#include "util_parse.hh"

#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

void verify_noisy_relation_domains(const T_schema& schema) {
  for (const auto& [relname, trelation] : schema) {
    std::visit(
        [&](const auto& trel) {
          using T = std::decay_t<decltype(trel)>;
          if constexpr (std::is_same_v<T, T_noisy_relation>) {
            std::vector<std::string> base_domains = std::visit(
                [&](const auto& brel) { return brel.domains; },
                schema.at(trel.base_relation));
            for (size_t i = 0; i != base_domains.size(); ++i) {
              assert(base_domains[i] == trel.domains[i] &&
                     "The first domains of a noisy relation must be the same " ￼                     "as the domains of the base relation.");
            }
          }
        }, trelation);
  }
}

T_schema load_schema(const std::string& path) {
  std::ifstream fp(path, std::ifstream::in);
  assert(fp.good());

  T_schema schema;
  std::string line;
  std::vector<Token> tokens;
  while (std::getline(fp, line)) {
    tokens.clear();
    assert(tokenize(line, &tokens));

    if (tokens.empty()) {
      continue;
    }

    if (tokens.size() < 6) {
      printf("Error parsing schema line %s:  expected at least six tokens, "
             "got %ld instead.\n", line.c_str(), tokens.size());
      assert(false);
    }

    if (tokens[0].type != TokenType::identifier) {
      printf("Error parsing schema line %s:  expected relation name as first token\n", line.c_str());
      printf("Got %s of type %d instead\n",
             tokens[0].val.c_str(), (int)(tokens[0].type));
      assert(false);
    }
    std::string relname = tokens[0].val;

    if (tokens[1].val != "~") {
      printf("Error parsing schema line %s:  expected '~' as second token\n", line.c_str());
      printf("Got %s of type %d instead\n",
             tokens[1].val.c_str(), (int)(tokens[1].type));
      assert(false);
    }

    if (tokens[2].type != TokenType::identifier) {
      printf("Error parsing schema line %s:  expected distribution or "
             "emission name as third token\n", line.c_str());
      printf("Got %s of type %d instead\n",
             tokens[2].val.c_str(), (int)(tokens[2].type));
      assert(false);
    }
    std::string dist_name = tokens[2].val;

    size_t i = 3;
    std::map<std::string, std::string> params;
    if (tokens[i].val == '[') { // Handle parameters
      do {
        i += 1;
        if (tokens[i].type != TokenType::identifier) {
          printf("Error parsing schema line %s: expected identifier as parameter name\n", line.c_str());
          printf("Got %s of type %d instead\n",
                 tokens[i].val.c_str(), (int)(tokens[i].type));
          assert(false);
        }
        std::string param_name = tokens[i++].val;

        if (tokens[i].val != "=") {
          printf("Error parsing schema line %s: expected '=' in parameter definition\n", line.c_str());
          printf("Got %s of type %d instead\n",
                 tokens[i].val.c_str(), (int)(tokens[i].type));
          assert(false);
        }
        i += 1;

        params[name] = tokens[i++].val;

        if ((tokens[i].val != ",") && (tokens[i].val != ']')) {
          printf("Error parsing schema line %s: expected ',' or ']' after parameter definition\n", line.c_str());
          printf("Got %s of type %d instead\n",
                 tokens[i].val.c_str(), (int)(tokens[i].type));
          assert(false);
        }

      } while (tokens[i].val == ',');
      i += 1;
    }

    if (tokens[i].val != "(") {
      printf("Error parsing schema line %s:  expected ( as %ld-th token\n",
             line.c_str(), i+1);
      printf("Got %s of type %d instead\n",
             tokens[i].val.c_str(), (int)(tokens[i].type));
      assert(false);
    }
    i += 1;

    // Handle domains and maybe base emission
    std::vector<std::string> base_relation;
    std::vector<std::string> domains;

    if (tokens[i].type != TokenType::identifier) {
      printf("Error parsing schema line %s:  expected identifier after '('.\n"
             line.c_str());
      printf("Got %s of type %d instead\n",
             tokens[i].val.c_str(), (int)(tokens[i].type));
      assert(false);
    }

    if (tokens[i+1].val == ";") {  // We have an emission!
      base_relation = tokens[i].val;
      i += 2;
    }

    do {
      domains.push_back(tokens[i++]);

      if ((tokens[i].val != ",") && (tokens[i].val != ')')) {
        printf("Error parsing schema line %s: expected ',' or ')' after domain\n", line.c_str());
        printf("Got %s of type %d instead\n",
               tokens[i].val.c_str(), (int)(tokens[i].type));
        assert(false);
      }
    } while (tokens[i++].val == ',');

    if (input_relation.empty()) { // Clean relation
      T_clean_relation relation;
      relation.domains = domains;
      assert(relation.domains.size() > 0);
      relation.distribution_spec = DistributionSpec(dist_name, params);
      schema[relname] = relation;
    } else {
      T_noisy_relation relation;
      relation.domains = domains;
      assert(relation.domains.size() > 0);
      relation.emission_spec = EmissionSpec(dist_name, params);
      relation.base_relation = base_relation;
      // If the data contains observations of this relation, this bool will be
      // overwritten to true.
      relation.is_observed = false;
      schema[relname] = relation;
    }
  }
  fp.close();

  verify_noisy_relation_domains(schema);

  return schema;
}

T_observations load_observations(const std::string& path,
                                 const T_schema& schema) {
  std::ifstream fp(path, std::ifstream::in);
  assert(fp.good());

  T_observations observations;
  std::string line;
  while (std::getline(fp, line)) {
    std::istringstream stream(line);

    std::string value_str;
    std::string relname;
    std::vector<std::string> items;

    stream >> value_str;
    stream >> relname;
    ObservationVariant value = observation_string_to_value(
        value_str,
        std::visit([](const auto& trel) { 
          using T = std::decay_t<decltype(trel)>;
          if constexpr (std::is_same_v<T, T_clean_relation>) {
            return trel.distribution_spec.observation_type; 
          } else if constexpr (std::is_same_v<T, T_noisy_relation>) {
            return trel.emission_spec.observation_type;
          } else {
            assert(false && "Unrecognized relation type.");
          }
        }, schema.at(relname)));
    for (std::string w; stream >> w;) {
      items.push_back(w);
    }
    assert(items.size() > 0);
    auto entry = std::make_tuple(relname, items, value);
    observations.push_back(entry);
  }
  fp.close();
  return observations;
}

// Assumes that T_item is integer.
T_encoding encode_observations(const T_schema& schema,
                               const T_observations& observations) {
  // Counter and encoding maps.
  std::map<std::string, int> domain_item_counter;
  T_encoding_f item_to_code;
  T_encoding_r code_to_item;
  // Create a counter of items for each domain.
  for (const auto& [r, relation] : schema) {
    for (const std::string& domain :
         std::visit([](const auto& r) { return r.domains; }, relation)) {
      domain_item_counter[domain] = 0;
      item_to_code[domain] = std::map<std::string, T_item>();
      code_to_item[domain] = std::map<T_item, std::string>();
    }
  }
  // Create the codes for each item.
  for (const T_observation& i : observations) {
    std::string relation = std::get<0>(i);
    std::vector<std::string> items = std::get<1>(i);
    int counter = 0;
    std::vector<std::string> domains =
        std::visit([](const auto& r) { return r.domains; }, schema.at(relation));
    for (const std::string& item : items) {
      // Obtain domain that item belongs to.
      std::string domain = domains.at(counter);
      // Compute its code, if necessary.
      if (!item_to_code.at(domain).contains(item)) {
        int code = domain_item_counter[domain];
        item_to_code[domain][item] = code;
        code_to_item[domain][code] = item;
        ++domain_item_counter[domain];
      }
      ++counter;
    }
  }
  return std::make_pair(item_to_code, code_to_item);
}

void incorporate_observations(std::mt19937* prng, IRM& irm,
                              const T_encoding& encoding,
                              const T_observations& observations) {
  T_encoding_f item_to_code = std::get<0>(encoding);
  for (const auto& [relation, items, value] : observations) {
    int counter = 0;
    T_items items_e;
    std::vector<std::string> domains = std::visit(
        [&](const auto& trel) { return trel.domains; }, irm.schema.at(relation));
    for (const std::string& item : items) {
      std::string domain = domains[counter];
      counter += 1;
      int code = item_to_code.at(domain).at(item);
      items_e.push_back(code);
    }
    irm.incorporate(prng, relation, items_e, value);
  }
}

void incorporate_observations(std::mt19937* prng, HIRM& hirm,
                              const T_encoding& encoding,
                              const T_observations& observations) {
  T_encoding_f item_to_code = std::get<0>(encoding);
  for (const auto& [relation, items, value] : observations) {
    int counter = 0;
    T_items items_e;
    std::vector<std::string> domains = std::visit(
        [&](const auto& trel) { return trel.domains; }, hirm.schema.at(relation));
    for (const std::string& item : items) {
      std::string domain = domains[counter];
      counter += 1;
      int code = item_to_code.at(domain).at(item);
      items_e.push_back(code);
    }
    std::visit(
        [&](const auto& v) { hirm.incorporate(prng, relation, items_e, v); },
        value);
  }
}

void to_txt(std::ostream& fp, const IRM& irm, const T_encoding& encoding) {
  T_encoding_r code_to_item = std::get<1>(encoding);
  for (const auto& [d, domain] : irm.domains) {
    auto i0 = domain->crp.tables.begin();
    auto i1 = domain->crp.tables.end();
    std::map<int, std::unordered_set<T_item>> tables(i0, i1);
    for (const auto& [table, items] : tables) {
      fp << domain->name << " ";
      fp << table << " ";
      int i = 1;
      for (const T_item& item : items) {
        fp << code_to_item.at(domain->name).at(item);
        if (i++ < std::ssize(items)) {
          fp << " ";
        }
      }
      fp << "\n";
    }
  }
}

void to_txt(std::ostream& fp, const HIRM& hirm, const T_encoding& encoding) {
  // Write the relation clusters.
  auto i0 = hirm.crp.tables.begin();
  auto i1 = hirm.crp.tables.end();
  std::map<int, std::unordered_set<T_item>> tables(i0, i1);
  for (const auto& [table, rcs] : tables) {
    fp << table << " ";
    int i = 1;
    for (const T_item rc : rcs) {
      fp << hirm.code_to_relation.at(rc);
      if (i++ < std::ssize(rcs)) {
        fp << " ";
      }
    }
    fp << "\n";
  }
  fp << "\n";
  // Write the IRMs.
  int j = 0;
  for (const auto& [table, rcs] : tables) {
    const IRM* const irm = hirm.irms.at(table);
    fp << "irm=" << table << "\n";
    to_txt(fp, *irm, encoding);
    if (j < std::ssize(tables) - 1) {
      fp << "\n";
      j += 1;
    }
  }
}

void to_txt(const std::string& path, const IRM& irm,
            const T_encoding& encoding) {
  std::ofstream fp(path);
  assert(fp.good());
  to_txt(fp, irm, encoding);
  fp.close();
}

void to_txt(const std::string& path, const HIRM& hirm,
            const T_encoding& encoding) {
  std::ofstream fp(path);
  assert(fp.good());
  to_txt(fp, hirm, encoding);
  fp.close();
}

std::map<std::string, std::map<int, std::vector<std::string>>>
load_clusters_irm(const std::string& path) {
  std::ifstream fp(path, std::ifstream::in);
  assert(fp.good());

  std::map<std::string, std::map<int, std::vector<std::string>>> clusters;
  std::string line;
  while (std::getline(fp, line)) {
    std::istringstream stream(line);

    std::string domain;
    int table;
    std::vector<std::string> items;

    stream >> domain;
    stream >> table;
    for (std::string w; stream >> w;) {
      items.push_back(w);
    }
    assert(items.size() > 0);
    assert(clusters[domain].count(table) == 0);
    clusters[domain][table] = items;
  }
  fp.close();
  return clusters;
}

int isnumeric(const std::string& s) {
  for (char c : s) {
    if (!isdigit(c)) {
      return false;
    }
  }
  return !s.empty() && true;
}

std::tuple<std::map<int, std::vector<std::string>>,  // x[table] = {relation
                                                     // list}
           std::map<
               int,
               std::map<
                   std::string,
                   std::map<int, std::vector<
                                     std::string>>>>  // x[table][domain][table]
                                                      // =
                                                      // {item
                                                      // list}
           >
load_clusters_hirm(const std::string& path) {
  std::ifstream fp(path, std::ifstream::in);
  assert(fp.good());

  std::map<int, std::vector<std::string>> relations;
  std::map<int, std::map<std::string, std::map<int, std::vector<std::string>>>>
      irms;

  std::string line;
  int irmc = 0;

  while (std::getline(fp, line)) {
    std::istringstream stream(line);

    std::string first;
    stream >> first;

    // Parse a relation cluster.
    if (isnumeric(first)) {
      int table = std::stoi(first);
      std::vector<std::string> items;
      for (std::string item; stream >> item;) {
        items.push_back(item);
      }
      assert(items.size() > 0);
      assert(relations.count(table) == 0);
      relations[table] = items;
      continue;
    }

    // Skip a new line.
    if (first.size() == 0) {
      irmc = -1;
      continue;
    }

    // Parse an irm= line.
    if (first.rfind("irm=", 0) == 0) {
      assert(irmc == -1);
      assert(first.size() > 4);
      std::string x = first.substr(4);
      irmc = std::stoi(x);
      assert(irms.count(irmc) == 0);
      irms[irmc] = {};
      continue;
    }

    // Parse a domain cluster.
    assert(irmc > -1);
    assert(irms.count(irmc) == 1);
    std::string second;
    stream >> second;
    assert(second.size() > 0);
    assert(isnumeric(second));
    std::string& domain = first;
    int table = std::stoi(second);
    std::vector<std::string> items;
    for (std::string item; stream >> item;) {
      items.push_back(item);
    }
    assert(items.size() > 0);
    if (irms.at(irmc).count(domain) == 0) {
      irms.at(irmc)[domain] = {};
    }
    assert(irms.at(irmc).at(domain).count(table) == 0);
    irms.at(irmc).at(domain)[table] = items;
  }

  assert(relations.size() == irms.size());
  for (const auto& [t, rs] : relations) {
    assert(irms.count(t) == 1);
  }
  fp.close();
  return std::make_pair(relations, irms);
}

void from_txt(std::mt19937* prng, IRM* const irm,
              const std::string& path_schema, const std::string& path_obs,
              const std::string& path_clusters) {
  // Load the data.
  T_schema schema = load_schema(path_schema);
  T_observations observations = load_observations(path_obs, schema);
  T_encoding encoding = encode_observations(schema, observations);
  auto clusters = load_clusters_irm(path_clusters);
  // Add the relations.
  assert(irm->schema.empty());
  assert(irm->domains.empty());
  assert(irm->relations.empty());
  assert(irm->domain_to_relations.empty());
  for (const auto& [r, ds] : schema) {
    std::visit([&](const auto& trel) { irm->add_relation(r, trel); }, ds);
  }
  // Add the domain entities with fixed clustering.
  T_encoding_f item_to_code = std::get<0>(encoding);
  for (const auto& [domain, tables] : clusters) {
    assert(irm->domains.at(domain)->items.size() == 0);
    for (const auto& [table, items] : tables) {
      assert(0 <= table);
      for (const std::string& item : items) {
        T_item code = item_to_code.at(domain).at(item);
        irm->domains.at(domain)->incorporate(prng, code, table);
      }
    }
  }
  // Add the observations.
  incorporate_observations(prng, *irm, encoding, observations);
}

void from_txt(std::mt19937* prng, HIRM* const hirm,
              const std::string& path_schema, const std::string& path_obs,
              const std::string& path_clusters) {
  T_schema schema = load_schema(path_schema);
  T_observations observations = load_observations(path_obs, schema);
  T_encoding encoding = encode_observations(schema, observations);
  auto [relations, irms] = load_clusters_hirm(path_clusters);
  // Add the relations.
  assert(hirm->schema.empty());
  assert(hirm->irms.empty());
  assert(hirm->relation_to_code.empty());
  assert(hirm->code_to_relation.empty());
  for (const auto& [r, ds] : schema) {
    hirm->add_relation(prng, r, ds);
    assert(hirm->irms.size() == hirm->crp.tables.size());
    hirm->set_cluster_assignment_gibbs(prng, r, -1);
  }
  // Add each IRM.
  for (const auto& [table, rs] : relations) {
    assert(hirm->irms.size() == hirm->crp.tables.size());
    // Add relations to the IRM.
    for (const std::string& r : rs) {
      assert(hirm->irms.size() == hirm->crp.tables.size());
      int table_current = hirm->relation_to_table(r);
      if (table_current != table) {
        assert(hirm->irms.size() == hirm->crp.tables.size());
        hirm->set_cluster_assignment_gibbs(prng, r, table);
      }
    }
    // Add the domain entities with fixed clustering to this IRM.
    // TODO: Duplicated code with from_txt(IRM)
    IRM* irm = hirm->irms.at(table);
    auto clusters = irms.at(table);
    assert(irm->relations.size() == rs.size());
    T_encoding_f item_to_code = std::get<0>(encoding);
    for (const auto& [domain, tables] : clusters) {
      assert(irm->domains.at(domain)->items.size() == 0);
      for (const auto& [t, items] : tables) {
        assert(0 <= t);
        for (const std::string& item : items) {
          int code = item_to_code.at(domain).at(item);
          irm->domains.at(domain)->incorporate(prng, code, t);
        }
      }
    }
  }
  assert(hirm->irms.count(-1) == 0);
  // Add the observations.
  incorporate_observations(prng, *hirm, encoding, observations);
}
