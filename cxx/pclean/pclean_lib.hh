// Copyright 2024
// Apache License, Version 2.0, refer to LICENSE.txt

#pragma once

#include "irm.hh"
#include "util_io.hh"
#include "pclean/csv.hh"
#include "pclean/pclean_lib.hh"

// For each non-missing value in the DataFrame df, create an
// observation in the returned T_observations.  The column name of the value
// is used as the relation name, and each entity in each domain is given
// its own unique value.
T_observations translate_observations(
    const DataFrame& df, const T_schema &schema);