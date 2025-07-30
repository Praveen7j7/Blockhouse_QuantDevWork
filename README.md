MBO to MBP-10 Order Book Reconstruction
========================================

OVERVIEW
--------
This C++ application reconstructs MBP-10 (Market By Price, 10 levels) order book snapshots 
from MBO (Market By Order) data using Databento format. It processes various order book 
actions and maintains real-time snapshots of the top 10 bid and ask price levels, outputting 
in the standard MBP space-separated format.

COMPILATION & EXECUTION
-----------------------
1. Compile using g++:
   g++ -O3 -Wall -Wextra -o reconstruction_mbo_mbp main.cpp

2. Run the executable with MBO CSV input:
   ./reconstruction_mbo_mbp mbo.csv

3. Output is automatically saved to mbp.csv in the correct MBP format

To run the code use the below commands:
------------------------------------
```bash
g++ -O3 -Wall -Wextra -o reconstruction_mbo_mbp main.cpp
./reconstruction_mbo_mbp mbo.csv
```

INPUT FORMAT (MBO - Databento Format)
------------------------------------
The program expects space-separated MBO data with these columns:
- ts_recv, ts_event, rtype, publisher_id, instrument_id, action, side, price, size, 
  channel_id, order_id, flags, ts_in_delta, sequence, symbol

Key columns used for processing:
- ts_event (column 1): Timestamp for the event
- action (column 5): Order action (A=Add, C=Cancel, M=Modify, T=Trade, F=Fill, R=Reset)
- side (column 6): Order side (B=Bid, A=Ask, N=None)
- price (column 7): Price level
- size (column 8): Order size
- order_id (column 10): Unique order identifier
- sequence (column 13): Sequence number

OUTPUT FORMAT (MBP-10)
----------------------
Space-separated MBP format with columns:
- Fixed fields: ts_recv, ts_event, rtype, publisher_id, instrument_id, action, side, 
  depth, price, size, flags, ts_in_delta, sequence
- 10 bid levels: bid_px_00 through bid_px_09, bid_sz_00 through bid_sz_09, 
  bid_ct_00 through bid_ct_09
- 10 ask levels: ask_px_00 through ask_px_09, ask_sz_00 through ask_sz_09,
  ask_ct_00 through ask_ct_09  
- Trailing fields: symbol, order_id

OPTIMIZATION STRATEGIES
-----------------------

1. DATA STRUCTURES:
   - Used std::map with custom comparators for automatic price-level sorting
   - Bids: map<double, int, greater<double>> (descending price order)
   - Asks: map<double, int> (ascending price order)
   - This eliminates need for manual sorting when extracting top 10 levels

2. MEMORY EFFICIENCY:
   - Aggregate size at each price level instead of tracking individual orders
   - Remove price levels when size reaches zero to prevent memory bloat
   - Use efficient C++ containers (map provides O(log n) operations)
   - unordered_map for tracking pending trades (O(1) average lookup)

3. I/O OPTIMIZATION:
   - Fast I/O with ios_base::sync_with_stdio(false)
   - Direct file output to mbp.csv without intermediate buffering
   - Single-pass processing of input data
   - Efficient string parsing using vector field splitting

4. ALGORITHMIC EFFICIENCY:
   - Special handling for T->F->C sequences using order ID tracking
   - Minimal string operations using C-style conversion functions (atoi, atof)
   - Efficient price level extraction (top 10 only, early termination)

SPECIAL PROCESSING RULES IMPLEMENTED
------------------------------------

1. INITIAL CLEAR ACTION:
   - First row and any row with action 'R' (reset/clear) is ignored
   - Order book starts empty as specified

2. TRADE SEQUENCE HANDLING (T->F->C):
   - Trade actions ('T') are tracked with their order IDs in pending_trades map
   - When corresponding Cancel ('C') action is processed, the trade is applied 
     to the OPPOSITE side of where it originally appeared
   - This reflects that trades consume liquidity from the existing book
   - Fill actions ('F') are tracked but don't modify the book directly
   - The MBP snapshot uses the original trade timestamp and sequence

3. NULL SIDE TRADES:
   - Trade actions with side 'N' are ignored and don't affect the book

4. ORDER BOOK MAINTENANCE:
   - Add orders ('A'): Aggregate size at price levels
   - Cancel orders ('C'): Reduce size, remove level if size <= 0
   - Modify orders ('M'): Simplified as size replacement at price level

COMPATIBILITY FEATURES
-----------------------
- Uses C-style string conversion functions (atoi, atof, strtoul) for maximum compiler compatibility
- Compatible with older C++ compilers (no C++11 requirement)
- Uses bits/stdc++.h and "using namespace std" for competitive programming style
- Robust CSV parsing handles variable field counts

PERFORMANCE CHARACTERISTICS
---------------------------
- Time Complexity: O(n log k) where n = number of MBO records, k = average price levels
- Space Complexity: O(k + t) where k = active price levels, t = pending trades
- Memory usage scales with order book depth, not total order count
- Optimized for high-frequency data processing

OUTPUT DETAILS
--------------
- Empty price levels are filled with "0" for price, size, and count
- Bid/Ask counts (bid_ct_XX, ask_ct_XX) are set to 1 if level exists, 0 if empty
- Symbol is hardcoded as "ARL" and trailing order_id as "0" to match format
- Timestamps are preserved from the original MBO events
- Fixed fields use standard values: rtype=10, publisher_id=2, instrument_id=1108

TESTING
-------
The program processes real Databento MBO format and outputs standard MBP-10 format.
Compare output mbp.csv with expected results to verify correctness.

COMPILER REQUIREMENTS
--------------------
- Any C++ compiler supporting basic STL (g++ 4.x+ or clang++ 3.x+)
- No C++11 features required for maximum compatibility
- Tested with g++ using -O3 optimization flags

TROUBLESHOOTING
---------------
- Ensure input file has proper Databento MBO format with space-separated values
- Verify CSV parsing matches the expected 15+ column structure
- Check that file permissions allow reading input and writing output
- For large files, ensure sufficient disk space for mbp.csv output
- If compilation fails, try without optimization flags: g++ -o reconstruction_mbo_mbp main.cpp

FILE STRUCTURE
--------------
- main.cpp: Core implementation with OrderBook and MBOProcessor classes
- Makefile: Build system with multiple optimization levels
- README.txt: This documentation file
- mbp.csv: Generated output file (created after running program)
