/******************************************
Copyright (c) 2020, Jeffrey Dudek
******************************************/

#include <string.h>
#include <iostream>
#include <chrono>
#include <algorithm>

#include "../../lib/cxxopts.hpp"
#include "src/formula.h"

int main(int argc, char *argv[]) {
  cxxopts::Options options("deweight",
    "A tool to add random weights to unweighted benchmarks");

  options.add_options()
    ("s, seed", "Seed for random number generator", cxxopts::value<int>())
    ("d, decimal", "Output weights as decimals", cxxopts::value<bool>()->default_value("true"))
    ("u, uniform", "Assign the weight p/q to positive variables", cxxopts::value<std::string>())
    ("r, random", "Assign random weight {1/r, ..., (r-1)/r} to positive variables", cxxopts::value<int>())
    ("q, random_frac", "Assign random weight {1/2, 1/3, 2/3, ..., (q-1)/q} to positive variables", cxxopts::value<int>())
    ("m, mask", "Choose which variables to assign weights to", cxxopts::value<std::string>())
    ("o, output", "Output format to use for weights.\n"
     "--output=cachet\tAs \"w [var] [positive weight]\"\n"
     "--output=ganak\tAs \"w [lit] [weight] 0\"\n"
     "--output=sdimacs\tAs \"r [positive weight] [var] 0\" and \"e [var] 0\"",
     cxxopts::value<deweight::OutputFormat>()->default_value("cachet"))
    ("h, help", "Print usage");
  auto args = options.parse(argc, argv);
  if (args.count("help")) {
    std::cout << options.help() << std::endl;
    exit(0);
  }

  if (args.count("seed")) {
    srand(args["seed"].as<int>());
  } else {
    srand(time(0));
  }

  // Parse formula from stdin
  StreamBuffer<FILE*, FN> in = StreamBuffer<FILE*, FN>(stdin);
  auto formula = deweight::Formula::parse_DIMACS(in);
  if (formula.num_variables() == 0) {
    std::cerr << "Error: Unable to read formula." << std::endl;
    return -1;
  }

  // Determine which variables to weight, using mask
  std::vector<int> literals_to_weight;
  std::string mask = "";
  if (args.count("mask") > 0) {
    mask = args["mask"].as<std::string>();
  }
  if (formula.has_independent_support()) { 
    auto support = formula.get_independent_support();
    for (int i = 0; i < support.size(); i++) {
      if (mask.size() > 0 && (i >= mask.size() || mask.at(i) != '*')) {
        continue;
      }
      literals_to_weight.push_back(support[i]);
    }
  } else {
    for (int i = 0; i < formula.num_variables(); i++) {
      if (mask.size() > 0 && (i >= mask.size() || mask.at(i) != '*')) {
        continue;
      }
      literals_to_weight.push_back(i+1);
    }
  }

  // Weight specified variables
  if (args.count("uniform") > 0) {
    auto w = deweight::Rational::parse(args["uniform"].as<std::string>());
    for (int lit : literals_to_weight) {
      formula.set_weight(lit, w);
    }
  } else if (args.count("random") > 0) {
    int denom = args["random"].as<int>();
    if (denom <= 1) {
      std::cerr << "Error: -r must be 2 or higher." << std::endl;
      return -1;
    }
    for (int lit : literals_to_weight) {
      deweight::Rational weight((rand() % (denom-1)) + 1, denom);
      formula.set_weight(lit, weight);
    }
  } else if (args.count("random_frac") > 0) {
    int max_val = args["random_frac"].as<int>();
    if (max_val <= 1) {
      std::cerr << "Error: -q must be 2 or higher." << std::endl;
      return -1;
    }
    for (int lit : literals_to_weight) {
      int a = 0, b = 0;
      while (a >= b) {
        a = (rand() % max_val) + 1;
        b = (rand() % max_val) + 1;
      }
      deweight::Rational weight(a, b);
      formula.set_weight(lit, weight);
    }
  }

  // Output formula with weights
  formula.write(&std::cout,
                args["decimal"].as<bool>(),
                args["output"].as<deweight::OutputFormat>());
  return 0;
}
