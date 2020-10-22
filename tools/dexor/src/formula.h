/******************************************
Copyright (c) 2020, Jeffrey Dudek
******************************************/

#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../../lib/streambuffer.h"

namespace deweight {
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
  explicit Formula(StreamBuffer<FILE*, FN> *in);

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
   * Output the DIMACS of this formula, either with or without the weights.
   */
  void write(std::ostream *output) const;

  int num_variables() const { return num_variables_; }

 private:
  // Number of variables in the formula
  size_t num_variables_ = 0;
  size_t num_clauses_ = 0;

  // List of clauses and comments
  std::string body_ = "";
};
}  // namespace deweight
