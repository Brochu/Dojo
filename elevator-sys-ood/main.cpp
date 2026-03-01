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
        if (pos == e->up_stops.end() || (*pos) != floor) {
            e->up_stops.emplace(pos, floor);
        }
    }
    else if (dir == Direction::DOWN) {
        auto pos = e->down_stops.begin();
        while (pos != e->down_stops.end() && (*pos) > floor) {
            pos++;
        }
        if (pos == e->down_stops.end() || (*pos) != floor) {
            e->down_stops.emplace(pos, floor);
        }
    }
};

class PickStrategy {
public:
    virtual bool NextAvailable(int floor, Direction dir, std::vector<Elevator> &elevators, int &elevator_id) = 0;
};

class FirstFreeStrategy : public PickStrategy{
public:
    virtual bool NextAvailable(int floor, Direction dir, std::vector<Elevator> &elevators, int &elevator_id) override {
        elevator_id = -1;

        for (int i = 0; i < elevators.size(); i++) {
            if (elevators[i].direction == dir) {
                elevator_id = i;
                return true;
            }
        }

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
        strategy->NextAvailable(floor, dir, elevators, elevator_id);
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
        Elevator &e = elevators[elevator_id];

        if (floor < 0 || floor > floors_count || floor == e.floor) {
            return;
        }

        elevator_add_stop(&e, floor, (floor - e.floor > 0) ? Direction::UP : Direction::DOWN);
    }

    void Step() {
        //TODO: Any point in making these update in parallel? Probably not for 3 elevators
        // They do not impact eachother should not require too much synchronization
        for (Elevator &e : elevators) {
            if (e.direction == Direction::UP) {
                if (e.up_stops.empty()) {
                    continue;
                }

                int target = e.up_stops.front();
                if (e.floor == target) {
                    e.up_stops.erase(e.up_stops.begin());
                    if (e.up_stops.empty() && !e.down_stops.empty()) {
                        e.direction = Direction::DOWN;
                    }
                }
                else {
                    e.floor++;
                }
            }
            else if (e.direction == Direction::DOWN) {
                if (e.down_stops.empty()) {
                    continue;
                }

                int target = e.down_stops.front();
                if (e.floor == target) {
                    e.down_stops.erase(e.down_stops.begin());
                    if (e.down_stops.empty() && !e.up_stops.empty()) {
                        e.direction = Direction::UP;
                    }
                }
                else {
                    e.floor--;
                }
            }
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
