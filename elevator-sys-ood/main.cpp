#include <cstdio>

enum class Direction { UP, DOWN };

class ElevatorSystem {
public:
    ElevatorSystem() {}

    void RequestElevator(int floor, Direction dir) {}
    void SelectFloor(int elevatorId, int floor) {}
    void Step() {}
};

int main() {
    printf("[MAIN] Elevator system online!\n");
    return 0;
}
