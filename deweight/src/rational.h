/******************************************
Copyright (c) 2020, Jeffrey Dudek
******************************************/

#pragma once

#include <iostream>
#include <string>

namespace deweight {
enum RoundingStrategy {up, down, near};
std::istream& operator>> (std::istream& is, RoundingStrategy& rs);

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
  Rational simplify() const;
  double value() const {
    return static_cast<double>(num) / static_cast<double>(denom);
  }

  Rational round(int new_denom, RoundingStrategy strategy) const;

  static Rational parse(std::string decimal);

  const int num;
  const int denom;
};

inline std::string to_string(const Rational& r) {
    return std::to_string(r.num) + "/" + std::to_string(r.denom);
}
}  // namespace deweight
