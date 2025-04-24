/* Match_detect
   Author: Jack Burton
   April 2025
   
   Continuously pulls UART responses from fingerprint sensor, checking
   for signature indicating successful Search, then enables DIO6 for 5sec.
   Requirements:
    UART Spy (Protocol tab) enabled with TX = DIO 1, RX = PIO0
    Baud rate 57.6k
    Parity: None, 8 Bits, Polarity: Standard
    Wavegen1 open with appropriate voltage for whatever you're powering (i.e. 2V for LED)
*/

if (!('Protocol' in this)) throw "Please open the Protocol tool";

Protocol.Mode.text = "UART"; // Ensure UART mode is selected
Protocol.UART.Receiver();    // Reset UART receiver

// Continuously check for packets
while(wait(0.1)) {
    var target = [7, 0, 7, 0]; // Target signature, indicates successful Search (which happens during matching process)
    

    var rx = Protocol.UART.ReceiveArray(); // Read received data
    
    // Print received data in hex format
    var hexStr = "";
    for (var i = 0; i < rx.length; i++) {
        var hex = rx[i].toString(16).toUpperCase();
        if (hex.length < 2) hex = "0" + hex;
        hexStr += hex + " ";
    }
    print(hexStr.trim());

    // Check for pattern match anywhere in rx
    var match = false;
    for (var i = 0; i <= rx.length - target.length; i++) {
        var found = true;
        for (var j = 0; j < target.length; j++) {
            if (rx[i + j] != target[j]) {
                found = false;
                break;
            }
        }
        if (found) { // End loop temporarily if matched fingerprint
            print("match found!\n")
            match = true;
            break;
        }
    }

    // Set DIO6 high if match found
    if (match) {
        Wavegen1.Channel1.checked = true;
        wait(5);
        // Protocol.DIO.Write(6, true);
    } else {
        Wavegen1.Channel1.checked = false;
        // Protocol.DIO.Write(6, false);
    }

    print("Polling...");
}
