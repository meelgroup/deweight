/******************************************
Copyright (c) 2020, Jeffrey Dudek
******************************************/

#include "src/formula.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <sstream>

#include <../../lib/cxxopts.hpp>

namespace deweight {
  std::istream& operator>> (std::istream& is, OutputFormat& rs) {
    std::string val;
    is >> val;
    std::transform(val.begin(), val.end(), val.begin(), tolower);
    if (val == "cachet") {
      rs = OutputFormat::cachet;
    } else if (val == "ganak") {
      rs = OutputFormat::ganak;
    } else if (val == "sdimacs") {
      rs = OutputFormat::sdimacs;
    } else {
      throw cxxopts::argument_incorrect_type("output");
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

  void Rational::output(std::ostream *out, bool use_decimals) const {
    if (use_decimals) {
      *out << (static_cast<double>(num) / static_cast<double>(denom));
    } else {
      *out << num << "/" << denom;
    }
  }

  Rational Rational::complement() const {
    return Rational(denom - num, denom);
  }

  Formula Formula::parse_DIMACS(StreamBuffer<FILE*, FN> &in) {
    Formula result;
    std::string entry;
    int line_num = 0;
    for (;;) {
      in.skipWhitespace();
      switch (*in) {
        case EOF:
          return result;
        case 'p':
          if (!in.consume("p ")) {
            result.set_header(0, 0);
            return result;
          }
          in.parseString(entry);
          if (entry != "cnf" && entry != "pcnf") {
            result.set_header(0, 0);
            return result;
          }
          int num_variables, num_clauses;
          in.parseInt(num_variables, line_num);
          in.parseInt(num_clauses, line_num);
          result.set_header(num_variables, num_clauses);
          break;
        case 'w':
          if (!in.consume("w ")) {
            result.set_header(0, 0);
            return result;
          }
          int literal;
          in.parseInt(literal, line_num);
          in.parseString(entry);
          if (entry == "-1") {
            // A weight of -1 indicates the same weight for x and -x
            if (!result.set_weight(literal, Rational(1, 1))
                || !result.set_weight(-literal, Rational(1, 1))) {
              result.set_header(0, 0);
              return result;
            }
          } else {
            Rational literal_weight = Rational::parse(entry);
            if (!result.set_weight(literal, literal_weight)
                || !result.set_weight(-literal, literal_weight.complement())) {
              result.set_header(0, 0);
              return result;
            }
          }
          break;
        case 'v':
          if (!in.consume("vp ")) {
            result.set_header(0, 0);
            return result;
          }
          int variable;
          in.parseInt(variable, line_num);
          while (variable != 0) {
            result.add_independent_support(variable);
            in.parseInt(variable, line_num);
          }
          break;
        case 'c':
          if (!in.consume("c")) {
            result.set_header(0, 0);
            return result;
          }
          if (!in.consume(" ")) {
            // Handle lines of 'c\n'
            result.body_.push_back('c');
            result.body_.push_back('\n');
            break;
          }
          in.parseString(entry);
          if (entry == "ind") {
            int variable;
            in.parseInt(variable, line_num);
            while (variable != 0) {
              result.add_independent_support(variable);
              in.parseInt(variable, line_num);
            }
          } else {
            // First word of comment was consumed; re-add it
            result.body_.push_back('c');
            result.body_.push_back(' ');
            result.body_.append(entry);
            result.body_.push_back(' ');
            in.appendLine(result.body_);
            result.body_.push_back('\n');
          }
          break;
        default:
          in.appendLine(result.body_);
          result.body_.push_back('\n');
          break;
      }
      in.skipLine();
      line_num++;
    }

    result.set_header(0, 0);
    return result;
  }

  bool Formula::set_weight(int literal, Rational weight) {
    if (literal == 0 || abs(literal) > num_variables_) {
      return false;
    }
    weights_.emplace(literal, weight);
    return true;
  }

  Rational Formula::get_weight(int literal) const {
    auto elem = weights_.find(literal);
    if (elem == weights_.end()) {
      return Rational(1, 1);
    } else {
      return elem->second;
    }
  }

  void Formula::write(std::ostream *out, bool use_decimals, OutputFormat format) const {
    // Write header
    *out << "p cnf " << num_variables_ << " " << num_clauses_ << "\n";

    switch (format) {
      case OutputFormat::cachet:
        for (int variable = 1; variable <= num_variables_; variable++) {
          Rational w = get_weight(variable);
          if (w.num != 1 || w.denom != 1) {
            *out << "w " << variable << " ";
            w.output(out, use_decimals);
            *out << "\n";
          }
        }
        break;
      case OutputFormat::ganak:
        for (int variable = 1; variable <= num_variables_; variable++) {
          Rational w = get_weight(variable);
          if (w.num != 1 || w.denom != 1) {
            *out << "w " << variable << " ";
            w.output(out, use_decimals);
            *out << " 0 \n";
            *out << "w -" << variable << " ";
            w.complement().output(out, use_decimals);
            *out << " 0 \n";
          }
        }
        break;
      case OutputFormat::sdimacs:
        std::cout << std::setprecision(4) << std::fixed;
        for (int variable : independent_support_) {
          Rational w = get_weight(variable);
          *out << "r ";
          w.output(out, use_decimals);
          *out << " " << variable << " 0\n";
        }
        std::vector<int> random_vars = independent_support_;
        std::sort(random_vars.begin(), random_vars.end());
        for (int variable = 1; variable <= num_variables_; variable++) {
          if (!std::binary_search(random_vars.begin(), random_vars.end(), variable)) {
            *out << "e " << variable << " 0\n";
          }
        }
        break;
    }

    // Write independent support
    if (independent_support_.size() > 0 && format != OutputFormat::sdimacs) {
      *out << "c ind";
      for (int variable : independent_support_) {
        *out << " " << variable;
      }
      *out << " 0\n";
    }

    // Write clauses
    *out << body_;
  }
}  // namespace deweight
