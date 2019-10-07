// Copyright (c) 2019 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php .

#ifndef MEMPOOLLIMIT_H
#define MEMPOOLLIMIT_H

#include <map>
#include <set>
#include <vector>

#include "boost/optional.hpp"
#include "primitives/transaction.h"
#include "uint256.h"

const size_t DEFAULT_MEMPOOL_TOTAL_WEIGHT_LIMIT = 80000000;
const int64_t DEFAULT_MEMPOOL_EVICTION_MEMORY_MINUTES = 60;

const size_t RECENTLY_EVICTED_SIZE = 10000;
const uint64_t MIN_TX_WEIGHT = 4000;
const uint64_t LOW_FEE_PENALTY = 16000; 

class RecentlyEvictedList
{
    const size_t capacity;
    size_t txIdsAndTimesIndex = 0;

    const int64_t timeToKeep;
    // Pairs of txid and time (seconds since epoch)
    boost::optional<std::pair<uint256, int64_t>> txIdsAndTimes[RECENTLY_EVICTED_SIZE];
    std::set<uint256> txIdSet;

    void pruneList();

public:
    RecentlyEvictedList(size_t capacity_, int64_t timeToKeep_) : capacity(capacity_), timeToKeep(timeToKeep_) 
    {
        assert(capacity <= RECENTLY_EVICTED_SIZE);
        std::fill_n(txIdsAndTimes, capacity, boost::none);
    }
    RecentlyEvictedList(int64_t timeToKeep_) : RecentlyEvictedList(RECENTLY_EVICTED_SIZE, timeToKeep_) {}

    void add(const uint256& txId);
    bool contains(const uint256& txId);
};


struct TxWeight {
    uint64_t weight;
    uint64_t lowFeePenaltyWeight;

    TxWeight(uint64_t weight_, uint64_t lowFeePenaltyWeight_)
        : weight(weight_), lowFeePenaltyWeight(lowFeePenaltyWeight_) {}

    TxWeight add(const TxWeight& other) const;
    TxWeight negate() const;
};


struct WeightedTxInfo {
    uint256 txId;
    TxWeight txWeight;

    WeightedTxInfo(uint256 txId_, TxWeight txWeight_) : txId(txId_), txWeight(txWeight_) {}

    static WeightedTxInfo from(const CTransaction& tx, const CAmount& fee);
};


class WeightedTxTree
{
    const uint64_t capacity;
    size_t size = 0;
    
    std::vector<WeightedTxInfo> txIdAndWeights;
    std::vector<TxWeight> childWeights;
    std::map<uint256, size_t> txIdToIndexMap;

    TxWeight getWeightAt(size_t index) const;
    void backPropagate(size_t fromIndex, const TxWeight& weightDelta);
    size_t findByWeight(size_t fromIndex, uint64_t weightToFind) const;

public:
    WeightedTxTree(uint64_t capacity_) : capacity(capacity_) {}

    TxWeight getTotalWeight() const;

    void add(const WeightedTxInfo& weightedTxInfo);
    void remove(const uint256& txId);

    boost::optional<uint256> maybeDropRandom();
};


#endif // MEMPOOLLIMIT_H
