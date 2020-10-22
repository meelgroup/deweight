/******************************************
Copyright (c) 2020, Jeffrey Dudek
******************************************/

#include <string.h>
#include <iostream>
#include <algorithm>

#include "src/formula.h"


int main(int argc, char *argv[]) {
  StreamBuffer<FILE*, FN> in = StreamBuffer<FILE*, FN>(stdin);
  deweight::Formula formula(&in);
  if (formula.num_variables() > 0) {
    formula.write(&std::cout);
  } else {
    std::cerr << "Error: Unable to read formula." << std::endl;
    return -1;
  }
  return 0;
}
