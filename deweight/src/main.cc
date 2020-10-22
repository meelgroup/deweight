/******************************************
Copyright (c) 2020, Jeffrey Dudek
******************************************/

#include <string.h>
#include <iostream>
#include <chrono>
#include <algorithm>

#include "../lib/cxxopts.hpp"
#include "../lib/BigInt.hpp"
#include "src/formula.h"


/**
 * Returns the clauses for a formula with the provided variables
 * with the specified number of solutions.
 */
std::vector<std::vector<int>> chain_formula(const std::vector<size_t> &vars,
                                            int num_solutions) {
  // For weight 0, return an UNSAT formula
  if (num_solutions == 0) {
    return {{static_cast<int>(vars[0])}, {-static_cast<int>(vars[0])}};
  }

  // For maximum weight, return the completely SAT formula
  if (num_solutions == (1 << vars.size())) {
    return {};
  }

  if (num_solutions >= (1 << vars.size())) {
    std::cerr << "Unable to form " << num_solutions << "solutions";
    std::cerr << " with " << vars.size() << " variables" << std::endl;
    return {};
  }

  std::vector<std::vector<int>> clauses;
  int bit = 0;

  // If the lowest bits are 0, ignore that variable.
  while ((num_solutions & (1 << bit)) == 0) {
    bit++;
  }

  // Add a clause for the first 1 bit
  clauses.push_back({static_cast<int>(vars[bit])});
  bit++;

  for (; bit < vars.size(); bit++) {
    if ((num_solutions & (1 << bit)) > 0) {
      // 1 bit implies disjunction in the chain formula
      for (int i = 0; i < clauses.size(); i++) {
        clauses[i].push_back(static_cast<int>(vars[bit]));
      }
    } else {
      clauses.push_back({static_cast<int>(vars[bit])});
    }
  }
  return clauses;
}

/**
 * Add clauses to [formula] so that all weights are captured in the clauses.
 */
BigInt reduce(deweight::Formula *formula) {
  std::vector<int> free_variables = formula->get_independent_support();
  // If there is no independent support, consider all variables
  if (free_variables.size() == 0) {
    for (int var = 1; var <= formula->num_variables(); var++) {
      free_variables.push_back(var);
    }
  }

  BigInt net_denom = 1;
  for (int var : free_variables) {
    deweight::Rational pos = formula->get_weight(var);
    deweight::Rational neg = formula->get_weight(-var);

    // Ensure both weights have identical denominators
    int pos_sol, neg_sol, denom;
    if (pos.denom == neg.denom) {
      pos_sol = pos.num;
      neg_sol = neg.num;
      denom = pos.denom;
    } else {
      pos_sol = pos.num * neg.denom;
      neg_sol = neg.num * pos.denom;
      denom = pos.denom * neg.denom;
    }

    // Simplify weights if possible
    int gcd = std::__gcd(pos_sol, neg_sol);
    gcd = std::__gcd(gcd, denom);
    pos_sol /= gcd;
    neg_sol /= gcd;
    denom /= gcd;

    if (pos_sol < 0 || neg_sol < 0) {
      std::cerr << "Skipping var " << var << " (negative weight)" << std::endl;
      continue;
    }

    // Add clauses to [formula] so that:
    //   var  -> pos_sol solutions
    //   -var -> neg_sol solutions

    // Add the number of variables needed to represent both weights.
    // (n variables can represent <= 2^n)
    std::vector<size_t> vars;
    int log = 0;
    while ((1 << log) < pos_sol || (1 << log) < neg_sol) {
      vars.push_back(formula->add_variable());
      if (formula->has_independent_support()) {
        formula->add_independent_support(vars.back());
      }
      log++;
    }

    // var  -> [pos_sol] solutions
    if (pos_sol == 0) {
      formula->add_clause({-var});
    } else {
      for (auto &clause : chain_formula(vars, pos_sol)) {
        clause.insert(clause.begin(), -var);
        formula->add_clause(clause);
      }
    }
    // -var -> [neg weight] solutions
    if (neg_sol == 0) {
      formula->add_clause({var});
    } else {
      for (auto &clause : chain_formula(vars, neg_sol)) {
        clause.insert(clause.begin(), var);
        formula->add_clause(clause);
      }
    }

    net_denom *= denom;
  }
  return net_denom;
}

/**
 * Using the dyadic reduction, add clauses to [formula] so that all weights are 
 * captured in the clauses.
 * 
 * The denominators of weights must be powers of 2. All weights are rounded
 * to the nearest factor of 1/2^[bits_per_var] (rounding positive weight down).
 */
BigInt reduce_dyadic(
  deweight::Formula *formula,
  int bits_per_var,
  deweight::RoundingStrategy rounding) {
  BigInt result = 1;
  std::vector<int> free_variables = formula->get_independent_support();
  // If there is no independent support, consider all variables
  if (free_variables.size() == 0) {
    for (int var = 1; var <= formula->num_variables(); var++) {
      free_variables.push_back(var);
    }
  }

  for (int var : free_variables) {
    deweight::Rational pos = formula->get_weight(var).simplify();
    deweight::Rational neg = formula->get_weight(-var).simplify();

    if (pos.denom != neg.denom || pos.num + neg.num != pos.denom) {
      std::cerr << "Skipping var " << var << " (non-probabilistic weights)";
      std::cerr << std::endl;
      continue;
    }

    if (pos.num < 0 || neg.num < 0) {
      std::cerr << "Skipping var " << var << " (negative weight)" << std::endl;
      continue;
    }

    if (pos.num == 1 && neg.num == 1 && pos.denom == 1) {
      result *= 2;
      continue;  // No need to modify unweighted variables.
    }

    // Round the weight to the nearest dyadic weight
    // (rounding the positive weight down)
    auto approx = pos.round(1 << bits_per_var, rounding).simplify();
    int bits_needed = 0;
    while ((1 << bits_needed) < approx.denom) {
      bits_needed++;
    }
    formula->add_comment(
      "adjust w " + std::to_string(var)
       + " " + deweight::to_string(pos)
       + " to " + deweight::to_string(approx));

    if (approx.num == 1 && approx.denom == 2) {
      // No need to include any variables for weights (1/2, 1/2)
      result *= 2;
      continue;
    }

    // Add clauses for chain formula
    std::vector<size_t> vars;
    for (int i = 0; i < bits_needed; i++) {
      vars.push_back(formula->add_variable());
      if (formula->has_independent_support()) {
        formula->add_independent_support(vars.back());
      }
    }

    // var -> [pos weight] solutions
    for (auto &clause : chain_formula(vars, approx.num)) {
      clause.insert(clause.begin(), -var);
      formula->add_clause(clause);
    }

    // -var -> [neg weight] solutions
    for (int i = 0; i < bits_needed; i++) {
      vars[i] = -vars[i];  // start counting from the lexicographic bottom
    }
    for (auto &clause : chain_formula(vars, approx.complement().num)) {
      clause.insert(clause.begin(), var);
      formula->add_clause(clause);
    }

    result *= approx.denom;
  }
  return result;
}

int main(int argc, char *argv[]) {
  cxxopts::Options options("deweight",
    "A tool to reduce discrete integration to unweighted model counting.");
  options.custom_help("[OPTION...] < [WEIGHTED CNF FILE]");
  options.add_options()
    ("d, dyadic", "Use dyadic reduction with "
    "[arg] bits per weight.", cxxopts::value<int>())
    ("r, rounding", "Rounding used to adjust weight of positive literal.\n"
     "Note that weights will never be adjusted to 0 or 1.\n"
     "--rounding=down\tRound down to next allowed weight\n"
     "--rounding=up\tRound up to next allowed weight\n"
     "--rounding=near\tRound to nearest allowed weight\n",
     cxxopts::value<deweight::RoundingStrategy>()->default_value("down"))
    ("w, weights", "Format of weights to parse from CNF.\n"
     "--weights=detect\tAutomatically detect weight format\n"
     "--weights=cachet\tParse cachet weights\n"
     "--weights=minic2d\tParse miniC2D weights\n"
     "--weights=mc20\tParse weights from MC 2020 competition\n",
     cxxopts::value<deweight::WeightFormat>()->default_value("detect"))
    ("h, help", "Print usage");
  auto args = options.parse(argc, argv);
  if (args.count("help")) {
    std::cout << options.help() << std::endl;
    exit(0);
  }

  auto start_time = std::chrono::steady_clock::now();
  StreamBuffer<FILE*, FN> in = StreamBuffer<FILE*, FN>(stdin);
  auto weight_format = args["weights"].as<deweight::WeightFormat>();
  auto formula = deweight::Formula(&in, weight_format);
  if (formula.num_variables() == 0) {
    std::cerr << "Error: Unable to read formula." << std::endl;
    return -1;
  }

  BigInt denom;
  if (args.count("dyadic") > 0) {
    int num_bits = args["dyadic"].as<int>();
    auto rounding = args["rounding"].as<deweight::RoundingStrategy>();
    denom = reduce_dyadic(&formula, num_bits, rounding);
  } else {
    denom = reduce(&formula);
  }

  std::cout << "c denom " << denom << std::endl;
  auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
    std::chrono::steady_clock::now() - start_time).count();
  std::cout << "c deweight time " << elapsed << "\n";
  formula.write(&std::cout);

  return 0;
}
