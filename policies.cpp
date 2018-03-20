#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <ctime>
#include <algorithm>
#include "policies.hpp"
using std::vector;
using std::uniform_int_distribution;
using std::default_random_engine;

static const int INVALID_PAGE = -1;

/*!
 *  \brief Calculate number of page hits when using FIFO page replacement policy.
 *
 *  Calculates the number of page cache hits generated for a given sequence of
 *  page accesses when using the FIFO page replacement policy.
 *
 *  \param workload Vector of page accesses to evaluate

 *  \param memsize Memory size, in pages
 *  \return Number of cache hits generated by using FIFO policy
 */
int PRP_FIFO(const vector<int>& workload, unsigned int memsize) {
    int hits = 0;
    vector<int> cachedPages(memsize, INVALID_PAGE);
    vector<int>::size_type cachedPagesHead = 0;

    // Loop for each page access in workload
    for (int access : workload) {
        // Check if page being accessed is in the page cache
        if (std::count(cachedPages.begin(), cachedPages.end(), access) > 0) {
            // Page cache hit
            hits++;
        } else {
            // Page cache miss
            // Replace page at head of queue with the one being accessed
            cachedPages.at(cachedPagesHead) = access;
            // Move head of queue forward by one
            cachedPagesHead = (cachedPagesHead + 1) % cachedPages.size();
        }
    }

    return hits;
}

/*!
 *  \brief Calculate number of page hits when using optimal page replacement policy.
 *
 *  Calculates the number of page cache hits generated for a given sequence of
 *  page accesses when using the optimal page replacement policy.
 *
 *  \param workload Vector of page accesses to evaluate
 *	\param memsize Size of physical memory to work with (in pages)
 *
 *  \return Number of cache hits generated by using random policy
 */
int PRP_OPT(const vector<int>& workload, unsigned int memsize){
	std::unordered_set<int> pages_in_mem;
	int hits = 0;
	for(unsigned int i = 0; i < workload.size(); i++){
		if(pages_in_mem.count(workload[i]) > 0){
			hits++;
		} 
		else if(pages_in_mem.size() < memsize){
			pages_in_mem.insert(workload[i]);
		}
		else {
			std::unordered_set<int> replacement_canidiates = pages_in_mem;
			unsigned int j = i;
			for(;j < workload.size() && replacement_canidiates.size() > 1; j++){
				if(replacement_canidiates.count(workload[j] > 0)){
					replacement_canidiates.erase(workload[j]);
				}
			}
			pages_in_mem.erase( *(replacement_canidiates.begin()) );
			pages_in_mem.insert(workload[i]);
		}
	}
	return hits;
}

/*!
 *  \brief Calculate number of page hits when using random page replacement policy.
 *
 *  Calculates the number of page cache hits generated for a given sequence of
 *  page accesses when using the random page replacement policy.
 *
 *  \param workload Vector of page accesses to evaluate
 *	\param memsize Size of physical memory to work with (in pages)
 *
 *  \return Number of cache hits generated by using random policy
 */
int PRP_RAND(const vector<int>& workload, unsigned int memsize){
	vector<int> pages_in_mem;
	default_random_engine random_engine;
	random_engine.seed(std::time(NULL));
	int hits = 0;
	for(unsigned int i = 0; i < workload.size(); i++){
		bool found = false;
		for(unsigned int j = 0; j < pages_in_mem.size(); j++){
			if(pages_in_mem[j] == workload[i]){
				hits++;
				found = true;
				break;
			}
		}
		if(!found){
			if(pages_in_mem.size() < memsize){
				pages_in_mem.push_back(workload[i]);
			}
			else {
				uniform_int_distribution<int> gen(0 , pages_in_mem.size() - 1);
				unsigned int index = gen(random_engine);
				pages_in_mem[index] = workload[i];
			}
		}

	}
	return hits;
}

/*!
 *  \brief Calculate number of page hits when using LRU page replacement policy.
 *
 *  Calculates the number of page cache hits generated for a given sequence of
 *  page accesses when using the Least Recently Used page replacement policy.
 *
 *  \param workload Vector of page accesses to evaluate
 *  \param memsize Memory size, in pages
 *  \return Number of cache hits generated by using LRU policy
 */
int PRP_LRU(const vector<int>& workload, unsigned int memsize) {
    std::unordered_map<int, unsigned int> cache;
    unsigned int hits = 0;

    for (unsigned int time = 0; time < workload.size(); time++) {
        int access = workload[time];

        if (cache.count(access) == 0) {
            // Cache miss
            cache.emplace(access, time);

            // Should only ever loop once, but use while just in case
            while (cache.size() > memsize) {
                // Find least recently used entry and evict it from cache
                auto oldestEntry = cache.begin();
                for (auto iter = ++(cache.begin()); iter != cache.end(); iter++) {
                    if (iter->second < oldestEntry->second) {
                        oldestEntry = iter;
                    }
                }
                cache.erase(oldestEntry);
            }
        } else {
            // Cache hit
            hits++;
            cache.at(access) = time;
        }
    }

    return hits;
}

/*!
 *  \brief Calculate number of page hits when using the Clock page replacement policy.
 *
 *  Calculates the number of page cache hits generated for a given sequence of
 *  page accesses when using the Clock page replacement policy.
 *
 *  \param workload Vector of page accesses to evaluate
 *  \param memsize Memory size, in pages
 *  \return Number of cache hits generated by using Clock policy
 */
int PRP_CLOCK(const vector<int>& workload, unsigned int memsize) {
    // first: accessed page, second: use bit
    typedef std::pair<int, bool> ClockEntry;
    typedef vector<ClockEntry> Clock;

    int hits = 0;

    Clock cache;
    for (auto access : workload) {
        // Find page in cache if it is already there
        Clock::iterator iter = std::find_if(cache.begin(), cache.end(),
            [access](const ClockEntry& elem) {
                return elem.first == access;
            }
        );

        if (iter != cache.end()) {
            // Cache hit
            hits++;
            iter->second = true;
        } else {
            // Cache miss
            if (cache.size() < memsize) {
                // Cache can fit another page
                cache.emplace_back(access, true);
            } else {
                // Select a victim page to evict
                Clock::iterator clockHand = cache.begin();
                for (; clockHand != cache.end(); clockHand++) {
                    if (clockHand->second) {
                        clockHand->second = false;
                    } else {
                        break;
                    }
                }
                // Loop back to first page in cache if clockHand went past last entry
                if (clockHand == cache.end()) {
                    clockHand = cache.begin();
                }

                // Replace victim page in cache with page we are now accessing
                clockHand->first = access;
                clockHand->second = true;
            }
        }
    }

    return hits;
}
