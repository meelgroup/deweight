/******************************************
Copyright (c) 2020, Jeffrey Dudek
******************************************/

#include "src/rational.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include <../lib/cxxopts.hpp>

namespace deweight {
  std::istream& operator>> (std::istream& is, RoundingStrategy& rs) {
    std::string val;
    is >> val;
    std::transform(val.begin(), val.end(), val.begin(), tolower);
    if (val == "up") {
      rs = RoundingStrategy::up;
    } else if (val == "down") {
      rs = RoundingStrategy::down;
    } else if (val == "near") {
      rs = RoundingStrategy::near;
    } else {
      throw cxxopts::argument_incorrect_type("rounding");
    }
    return is;
  }

  Rational Rational::parse(std::string rational) {
    if (rational.find('/') == std::string::npos) {
      int numerator = 0;
      int denominator = 1;
      bool seen_decimal = false;

      for (char const &c : rational) {
        if (c == '.' && !seen_decimal) {
          seen_decimal = true;
        } else if (c >= '0' && c <= '9') {
          numerator = (numerator * 10) + (c - '0');
          if (seen_decimal) {
            denominator *= 10;
          }
        } else {
          std::cerr << "Unknown decimal: " << rational << "\n";
          return Rational(1, 0);
        }
      }
      return Rational(numerator, denominator);
    } else {
      int numerator = 0;
      int denominator = 0;
      bool seen_slash = false;
      for (char const &c : rational) {
        if (c == '/' && !seen_slash) {
          seen_slash = true;
        } else if (c >= '0' && c <= '9') {
          if (seen_slash) {
            denominator = (denominator * 10) + (c - '0');
          } else {
            numerator = (numerator * 10) + (c - '0');
          }
        } else {
          std::cerr << "Unknown fraction: " << rational << "\n";
          return Rational(1, 0);
        }
      }
      return Rational(numerator, denominator);
    }
  }

  Rational Rational::round(int new_denom, RoundingStrategy strategy) const {
    int new_num;
    switch (strategy) {
      case up:
        new_num = (num * new_denom + denom - 1) / denom;
        if (new_num == new_denom) {
            new_num--;
        }
        return Rational(new_num, new_denom);
      case down:
        new_num = (num * new_denom) / denom;
        if (new_num == 0) {
            new_num++;
        }
        return Rational(new_num, new_denom);
      case near:
        Rational ub = round(new_denom, RoundingStrategy::up);
        Rational lb = round(new_denom, RoundingStrategy::down);
        if (std::abs(lb.value() - value()) < std::abs(ub.value() - value())) {
          return lb;
        } else {
          return ub;
        }
    }
  }

  Rational Rational::complement() const {
    return Rational(denom - num, denom);
  }

  Rational Rational::simplify() const {
    int gcd = std::__gcd(num, denom);
    return Rational(num / gcd, denom / gcd);
  }
}  // namespace deweight
