#include <bits/stdc++.h>
using namespace std;

struct MBORecord {
    string timestamp;
    string action;
    string side;
    double price;
    int size;
    uint64_t order_id;
    int sequence;
};

struct PriceLevel {
    double price;
    int size;
    
    PriceLevel() : price(0.0), size(0) {}
    PriceLevel(double p, int s) : price(p), size(s) {}
};

class OrderBook {
private:
    // Use maps for automatic sorting: bid descending, ask ascending
    map<double, int, greater<double>> bids;  // price -> total_size (descending)
    map<double, int> asks;                   // price -> total_size (ascending)
    
public:
    void addOrder(char side, double price, int size) {
        if (side == 'B') {
            bids[price] += size;
        } else if (side == 'A') {
            asks[price] += size;
        }
    }
    
    void cancelOrder(char side, double price, int size) {
        if (side == 'B') {
            auto it = bids.find(price);
            if (it != bids.end()) {
                it->second -= size;
                if (it->second <= 0) {
                    bids.erase(it);
                }
            }
        } else if (side == 'A') {
            auto it = asks.find(price);
            if (it != asks.end()) {
                it->second -= size;
                if (it->second <= 0) {
                    asks.erase(it);
                }
            }
        }
    }
    
    void modifyOrder(char side, double price, int old_size, int new_size) {
        cancelOrder(side, price, old_size);
        if (new_size > 0) {
            addOrder(side, price, new_size);
        }
    }
    
    vector<PriceLevel> getBids(int levels = 10) const {
        vector<PriceLevel> result;
        int count = 0;
        for (const auto& pair : bids) {
            if (count >= levels) break;
            result.emplace_back(pair.first, pair.second);
            count++;
        }
        return result;
    }
    
    vector<PriceLevel> getAsks(int levels = 10) const {
        vector<PriceLevel> result;
        int count = 0;
        for (const auto& pair : asks) {
            if (count >= levels) break;
            result.emplace_back(pair.first, pair.second);
            count++;
        }
        return result;
    }
};

class MBOProcessor {
private:
    OrderBook orderbook;
    vector<string> output_lines;
    
    // Track pending trade actions for T->F->C sequence processing
    struct PendingTrade {
        string timestamp;
        char original_side;
        double price;
        int size;
        int sequence;
        bool has_fill = false;
        bool has_cancel = false;
    };
    
    unordered_map<uint64_t, PendingTrade> pending_trades;
    
public:
    void processFile(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            throw runtime_error("Cannot open file: " + filename);
        }
        
        string line;
        bool first_line = true;
        
        while (getline(file, line)) {
            if (first_line) {
                first_line = false;
                continue; // Skip header
            }
            
            MBORecord record = parseLine(line);
            
            // Skip initial clear action as instructed
            if (record.action == "R") {
                continue;
            }
            
            processRecord(record);
        }
        
        file.close();
    }
    
private:
    MBORecord parseLine(const string& line) {
        MBORecord record;
        stringstream ss(line);
        string item;
        vector<string> fields;
        
        // Split the entire line by commas
        while (getline(ss, item, ',')) {
            fields.push_back(item);
        }
        
        // Parse according to your CSV format:
        // 0: ts_recv, 1: ts_event, 2: rtype, 3: publisher_id, 4: instrument_id, 
        // 5: action, 6: side, 7: price, 8: size, 9: channel_id, 10: order_id, 
        // 11: flags, 12: ts_in_delta, 13: sequence, 14: symbol
        
        if (fields.size() >= 15) {
            record.timestamp = fields[1]; // ts_event
            record.action = fields[5];    // action
            record.side = fields[6];      // side
            record.price = atof(fields[7].c_str());     // price
            record.size = atoi(fields[8].c_str());      // size
            record.order_id = (uint64_t)strtoul(fields[10].c_str(), nullptr, 10); // order_id
            record.sequence = atoi(fields[13].c_str()); // sequence
        }
        
        return record;
    }
    
    void processRecord(const MBORecord& record) {
        char action = record.action[0];
        char side = record.side[0];
        
        switch (action) {
            case 'A': // Add order
                orderbook.addOrder(side, record.price, record.size);
                outputSnapshot(record.timestamp, record.sequence);
                break;
                
            case 'C': // Cancel order
                // Check if this is part of a T->F->C sequence
                if (pending_trades.find(record.order_id) != pending_trades.end()) {
                    auto& trade = pending_trades[record.order_id];
                    trade.has_cancel = true;
                    
                    // Process the trade on the opposite side (where the liquidity was consumed)
                    char opposite_side = (trade.original_side == 'A') ? 'B' : 'A';
                    orderbook.cancelOrder(opposite_side, record.price, record.size);
                    outputSnapshot(trade.timestamp, trade.sequence);
                    
                    pending_trades.erase(record.order_id);
                } else {
                    // Regular cancel
                    orderbook.cancelOrder(side, record.price, record.size);
                    outputSnapshot(record.timestamp, record.sequence);
                }
                break;
                
            case 'M': // Modify order
                // For modify, we need to handle size changes
                orderbook.modifyOrder(side, record.price, 0, record.size); // Simplified
                outputSnapshot(record.timestamp, record.sequence);
                break;
                
            case 'T': // Trade
                if (side != 'N') {
                    // Start tracking this trade for T->F->C sequence
                    PendingTrade trade;
                    trade.timestamp = record.timestamp;
                    trade.original_side = side;
                    trade.price = record.price;
                    trade.size = record.size;
                    trade.sequence = record.sequence;
                    pending_trades[record.order_id] = trade;
                }
                break;
                
            case 'F': // Fill
                // Mark that we've seen the fill part of T->F->C sequence
                if (pending_trades.find(record.order_id) != pending_trades.end()) {
                    pending_trades[record.order_id].has_fill = true;
                }
                break;
                
            default:
                // Handle other actions if needed
                break;
        }
    }
    
    void outputSnapshot(const string& timestamp, int sequence) {
        stringstream ss;
        
        auto bids = orderbook.getBids(10);
        auto asks = orderbook.getAsks(10);
        
        // Format: timestamp,sequence,bid_px_XX,bid_sz_XX,ask_px_XX,ask_sz_XX (for levels 00-09)
        ss << timestamp << "," << sequence;
        
        // Output bid levels (00-09)
        for (int i = 0; i < 10; i++) {
            ss << ",";
            if (i < (int)bids.size()) {
                ss << fixed << setprecision(2) << bids[i].price;
            }
            ss << ",";
            if (i < (int)bids.size()) {
                ss << bids[i].size;
            }
        }
        
        // Output ask levels (00-09)
        for (int i = 0; i < 10; i++) {
            ss << ",";
            if (i < (int)asks.size()) {
                ss << fixed << setprecision(2) << asks[i].price;
            }
            ss << ",";
            if (i < (int)asks.size()) {
                ss << asks[i].size;
            }
        }
        
        output_lines.push_back(ss.str());
    }
    
public:
    void writeOutput() const {
        // Open output file
        ofstream outfile("mbp_output.csv");
        if (!outfile.is_open()) {
            cerr << "Error: Cannot create output file mbp_output.csv" << endl;
            return;
        }
        
        // Output header
        outfile << "timestamp,sequence";
        for (int i = 0; i < 10; i++) {
            outfile << ",bid_px_" << setfill('0') << setw(2) << i
                    << ",bid_sz_" << setfill('0') << setw(2) << i;
        }
        for (int i = 0; i < 10; i++) {
            outfile << ",ask_px_" << setfill('0') << setw(2) << i
                    << ",ask_sz_" << setfill('0') << setw(2) << i;
        }
        outfile << endl;
        
        // Output data
        for (const auto& line : output_lines) {
            outfile << line << endl;
        }
        
        outfile.close();
        cout << "Output written to mbp_output.csv" << endl;
    }
};

int main(int argc, char* argv[]) {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);
    
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <mbo.csv>" << endl;
        return 1;
    }
    
    try {
        MBOProcessor processor;
        processor.processFile(argv[1]);
        processor.writeOutput();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}