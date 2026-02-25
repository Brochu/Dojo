#include <cassert>
#include <chrono>
#include <cstdio>
#include <initializer_list>
#include <vector>

using Timestamp = std::chrono::steady_clock::time_point;
using MinuteDuration = std::chrono::duration<double, std::chrono::minutes::period>;
#define TimestampNow std::chrono::steady_clock::now()

#define NUM_SIZES 3

enum class Size { SMALL, MEDIUM, LARGE };

class Vehicle {
public:
    Vehicle(Size s): size(s) { }

    Size size;
};

class ParkingSpot {
public:
    Vehicle *assignedVehicle = nullptr;
};

struct ParkingTicket {
    Size spotSize;
    int spotIndex;
};

class PricingStrategy {
    virtual int CalcPrice(int basePrice, Size spotSize, Timestamp start, Timestamp end) = 0;
};

class DurationBasedPricing : public PricingStrategy {
    virtual int CalcPrice(int basePrice, Size spotSize, Timestamp start, Timestamp end) override {
        return 0;
    };
};

class SizeBasedPricing : public PricingStrategy {
    virtual int CalcPrice(int basePrice, Size spotSize, Timestamp start, Timestamp end) override {
        return 0;
    };
};

class ParkingLot {
public:
    ParkingLot(int countSmall, int countMedium, int countLarge, std::initializer_list<PricingStrategy*> strats) {
        int counts[NUM_SIZES] { countSmall, countMedium, countLarge };

        for (int i = 0; i < NUM_SIZES; i++) {
            pools[i].spots.resize(counts[i]);
            pools[i].freelist.reserve(counts[i]);

            for (int j = 0; j < counts[i]; j++) {
                pools[i].freelist.push_back(j);
            }
        }

        strategies = strats;
    }

    ParkingTicket PlaceVehicle(Vehicle& v) {
        ParkingPool *pool = nullptr;
        Size chosenSize = (Size)0;

        for (int i = (int)v.size; i < NUM_SIZES; i++) {
            if (!pools[i].freelist.empty()) {
                pool = &pools[i];
                chosenSize = (Size)i;
                break;
            }
        }

        if (pool == nullptr) {
            //TODO: Better parking full handling
            assert(false && "Parking currently full");
        }

        int index = pool->freelist.back();
        pool->freelist.pop_back();
        ParkingSpot &spot = pool->spots[index];
        spot.assignedVehicle = &v;

        return { chosenSize, index };
    };

    void RemoveVehicle(ParkingTicket ticket) {
        ParkingPool &pool = pools[(size_t)ticket.spotSize];
        ParkingSpot &spot = pool.spots[ticket.spotIndex];

        if (spot.assignedVehicle == nullptr) {
            // Invalid ticket, spot already empty
            return;
        }

        spot.assignedVehicle = nullptr;
        pool.freelist.push_back(ticket.spotIndex);
    }

private:
    struct ParkingPool {
        std::vector<ParkingSpot> spots;
        std::vector<int> freelist;
    };

    ParkingPool pools[NUM_SIZES];
    std::vector<PricingStrategy*> strategies;
};

int main() {
    PricingStrategy *strats[] {
    };
    ParkingLot lot(10, 50, 10);
}
