// Copyright 2024
// See LICENSE.txt

#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

struct ScalarVar {
  // joint_name is the name for the joint distribution/emission combination.
  std::string joint_name;
  std::map<std::string, std::string> params;
};


struct ClassVar {
  std::string class_name;
};

struct PCleanVariable {
  std::string name;
  std::variant<ScalarVar, ClassVar> spec;
};

struct PCleanClass {
  std::string name;
  // Map from variable name to variable.
  std::map<std::string, PCleanVariable> vars;
  // TODO(thomaswc): Figure out how to handle class level configurations.
};

// TODO(emilyaf): Maybe get rid of name here, since it's redundant, and just
// do typedef vector<string> QueryField.
struct QueryField {
  // "physician.school_name as School" gets parsed as
  // {name="School", class_path=["physician","school"]}.
  std::string name;
  std::vector<std::string> class_path;
};

struct PCleanQuery {
  std::string record_class;
  std::map<std::string, QueryField> fields;
};

struct PCleanSchema {
  // Map from class name to class.
  std::map<std::string, PCleanClass> classes;
  PCleanQuery query;
};
