import argparse
import os
import tempfile
import subprocess
import sys


def run(args):
    current_dir = os.path.realpath(__file__)
    current_dir = current_dir[:current_dir.rindex('/')]

    with tempfile.TemporaryFile('w+') as unweighted_formula:
        # Call DeWeight with the provided arguments
        deweight_cmd = ["deweight/build/deweight"]
        for arg, val in args._get_kwargs():
            if arg not in ['approxmc', 'epsilon', 'delta'] and val != 0 and val != '':
                deweight_cmd += ["--" + arg + '=' + str(val) + '']
        log("[DeWeight] " + " ".join(deweight_cmd))
        subprocess.run(deweight_cmd, stdout=unweighted_formula, cwd=current_dir)

        # Record reduction information from DeWeight
        unweighted_formula.seek(0)
        lower_approx, upper_approx = 1, 1
        for line in unweighted_formula:
            if line.startswith('c denom'):
                output_pair("Normalization", line.split()[-1])
                normalize = float(line.split()[-1])
            elif line.startswith('c deweight time'):
                output_pair("Deweight Time", line.split()[-1])
            elif line.startswith('c adjust w'):
                real_weight = parse_rational(line.split()[-3])
                appx_weight = parse_rational(line.split()[-1])
                if appx_weight < real_weight:
                    # The positive literal is the under-approximation
                    upper_approx /= (appx_weight / real_weight)
                    lower_approx /= (1 - appx_weight) / (1 - real_weight)
                else:
                    # The negative literal is the under-approximation
                    lower_approx /= (appx_weight / real_weight)
                    upper_approx /= (1 - appx_weight) / (1 - real_weight)
                log(line, end='')
        output_pair("Weight adjustment", [lower_approx, upper_approx])
        log("")

        # Call ApproxMC with the unweighted formula
        unweighted_formula.seek(0)
        approxmc_cmd = [args.approxmc, "--sparse", "1"]
        if args.epsilon != 0.8:
            approxmc_cmd.extend(["--epsilon", str(args.epsilon)])
        if args.delta != 0.2:
            approxmc_cmd.extend(["--delta", str(args.epsilon)])
        log("[DeWeight] " + " ".join(approxmc_cmd))
        process = subprocess.Popen(
            approxmc_cmd,
            stdin=unweighted_formula,
            bufsize=1,  # Line-buffered
            universal_newlines=True,  # Required for line-buffered
            stdout=subprocess.PIPE,
            cwd=current_dir,
        )
        exact = False
        for line in process.stdout:
            log(line, end='')
            if line.startswith("[appmc] FINISHED AppMC T:"):
                output_pair("ApproxMC Time", line.split()[-2])
            elif "i.e. we got exact count" in line:
                exact = True
            elif line.startswith("[appmc] Number of solutions is:"):
                components = line.split()[-1].split("*")  # solutions are of the form "A*2**B"
                solutions = int(components[0]) * (2 ** int(components[-1]))
                output_pair("Solutions", solutions)
                probability = float(solutions) / float(normalize)
                output_pair("Probability", probability)
                if exact:
                    lower_bound = lower_approx * probability
                    upper_bound = upper_approx * probability
                else:
                    lower_bound = lower_approx * probability / (1 + args.epsilon)
                    upper_bound = upper_approx * probability * (1 + args.epsilon)
                output_pair("Probability Interval", [lower_bound, upper_bound])


def log(line, **kwargs):
    print(line, file=sys.stderr, **kwargs)
    sys.stderr.flush()


def parse_rational(rational):
    parts = rational.split("/")
    return float(parts[0]) / float(parts[1])


def output_pair(key, value):
    print(str(key) + ": " + str(value))
    sys.stdout.flush()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="A tool to reduce discrete integration to unweighted model counting.")
    parser.add_argument("--dyadic", type=int, help="Use dyadic reduction with [arg] bits per weight.", default=0)
    parser.add_argument("--rounding", type=str, help="Rounding used to adjust weight of positive literal.", default="")
    parser.add_argument("--weights", type=str, help="Format of weights to parse from CNF.", default="")
    parser.add_argument("--approxmc", type=str, help="Path to ApproxMC (Relative to script)", default="./approxmc")
    parser.add_argument("--epsilon", type=float, help="Epsilon for ApproxMC", default=0.8)
    parser.add_argument("--delta", type=float, help="Delta for ApproxMC", default=0.2)
    run(parser.parse_args())
