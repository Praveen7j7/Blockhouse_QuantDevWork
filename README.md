# MBO to MBP-10 Order Book Reconstruction

## OVERVIEW

This C++ application reconstructs **MBP-10 (Market By Price, 10 levels)** order book snapshots from **MBO (Market By Order)** data using the Databento format. It processes various order book actions and maintains real-time snapshots of the top 10 bid and ask price levels, outputting in the standard MBP space-separated format.

---

## COMPILATION & EXECUTION

### Build

```bash
g++ -O3 -Wall -Wextra -o reconstruction_mbo_mbp MainLogic.cpp
```

### Run

```bash
./reconstruction_mbo_mbp.exe mbo.csv
```

### Output

Output will be stored in the file `mbp_output.csv`.
