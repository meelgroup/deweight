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
enum OutputFormat {cachet, ganak, sdimacs};
std::istream& operator>> (std::istream& is, OutputFormat& rs);

/** 
 * Represents a weight for a variable.
 */
class Rational {
 public:
  Rational(const Rational& other) = default;
  Rational& operator=(const Rational& other) = default;

  explicit Rational(int numerator, int denominator)
  : num(numerator), denom(denominator)
  { }

  Rational complement() const;

  void output(std::ostream *out, bool use_decimals) const;
  static Rational parse(std::string decimal);

  const int num;
  const int denom;
};

/**
 * Represents a boolean formula in CNF with literal weights.
 */
class Formula {
 public:
  Formula(const Formula& other) = default;
  Formula& operator=(const Formula& other) = default;

  Formula() {}

  void set_header(int num_variables, int num_clauses) {
    num_variables_ = num_variables;
    num_clauses_ = num_clauses;
  }

  /**
   * Output the DIMACS of this formula, with weights as fractions or decimals.
   */
  void write(std::ostream *out, bool use_decimals, OutputFormat format) const;

  /**
   * Get the weight of a literal.
   */
  Rational get_weight(int literal) const;

  /**
   * Set the weight of a literal.
   */
  bool set_weight(int literal, Rational weight);

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

  /*
  * Parses a file in DIMACS format into a boolean formula.
  *
  * Returns the parsed formula if the DIMACS file is in a valid format.
  */
  static Formula parse_DIMACS(StreamBuffer<FILE*, FN> &in);

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
