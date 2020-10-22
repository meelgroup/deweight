# DeWeight
A tool to reduce discrete integration to unweighted model counting.

## Installation

Requires gcc and make.
```
git clone https://github.com/Kasekopf/Deweight.git
cd Deweight
make -C deweight
```

## Usage
```
$ deweight/build/deweight --help
A tool to reduce discrete integration to unweighted model counting.
Usage:
  deweight [OPTION...] < [WEIGHTED CNF FILE]

  -d, --dyadic arg    Use dyadic reduction with [arg] bits per weight.
  -r, --rounding arg  Rounding used to adjust weight of positive literal.
                      Note that weights will never be adjusted to 0 or 1.
                      --rounding=down   Round down to next allowed weight
                      --rounding=up     Round up to next allowed weight
                      --rounding=near   Round to nearest allowed weight
                       (default: down)
  -w, --weights arg   Format of weights to parse from CNF.
                      --weights=detect  Automatically detect weight format
                      --weights=cachet  Parse cachet weights
                      --weights=minic2d Parse miniC2D weights
                      --weights=mc20    Parse weights from MC 2020 competition
                       (default: detect)
  -h, --help          Print usage
```

### New Reduction
DeWeight reads a weighted CNF file from STDIN and writes the reduced unweighted CNF to STDOUT.
```
$ deweight/build/deweight < demo.cnf
c denom 30
c deweight time 0.0036257
p cnf 6 5
-1 0
2 3 0
c detected weight format: cachet
1 4 0
-2 5 0
-2 6 0
```

The weight of the input weighted CNF `demo.cnf` is 0.2. The CNF output of DeWeight has 6 solutions and the normalizing factor is 30 (from `c denom 30`), corresponding to a weighted model count of 6 / 30.

Weights must be nonnegative, but need not be probabilisitic. This is, the weight of positive and negative literals need not add up to 1. Such weights can be easily specified using the miniC2D weight format.

### Dyadic Reduction
DeWeight also implements the old dyadic reduction, for which every weight must be adjusted to a nearby dyadic weight (p/2^d for integers p and d). For example, the following command rounds each weight up to the nearest multiple of 1/4 before performing the reduction:
```
$ deweight/build/deweight --dyadic=2 --rounding=up < demo.cnf
c denom 32
c deweight time 0.0038668
p cnf 7 8
-1 0
2 3 0
c detected weight format: cachet
c adjust w 1 2/3 to 3/4
-1 4 5 0
1 -4 0
1 -5 0
c adjust w 2 1/5 to 1/4
-2 6 0
-2 7 0
2 -6 -7 0
c adjust w 3 1/2 to 1/2
```

Note that the dyadic reduction assumes that weights are probabilistic, i.e. that the weights of positive and negative literals add up to 1.

# Wrapper with ApproxMC

We also provide a Python script that integrates DeWeight with the unweighted, approximate model counter [ApproxMC](https://github.com/meelgroup/approxmc). This wrapper runs both DeWeight and ApproxMC to produce an interval in which the answer to the discrete integration exists with probability `--delta` (default: 0.8). The resulting interval incorporates both error from ApproxMC and error from adjusting the weights (for the dyadic reduction, if required).

## Installation

Requires python and [ApproxMC](https://github.com/meelgroup/approxmc) to be installed.

## Usage
```
python deweight_wrapper.py --help
usage: deweight_wrapper.py [-h] [--dyadic DYADIC] [--rounding ROUNDING]
                           [--weights WEIGHTS] [--approxmc APPROXMC]
                           [--epsilon EPSILON] [--delta DELTA]

A tool to reduce discrete integration to unweighted model counting.

optional arguments:
  -h, --help           show this help message and exit
  --dyadic DYADIC      Use dyadic reduction with [arg] bits per weight.
  --rounding ROUNDING  Rounding used to adjust weight of positive literal.
  --weights WEIGHTS    Format of weights to parse from CNF.
  --approxmc APPROXMC  Path to ApproxMC (Relative to script)
  --epsilon EPSILON    Epsilon for ApproxMC
  --delta DELTA        Delta for ApproxMC
```

### New Reduction
The wrapper reads a weighted CNF file from STDIN and outputs the probability interval to STDOUT. Additional log information is sent to STDERR.
```
$ python deweight_wrapper.py --approxmc="../ApproxMC/approxmc" < demo.cnf 2> /dev/null
Normalization: 30
Deweight Time: 0.0038164
Weight adjustment: [1, 1]
ApproxMC Time: 0.00
Solutions: 6
Probability: 0.2
Probability Interval: [0.2, 0.2]
```

In this case, the probability interval is tight at 0.2 since ApproxMC produced an exact count. For larger benchmarks, this interval will not be tight if ApproxMC computes only an approximate answer; the quality of ApproxMC approximation can be configured with `--epsilon`.

### Dyadic Reduction
The wrapper can also be combined with the dyadic reduction.
```
$ python deweight_wrapper.py --dyadic=2 --rounding=up --approxmc="../ApproxMC/approxmc" < demo.cnf 2> /dev/null
Normalization: 32
Deweight Time: 0.0035617
Weight adjustment: [0.711111111111111, 1.4222222222222223]
ApproxMC Time: 0.00
Solutions: 5
Probability: 0.15625
Probability Interval: [0.1111111111111111, 0.22222222222222224]
```

In this case, the probability interval is not tight even though ApproxMC produced an exact count. This is because of the error introduced by adjusting the weights as required by the dyadic reduction introduces error.