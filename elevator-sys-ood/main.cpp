#include <cstdio>
#include <vector>

enum class Direction { UP, DOWN };

struct Elevator {
    int floor;
    Direction direction;

    std::vector<int> up_stops;
    std::vector<int> down_stops;
};

void elevator_add_stop(Elevator *e, int floor, Direction dir) {
    if (dir == Direction::UP) {
        auto pos = e->up_stops.begin();
        while (pos != e->up_stops.end() && (*pos) < floor) {
            pos++;
        }
        e->up_stops.emplace(pos, floor);
    }
    else if (dir == Direction::DOWN) {
        auto pos = e->down_stops.begin();
        while (pos != e->down_stops.end() && (*pos) > floor) {
            pos++;
        }
        e->down_stops.emplace(pos, floor);
    }
    //TODO: Need logic to avoid duplicates
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
            elevator_add_stop(&e, floor, dir);
        }
    }

    void SelectFloor(int elevator_id, int floor) {
        if (elevator_id < 0 || elevator_id >= elevators.size()) {
            return;
        }
        if (floor < 0 || floor > floors_count) {
            return;
        }

        Elevator &e = elevators[elevator_id];
        elevator_add_stop(&e, floor, e.direction);
    }

    void Step() {
        //TODO: Any point in making these update in parallel? Probably not for 3 elevators
        // They do not impact eachother should not require too much synchronization
        for (Elevator &e : elevators) {
            //TODO: Process elevators
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
