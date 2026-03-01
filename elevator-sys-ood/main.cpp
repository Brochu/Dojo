#include <algorithm>
#include <cstdio>
#include <ppltasks.h>
#include <vector>

enum class Direction { UP, DOWN };

struct Elevator {
    int floor;
    Direction direction;

    std::vector<int> up_stops;
    std::vector<int> down_stops;
};

void elevator_add_stop(Elevator *e, int floor) {
    //TODO: Keep stops vectors ordered; find insert pos based on direction
    if (e->direction == Direction::UP) {
    }
    else if (e->direction == Direction::DOWN) {
    }
};

class PickStrategy {
public:
    virtual bool NextAvailable(int floor, Direction dir, int &elevator_id) = 0;
};

class FirstFreeStrategy : public PickStrategy{
public:
    virtual bool NextAvailable(int floor, Direction dir, int &elevator_id) override {
        return false;
    }
};

class ElevatorSystem {
public:
    ElevatorSystem(int num_floors, int num_elevators, PickStrategy *strat): floors_count(num_floors), strategy(strat) {
        elevators.resize(num_elevators);
    }

    void RequestElevator(int floor, Direction dir) {
        if (floor < 0 || floor > floors_count) {
            return;
        }

        int elevator_id = 0;
        strategy->NextAvailable(floor, dir, elevator_id);
        Elevator &e = elevators[elevator_id];

        if (e.up_stops.empty() && e.down_stops.empty()) {
            if (floor == e.floor) {
                e.direction = dir;
            }
            else {
                if (floor - e.floor > 0) {
                    e.direction = Direction::UP;
                    e.up_stops.push_back(floor);
                }
                else {
                    e.direction = Direction::DOWN;
                    e.down_stops.push_back(floor);
                }
            }
        }
        else {
            //TODO: Find good spot in one of the stops vectors
        }
    }

    void SelectFloor(int elevator_id, int floor) {
        if (elevator_id < 0 || elevator_id >= elevators.size()) {
            return;
        }
        if (floor < 0 || floor > floors_count) {
            return;
        }
    }

    void Step() {
        //TODO: Any point in making these update in parallel? Probably not for 3 elevators
        // They do not impact eachother should not require too much synchronization
        for (Elevator &e : elevators) {
        }
    }

    void DebugPrint() {
    }

private:
    int floors_count;
    std::vector<Elevator> elevators;

    PickStrategy *strategy;
};

int main() {
    printf("[MAIN] Elevator system online!\n");

    FirstFreeStrategy ffstrat;
    ElevatorSystem sys(32, 3, &ffstrat);
    sys.DebugPrint();

    return 0;
}
