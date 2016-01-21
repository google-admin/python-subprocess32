"""When ran as a script, simulates cat."""

import sys

if __name__ == "__main__":
    if len(sys.argv) == 1:
        files = [sys.stdin]
    else:
        files = (sys.stdin if f == '-' else open(f) for f in sys.argv[1:])
    for infile in files:
        with infile:
            for line in infile:
                sys.stdout.write(line)
