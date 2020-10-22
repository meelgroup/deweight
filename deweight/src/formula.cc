/******************************************
Copyright (c) 2020, Jeffrey Dudek
******************************************/

#include "src/formula.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <sstream>


namespace deweight {
  std::istream& operator>> (std::istream& is, WeightFormat& wf) {
    std::string val;
    is >> val;
    std::transform(val.begin(), val.end(), val.begin(), tolower);
    if (val == "detect") {
      wf = WeightFormat::detect;
    } else if (val == "cachet") {
      wf = WeightFormat::cachet;
    } else if (val == "minic2d") {
      wf = WeightFormat::minic2d;
    } else if (val == "mc20") {
      wf = WeightFormat::mc20;
    } else {
      throw cxxopts::argument_incorrect_type("weights");
    }
    return is;
  }

  void Formula::add_clause(std::vector<int> literals) {
    for (int literal : literals) {
      body_.append(std::to_string(literal));
      body_.push_back(' ');
    }
    body_.push_back('0');
    body_.push_back('\n');
    num_clauses_ += 1;
  }

  void Formula::add_comment(std::string comment) {
    body_.push_back('c');
    body_.push_back(' ');
    body_.append(comment);
    body_.push_back('\n');
  }

  Formula::Formula(StreamBuffer<FILE*, FN> *in, WeightFormat weights) {
    std::string entry;
    int line_num = 0;
    for (;;) {
      in->skipWhitespace();
      switch (**in) {
        case EOF:
          switch (weights) {
            case WeightFormat::detect:
            case WeightFormat::minic2d:
            case WeightFormat::mc20:
              break;
            case WeightFormat::cachet_or_mc20:
              // If only positive literals were assigned, assume cachet
              add_comment("detected weight format: cachet");
              weights = WeightFormat::cachet;
              // (Fallthrough)
            case WeightFormat::cachet:
              for (int i = 1; i <= num_variables_; i++) {
                auto weight = weights_.find(i);
                if (weight != weights_.end()) {
                  if (weights_.find(-i) == weights_.end()) {
                    set_weight(-i, weight->second.complement());
                  }
                } else {
                  set_weight(i, Rational(1, 2));
                  set_weight(-i, Rational(1, 2));
                }
              }
              break;
          }
          return;
        case 'p':
          if (!in->consume("p cnf ")) {
            set_header(0, 0);
            return;
          }
          int num_variables, num_clauses;
          in->parseInt(num_variables, line_num);
          in->parseInt(num_clauses, line_num);
          set_header(num_variables, num_clauses);
          break;
        case 'w':
          if (!in->consume("w ")) {
            set_header(0, 0);
            return;
          }

          switch (weights) {
            case WeightFormat::minic2d:
              set_header(0, 0);
              return;
            case WeightFormat::detect:
              // cachet vs mc20 is ambiguous until a negative literal is seen
              weights = WeightFormat::cachet_or_mc20;
              break;
            case WeightFormat::mc20:
            case WeightFormat::cachet_or_mc20:
            case WeightFormat::cachet:
              break;
          }

          int literal;
          in->parseInt(literal, line_num);
          if (!is_valid_literal(literal)) {
            set_header(0, 0);
            return;
          }

          if (literal < 0) {
            if (weights == WeightFormat::cachet) {
              set_header(0, 0);
              return;
            } else if (weights == WeightFormat::cachet_or_mc20) {
              // Once a negative literal occurs, we must be in the mc20 format
              add_comment("detected weight format: mc20");
              weights = WeightFormat::mc20;
            }
          }

          in->parseString(entry);
          if (entry == "-1") {
            switch (weights) {
              case WeightFormat::cachet_or_mc20:
                add_comment("detected weight format: cachet");
                weights = WeightFormat::cachet;
                // (Fallthrough)
              case WeightFormat::cachet:
                // A weight of -1 indicates the same weight for x and -x
                set_weight(literal, Rational(1, 1));
                set_weight(-literal, Rational(1, 1));
                break;
              default:
                // But weights must be positive in mc20
                set_header(0, 0);
                return;
            }
          } else {
            set_weight(literal, Rational::parse(entry));
          }
          break;
        case 'c':
          if (!in->consume("c")) {
            set_header(0, 0);
            return;
          }
          if (!in->consume(" ")) {
            // Handle lines of 'c\n'
            body_.push_back('c');
            body_.push_back('\n');
            break;
          }
          in->parseString(entry);
          if (entry == "ind") {
            int variable;
            in->parseInt(variable, line_num);
            while (variable != 0) {
              add_independent_support(variable);
              in->parseInt(variable, line_num);
            }
          } else if (entry == "weights" && (weights == WeightFormat::detect ||
                                            weights == WeightFormat::minic2d)) {
            if (weights == WeightFormat::detect) {
              add_comment("detected weight format: minic2d");
              weights = WeightFormat::minic2d;
            }
            for (int i = 1; i <= num_variables_; i++) {
              in->parseString(entry);
              set_weight(i, Rational::parse(entry));
              in->parseString(entry);
              set_weight(-i, Rational::parse(entry));
            }
          } else {
            // First word of comment was consumed; re-add it
            body_.push_back('c');
            body_.push_back(' ');
            body_.append(entry);
            body_.push_back(' ');
            in->appendLine(body_);
            body_.push_back('\n');
          }
          break;
        default:
          in->appendLine(body_);
          body_.push_back('\n');
          break;
      }
      in->skipLine();
      line_num++;
    }

    set_header(0, 0);
  }

  void Formula::set_weight(int literal, Rational weight) {
    weights_.emplace(literal, weight);
  }

  Rational Formula::get_weight(int literal) const {
    auto elem = weights_.find(literal);
    if (elem == weights_.end()) {
      return Rational(1, 1);
    } else {
      return elem->second;
    }
  }

  void Formula::write(std::ostream *output) const {
    // Write header
    *output << "p cnf " << num_variables_ << " " << num_clauses_ << "\n";

    // Write independent support
    if (independent_support_.size() > 0) {
      *output << "c ind";
      for (int variable : independent_support_) {
        *output << " " << variable;
      }
      *output << " 0\n";
    }

    // Write clauses
    *output << body_;
  }
}  // namespace deweight
