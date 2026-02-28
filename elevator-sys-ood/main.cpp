#include <cstdio>
#include <vector>

enum class Direction { UP, DOWN };
enum class State { CLOSED, MOVING, OPENED };

struct Elevator {
    State state;
    Direction dir;

    int floor;
    int target;
};

class ElevatorSystem {
public:
    ElevatorSystem(int num_floors, int num_elevators): floors_count(num_floors) {
        elevators.resize(num_elevators);
    }

    void RequestElevator(int floor, Direction dir) {
    }

    void SelectFloor(int elevator_id, int floor) {
        if (elevator_id < 0 || elevator_id >= elevators.size()) {
            return;
        }
        if (floor < 0 || floor > floors_count) {
            return;
        }

        elevators[elevator_id].target = floor;
    }

    void Step() {
        //TODO: Any point in making these update in parallel? Probably not for 3 elevators
        // They do not impact eachother should not require too much synchronization
        for (Elevator &e : elevators) {
            if (e.state == State::OPENED) {
                e.state = (e.floor == e.target) ? State::CLOSED : State::MOVING;
            }
            else if (e.state == State::MOVING) {
                if (e.floor == e.target) {
                    e.state = State::OPENED;
                } else {
                    e.floor = (e.dir == Direction::UP) ? e.floor + 1 : e.floor - 1;
                }
            }
            else if (e.state == State::CLOSED) {
                if (e.floor != e.target) {
                    e.state = State::MOVING;
                }
            }
        }
    }

    void DebugPrint() {
        printf("[SYS] %zu Elevators covering %i floors: \n", elevators.size(), floors_count);
        for (Elevator &e : elevators) {
            printf(" - [%i] {%i} - floor %i; target - %i\n", e.state, e.dir, e.floor, e.target);
        }
    }

private:
    int floors_count;
    std::vector<Elevator> elevators;
};

int main() {
    printf("[MAIN] Elevator system online!\n");

    ElevatorSystem sys(32, 3);
    sys.DebugPrint();

    return 0;
}
