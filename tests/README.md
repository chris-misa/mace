# Tests

This directory includes the scripts (bash and R) used to execute and process experiments for evaluation of MACE.

The main entry point is `run_bench.sh` which calls other scripts in this directory to execute experiment for a sequence of traffic settings.

All of these scripts assume MACE has already been compiled (by typing `make` in `../`)

# Notes

There is some potential confusion here since we moved from using iperf to flood ping to generate traffic.
For example, run_bench.sh actually uses flood ping (via add_container_ping_remote.sh) NOT iperf to generate traffic.
