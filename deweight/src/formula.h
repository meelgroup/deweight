/******************************************
Copyright (c) 2020, Jeffrey Dudek
******************************************/

#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../lib/streambuffer.h"
#include "../lib/cxxopts.hpp"
#include "src/rational.h"

namespace deweight {
/**
 * Represent a choice of weight format
 */
enum WeightFormat {detect, cachet, cachet_or_mc20, minic2d, mc20};
std::istream& operator>> (std::istream& is, WeightFormat& rs);

/**
 * Represents a boolean formula in CNF with literal weights.
 */
class Formula {
 public:
  /*
  * Parses a file in DIMACS format into a boolean formula.
  *
  * Returns the parsed formula if the DIMACS file is in a valid format.
  */
  explicit Formula(StreamBuffer<FILE*, FN> *in, WeightFormat weights);

  Formula(const Formula& other) = default;
  Formula& operator=(const Formula& other) = default;

  void set_header(int num_variables, int num_clauses) {
    num_variables_ = num_variables;
    num_clauses_ = num_clauses;
  }

  /**
   * Adds a CNF clause to the formula containing the provided literals.
   */
  void add_clause(std::vector<int> literals);

  /**
   * Adds a comment at the bottom of the formula.
   */
  void add_comment(std::string comment);

  /**
   * Get the next free variable id.
   */
  size_t add_variable() {
    return ++num_variables_;
  }

  /**
   * Output the unweighted DIMACS of this formula.
   */
  void write(std::ostream *output) const;

  /**
   * Get the weight of a literal.
   */
  Rational get_weight(int literal) const;

  /**
   * Set the weight of a literal.
   */
  void set_weight(int literal, Rational weight);

  /**
   * Return true if the provided literal can be used in the formula.
   */
  bool is_valid_literal(int literal) {
    return (literal != 0 && abs(literal) <= num_variables_);
  }

  /**
   * Set the independent support.
   */
  void add_independent_support(size_t var) {
    independent_support_.push_back(var);
  }

  bool has_independent_support() {
    return independent_support_.size() > 0;
  }

  std::vector<int> get_independent_support() {
    return independent_support_;
  }

  int num_variables() const { return num_variables_; }

 private:
  // Number of variables in the formula
  size_t num_variables_ = 0;
  size_t num_clauses_ = 0;

  // List of clauses and comments
  std::string body_ = "";

  // Independent support
  std::vector<int> independent_support_;
  // Set of weights
  std::unordered_map<int, Rational> weights_;
};
}  // namespace deweight
