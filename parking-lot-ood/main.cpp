#include <cassert>
#include <cstdio>
#include <vector>

enum class VehicleSize { SMALL, MEDIUM, LARGE };

class Vehicle {

public:
    Vehicle(VehicleSize s): size(s) { }

    VehicleSize size;
};

//TODO: Maybe need subclasses for different parking sizes
class ParkingSpot {

public:
    ParkingSpot() { }

    int Index; // This doesn't work
};

class ParkingLot {

public:
    ParkingLot(int smallCount, int medCount, int largeCount) {
        spotsSmall.resize(smallCount);
        freeSmall.reserve(smallCount);

        spotsMedium.resize(medCount);
        freeMedium.reserve(medCount);

        spotsLarge.resize(largeCount);
        freeLarge.reserve(largeCount);

        for (int i = 0; i < smallCount; i++) {
            spotsSmall[i] = {};
            freeSmall.push_back(i);
        }
        for (int i = 0; i < medCount; i++) {
            spotsMedium[i] = {};
            freeMedium.push_back(i);
        }
        for (int i = 0; i < largeCount; i++) {
            spotsLarge[i] = {};
            freeLarge.push_back(i);
        }
    }

    const ParkingSpot& PlaceVehicle(const Vehicle& v) {
        std::vector<ParkingSpot> &spots = chooseSpots(v.size);
        std::vector<int> &freelist = chooseFree(v.size);;

        const int index = freelist.back();
        freelist.pop_back();
        ParkingSpot &spot = spots[index];

        return spot;
    };

    void RemoveVehicle(const Vehicle& v, const ParkingSpot& s) {
        std::vector<int> &freelist = chooseFree(v.size);
        freelist.push_back(s.Index);
    }

private:
    std::vector<ParkingSpot> spotsSmall;
    std::vector<ParkingSpot> spotsMedium;
    std::vector<ParkingSpot> spotsLarge;

    std::vector<int> freeSmall;
    std::vector<int> freeMedium;
    std::vector<int> freeLarge;

    std::vector<ParkingSpot> &chooseSpots(VehicleSize s) {
        switch (s) {
            case VehicleSize::SMALL: return spotsSmall;
            case VehicleSize::MEDIUM: return spotsMedium;
            case VehicleSize::LARGE: return spotsLarge;
            default: assert(false && "Invalid vehicle size");
        }
    }
    std::vector<int> &chooseFree(VehicleSize s) {
        switch (s) {
            case VehicleSize::SMALL: return freeSmall;
            case VehicleSize::MEDIUM: return freeMedium;
            case VehicleSize::LARGE: return freeLarge;
            default: assert(false && "Invalid vehicle size");
        }
    }
};

int main() {
    printf("Hello world\n");
}
