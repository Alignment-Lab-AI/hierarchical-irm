// Copyright 2024
// Apache License, Version 2.0, refer to LICENSE.txt

#pragma once

#include <random>
#include "hirm.hh"
#include "irm.hh"

// Functions for running IRM or HIRM inference for a certain number of
// iterations or a timeout (in seconds) is reached.
void inference_irm(std::mt19937* prng, IRM* irm, int iters, int timeout,
                   bool verbose);
void inference_hirm(std::mt19937* prng, HIRM* hirm, int iters, int timeout,
                    bool verbose);