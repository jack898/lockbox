if (!('Protocol' in this)) throw "Please open the Protocol tool";

Protocol.Mode.text = "UART"; // Ensure UART mode is selected
Protocol.UART.Receiver();    // Reset UART receiver

while(wait(0.1)) {
    // Define the target pattern in decimal
    var target = [7, 0, 7, 0];
    
    // Read received data
    var rx = Protocol.UART.ReceiveArray();
    
    // Print received data as hex manually
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
        if (found) {
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
