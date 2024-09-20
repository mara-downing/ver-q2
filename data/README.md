# Guide to Raw Data Files

## Paired datafiles

Some data files come in pairs, *\<name\>*\_results.txt and output\_*\<name\>*.txt. The results file contains a comma separated table showing a set of important features of the test and the time taken by that test in ms. The output file contains the command-line output of each test, with the first line (./main etc.) showing the command that was run, and the following lines showing the output of that command. Each ./main line can be mapped to one line in the timing file.

**p_smallr:**
    These tests relate to the 11 input perturbation tests on the Parkinson's network set. Columns are network, constraint #, # inputs perturbed, perturbation radius, model counter for leaves (always z3), and time (ms)

**mccomp:**
    These tests are for the comparison with ABC counting at the leaves. Columns are network, constraint #, # inputs perturbed (always 2px), perturbation radius (always rallnorm), model counter for leaves (always abc), and time (ms). 

**bruteforce:**
    These tests are to estimate the time taken per sample plus an initial startup time to complete Table 3.


## Standalone datafiles: 

**i_strategycomp_results.txt:**
    These tests are for comparing constraint solving optimization strategies for the iris networks. Columns are network, constraint #, # inputs perturbed, perturbation radius, model counter for leaves (always z3), strategy being tested (1 for Base, 2 for Abstract Only, 3 for Model Generation Only), and time (ms). The all strategy data for this comparison is taken from the mccomp files.

**p_strategycomp_results.txt:**
    These tests are for comparing constraint solving optimization strategies for the parkinsons networks. Columns are network, constraint #, # inputs perturbed, perturbation radius, model counter for leaves (always z3), strategy being tested (1 for Base, 6 for all optimizations), and time (ms).

**c_strategycomp_results.txt:**
    These tests are for comparing constraint solving optimization strategies for the wisconsin breast cancer networks. Columns are network, constraint #, # inputs perturbed, perturbation radius, model counter for leaves (always z3), strategy being tested (1 for Base, 6 for all optimizations), and time (ms).

**quickmccomp_results.txt:**
    These tests are for comparing domain counting times before symbolic execution. The first column is the full path to the constraint file, the second column is the ABC solving time, and the third column is the Z3 solving time.

**output_bruteforce_sample_smallr.txt:**
    These tests are using a sampling approach given the same time limit as the tool took to run each of parkinsons networks radius 0.2 11 input tests.

**output_p_provero.txt:**
    These tests are using our Provero implementation; at the end of each test the lower bound, upper bound, and time (s) are reported.


## Network accuracies:
| Network Size | Network Size | Dataset | Dataset | Dataset |
|------------------------------------|-------------------------------|--------|-------------|-------|
| # HL                               | HL Size                       | Iris   | Parkinson's | Cancer |
| 1                                  | 20                            | 73.33  | 89.47       | 96.43  |
| 1                                  | 30                            | 73.33  | 89.47       | 92.86  |
| 1                                  | 40                            | 60.00  | 94.74       | 98.21  |
| 1                                  | 50                            | 73.33  | 78.95       | 96.43  |
| 1                                  | 60                            | 93.33  | 84.21       | 91.07  |
| 1                                  | 70                            | 100.00 | 84.21       | 100.00 |
| 2                                  | 10                            | 93.33  | 84.21       | 92.86  |
| 2                                  | 15                            | 93.33  | 89.47       | 89.29  |
| 2                                  | 20                            | 100.00 | 89.47       | 96.43  |
| 2                                  | 25                            | 100.00 | 94.74       | 98.21  |
| 2                                  | 30                            | 100.00 | 84.21       | 96.43  |
| 2                                  | 35                            | 100.00 | 89.47       | 96.43  |
| 3                                  | 5                             | 100.00 | 89.47       | 98.21  |
| 3                                  | 10                            | 93.33  | 84.21       | 96.43  |
| 3                                  | 15                            | 100.00 | 84.21       | 98.21  |
| 3                                  | 20                            | 100.00 | 78.95       | 98.21  |

