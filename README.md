/**
 * Simulates a digital display interface for an elevator control system.
 * It is used to test the functionalities of the Serial_Communicator software (lready commited in a previous project).
 * 
 * This class handles the display update for floor numbers, animations, and elevator state messages.
 * It uses a separate thread to simulate the functionality of hardware display updates asynchronously.
 * 
 * Key Features:
 * - Mutex locks are used to ensure that the function calling order is respected in a multi-threaded environment,
 *   avoiding race conditions when the display is being updated.
 * - Utilizes modern C++ threading mechanisms to run the display update loop on a separate thread, improving performance
 *   by decoupling display rendering from the main control logic.
 * - Handles multiple modes such as displaying the current floor, opening/closing doors, or warning about elevator
 *   overload. These modes can be triggered via corresponding function calls.
 * 
 * This simulation is designed for debugging and testing purposes and emulates the behavior of physical display
 * hardware found in elevator systems.
 *
 * Methods:
 * - updateFloorNumber(int floorNumber): Displays the current floor number on the screen.
 * - showOpenDoorMessage(): Simulates a display message when the elevator doors are opening.
 * - showCloseDoorMessage(): Simulates a display message when the elevator doors are closing.
 * - showOverloadMessage(): Displays an overload warning message.
 * - resetDisplay(): Clears the display.
 * 
 * The displayThread function continuously updates the display based on the system state,
 * ensuring that the display reflects the elevator's current mode or condition.
 * The system uses a mutex to protect shared resources and maintain synchronization between threads.
 */
