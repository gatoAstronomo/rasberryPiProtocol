#include "protocol.h"
#include <iostream>

class Receiver {
private:
    bool running;

public:
    Receiver() : running(true) {
        wiringPiSetup();
        pinMode(RX_PIN, INPUT);
        pinMode(TX_PIN, OUTPUT);
    }

    Packet receive_packet() {
        Packet packet;
        std::string data;
        uint8_t length_byte = receive_byte();
        data.push_back(length_byte);
        
        // Receive header
        for (int i = 0; i < 4; ++i) {
            data.push_back(receive_byte());
        }
        
        // Receive message
        for (int i = 0; i < length_byte; ++i) {
            data.push_back(receive_byte());
        }
        
        packet.deserialize(data);
        return packet;
    }

    void send_packet(const Packet& packet) {
        std::string data = packet.serialize();
        for (char c : data) {
            send_byte(c);
        }
    }

    void process_packet(const Packet& packet) {
        if (packet.command == 255) {  // Close command
            running = false;
            return;
        }

        // Echo the received packet back to the sender
        Packet response = packet;
        response.command = 2;  // Response command
        send_packet(response);

        std::cout << "Received message: " << packet.message << std::endl;
    }

    void run() {
        std::cout << "Receiver started. Waiting for packets..." << std::endl;
        while (running) {
            Packet packet = receive_packet();
            process_packet(packet);
        }
        std::cout << "Receiver closed." << std::endl;
    }
};

int main() {
    Receiver receiver;
    receiver.run();
    return 0;
}