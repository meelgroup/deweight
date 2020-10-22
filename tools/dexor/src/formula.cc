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
  Formula::Formula(StreamBuffer<FILE*, FN> *in) {
    std::string entry;
    int line_num = 0;
    std::vector<int> lits;
    std::vector<int> clause;
    int lit;

    for (;;) {
      in->skipWhitespace();
      switch (**in) {
        case EOF:
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
        case 'x':
          if (!in->consume("x ")) {
            set_header(0, 0);
            return;
          }
          lits.clear();
          for (;;) {
            in->parseInt(lit, line_num);
            if (lit == 0) {
              break;
            }
            lits.push_back(lit);
          }
          if (lits.size() > 8) {
            std::cerr << "Found long xor (" << lits.size() << ")" << std::endl;
          }
          /* Iterate through all CNF clauses and choose the false ones */
          for (unsigned int i = 0; i < (1 << lits.size()); i++) {
            if (__builtin_parity(i) == 1) {
              continue;
            }
            clause.clear();
            for (size_t j = 0; j < lits.size(); j++) {
              clause.push_back(lits[j]);
              if ((i & (1 << j)) != 0) {
                clause[j] *= -1;
              }
            }
            add_clause(clause);
          }
          num_clauses_--;
          break;
        default:
          in->appendLine(body_);
          body_.push_back('\n');
          break;
      }
      in->skipLine();
      line_num++;
    }
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

  void Formula::write(std::ostream *output) const {
    // Write header
    *output << "p cnf " << num_variables_ << " " << num_clauses_ << "\n";

    // Write clauses
    *output << body_;
  }
}  // namespace deweight
