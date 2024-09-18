#include "pclean/schema_helper.hh"

#include <cstdlib>

#include "pclean/get_joint_relations.hh"

PCleanSchemaHelper::PCleanSchemaHelper(const PCleanSchema& s,
                                       bool _only_final_emissions,
                                       bool _record_class_is_clean)
    : schema(s),
      only_final_emissions(_only_final_emissions),
      record_class_is_clean(_record_class_is_clean) {
  compute_domains_cache();
}

void PCleanSchemaHelper::compute_domains_cache() {
  for (const auto& c : schema.classes) {
    if (!domains.contains(c.first)) {
      compute_domains_for(c.first);
    }
  }
}

void PCleanSchemaHelper::compute_domains_for(const std::string& name) {
  std::vector<std::string> ds;
  std::vector<std::string> annotated_ds;
  PCleanClass c = schema.classes[name];

  // Recursively maps the indices of class "name" (and ancestors) in relation
  // items to the names and indices (in items) of their parents (reference
  // fields).
  std::map<int, std::map<std::string, int>> ref_indices;

  // Temporarily stores reference fields and indices for class "name";
  std::map<std::string, int> class_ref_indices;
  for (const auto& v : c.vars) {
    if (const ClassVar* cv = std::get_if<ClassVar>(&(v.second.spec))) {
      if (!domains.contains(cv->class_name)) {
        compute_domains_for(cv->class_name);
      }
      // Indices for foreign-key domains are generated by adding an offset
      // to their indices in the respective class.
      const int offset = ds.size();
      for (const std::string& s : domains[cv->class_name]) {
        ds.push_back(s);
      }
      class_ref_indices[v.first] = ds.size() - 1;
      std::map<std::string, int> child_class_indices;
      if (class_reference_indices.contains(cv->class_name)) {
        for (const auto& [ind, ref] :
             class_reference_indices.at(cv->class_name)) {
          std::map<std::string, int> class_ref_indices;
          for (const auto& [field_name, ref_ind] : ref) {
            child_class_indices[field_name] = ref_ind + offset;
          }
          ref_indices[ind + offset] = child_class_indices;
        }
      }
      for (const std::string& s : annotated_domains[cv->class_name]) {
        annotated_ds.push_back(v.first + ':' + s);
      }
    }
  }

  // Put the "primary" domain last, so that it survives reordering.
  ds.push_back(name);
  annotated_ds.push_back(name);

  domains[name] = ds;
  annotated_domains[name] = annotated_ds;

  // Do not store a `class_reference_indices` entry for classes
  // with no reference fields.
  if (class_ref_indices.size() > 0) {
    ref_indices[ds.size() - 1] = class_ref_indices;
    class_reference_indices[name] = ref_indices;
  }
}

void PCleanSchemaHelper::make_relations_for_queryfield(
    const QueryField& f, const PCleanClass& record_class, T_schema* tschema,
    std::map<std::string, std::vector<std::string>>*
        annotated_domains_for_relation) {
  // First, find all the vars and classes specified in f.class_path.
  std::vector<std::string> var_names;
  std::vector<std::string> class_names;
  PCleanVariable last_var;
  PCleanClass last_class = record_class;
  class_names.push_back(record_class.name);
  for (size_t i = 0; i < f.class_path.size(); ++i) {
    const PCleanVariable& v = last_class.vars[f.class_path[i]];
    last_var = v;
    var_names.push_back(v.name);
    if (i < f.class_path.size() - 1) {
      class_names.push_back(std::get<ClassVar>(v.spec).class_name);
      last_class = schema.classes[class_names.back()];
    }
  }
  // Remove the last var_name because it isn't used in making the path_prefix.
  var_names.pop_back();

  // Get the base relation from the last class and variable name.
  std::string base_relation_name = class_names.back() + ":" + last_var.name;

  // Handle queries of the record class specially.
  if (f.class_path.size() == 1) {
    if (record_class_is_clean) {
      // Just rename the existing clean relation and set it to be observed.
      T_clean_relation cr =
          std::get<T_clean_relation>(tschema->at(base_relation_name));
      cr.is_observed = true;
      (*tschema)[f.name] = cr;
      tschema->erase(base_relation_name);
      (*annotated_domains_for_relation)[f.name] =
          annotated_domains[record_class.name];
    } else {
      T_noisy_relation tnr =
          get_emission_relation(std::get<ScalarVar>(last_var.spec),
                                domains[record_class.name], base_relation_name);
      tnr.is_observed = true;
      (*tschema)[f.name] = tnr;
      (*annotated_domains_for_relation)[f.name] =
          annotated_domains[record_class.name];
      // If the record class is the only class in the schema, there will be
      // no entries in `relation_reference_indices`.
      if (class_reference_indices.contains(record_class.name)) {
        relation_reference_indices[f.name] =
            class_reference_indices.at(record_class.name);
      }
    }
    return;
  }

  // Handle only_final_emissions == true.
  if (only_final_emissions) {
    std::vector<std::string> noisy_domains = domains[class_names.back()];
    std::vector<std::string> adfr = annotated_domains[class_names.back()];
    for (int i = class_names.size() - 2; i >= 0; --i) {
      noisy_domains.push_back(class_names[i]);
      for (size_t j = 0; j < adfr.size(); ++j) {
        adfr[j] = var_names[i] + ":" + adfr[j];
      }
      adfr.push_back(class_names[i]);
      relation_reference_indices[f.name][noisy_domains.size() - 1]
                                [var_names[i]] = noisy_domains.size() - 2;
    }
    T_noisy_relation tnr = get_emission_relation(
        std::get<ScalarVar>(last_var.spec), noisy_domains, base_relation_name);
    tnr.is_observed = true;
    (*tschema)[f.name] = tnr;
    (*annotated_domains_for_relation)[f.name] = adfr;
    // If the record class is the only class in the schema, there will be
    // no entries in `relation_reference_indices`.
    if (relation_reference_indices.contains(base_relation_name)) {
      relation_reference_indices[f.name] =
          relation_reference_indices.at(base_relation_name);
    }
    return;
  }

  // Handle only_final_emissions == false.
  std::string& previous_relation = base_relation_name;
  std::vector<std::string> current_domains = domains[class_names.back()];
  std::vector<std::string> adfr = annotated_domains[class_names.back()];
  std::map<int, std::map<std::string, int>> ref_indices;
  for (int i = f.class_path.size() - 2; i >= 0; --i) {
    current_domains.push_back(class_names[i]);
    for (size_t j = 0; j < adfr.size(); ++j) {
      adfr[j] = var_names[i] + ":" + adfr[j];
    }
    adfr.push_back(class_names[i]);
    ref_indices[current_domains.size() - 1][var_names[i]] =
        current_domains.size() - 2;
    T_noisy_relation tnr = get_emission_relation(
        std::get<ScalarVar>(last_var.spec), current_domains, previous_relation);
    std::string rel_name;
    if (i == 0) {
      rel_name = f.name;
      tnr.is_observed = true;
    } else {
      // Intermediate emissions have a name of the form
      // "[Observing Class]::[QueryFieldName]"
      rel_name = class_names[i] + "::" + f.name;
      tnr.is_observed = false;
    }
    (*tschema)[rel_name] = tnr;
    // Since noisy relations have the leftmost domains in common with their base
    // relations, they share the reference indices with their base relations as
    // well.
    if (relation_reference_indices.contains(previous_relation)) {
      relation_reference_indices[rel_name] =
          relation_reference_indices.at(previous_relation);
    }
    relation_reference_indices[rel_name].merge(ref_indices);
    previous_relation = rel_name;
    (*annotated_domains_for_relation)[rel_name] = adfr;
  }
}

T_schema PCleanSchemaHelper::make_hirm_schema(
    std::map<std::string, std::vector<std::string>>*
        annotated_domains_for_relation) {
  T_schema tschema;

  // For every scalar variable, make a clean relation with the name
  // "[ClassName]:[VariableName]".
  for (const auto& c : schema.classes) {
    for (const auto& v : c.second.vars) {
      std::string rel_name = c.first + ':' + v.first;
      if (const ScalarVar* dv = std::get_if<ScalarVar>(&(v.second.spec))) {
        tschema[rel_name] = get_distribution_relation(*dv, domains[c.first]);
        (*annotated_domains_for_relation)[rel_name] =
            annotated_domains[c.first];
        if (class_reference_indices.contains(c.first)) {
          relation_reference_indices[rel_name] =
              class_reference_indices.at(c.first);
        }
      }
    }
  }

  // For every query field, make one or more relations by walking up
  // the class_path.  At least one of those relations will have name equal
  // to the name of the QueryField.
  const PCleanClass record_class = schema.classes[schema.query.record_class];
  for (const auto& [unused_name, f] : schema.query.fields) {
    make_relations_for_queryfield(f, record_class, &tschema,
                                  annotated_domains_for_relation);
  }

  return tschema;
}
