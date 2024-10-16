#include "protocol.h"
#include <iostream>
#include <chrono>
#include <thread>

class Emitter {
private:
    std::string test_message;
    int repeat_count;
    uint16_t packet_counter;
    int correct_messages;
    int error_messages;
    int undetected_errors;
    int total_bits_sent;
    int total_bits_received;

public:
    Emitter() : test_message(""), repeat_count(10000), packet_counter(0),
                correct_messages(0), error_messages(0), undetected_errors(0),
                total_bits_sent(0), total_bits_received(0) {
        wiringPiSetup();
        pinMode(TX_PIN, OUTPUT);
        pinMode(RX_PIN, INPUT);
    }

    void set_test_message() {
        std::cout << "Enter test message (max 20 chars): ";
        std::getline(std::cin, test_message);
        if (test_message.length() > 20) {
            test_message = test_message.substr(0, 20);
        }
    }

    void set_repeat_count() {
        std::cout << "Enter repeat count: ";
        std::cin >> repeat_count;
        std::cin.ignore();
    }

    void send_packet(const Packet& packet) {
        std::string data = packet.serialize();
        for (char c : data) {
            send_byte(c);
        }
        total_bits_sent += data.length() * 10;  // 8 data bits + start + stop
    }

    Packet receive_packet() {
        Packet packet;
        std::string data;
        for (int i = 0; i < 5 + packet.message_length; ++i) {
            data.push_back(receive_byte());
        }
        packet.deserialize(data);
        total_bits_received += data.length() * 10;
        return packet;
    }

    void send_test_message() {
        for (int i = 0; i < repeat_count; ++i) {
            Packet packet;
            packet.message_length = test_message.length();
            packet.command = 1;  // Test message command
            packet.packet_number = packet_counter++;
            packet.message = test_message;
            packet.error_check = packet.calculate_parity();

            send_packet(packet);

            // Wait for response
            Packet response = receive_packet();
            
            if (response.error_check == response.calculate_parity()) {
                if (response.message == test_message) {
                    correct_messages++;
                } else {
                    undetected_errors++;
                }
            } else {
                error_messages++;
            }

            if ((i + 1) % (repeat_count / 10) == 0) {
                display_statistics();
            }
        }
    }

    void display_statistics() {
        std::cout << "Correct messages: " << correct_messages << std::endl;
        std::cout << "Messages with detected errors: " << error_messages << std::endl;
        std::cout << "Messages with undetected errors: " << undetected_errors << std::endl;
        
        double ber = static_cast<double>(total_bits_received - total_bits_sent) / total_bits_sent;
        std::cout << "BER: " << ber << std::endl;
    }

    void run() {
        while (true) {
            std::cout << "\n1. Set test message\n"
                      << "2. Set repeat count\n"
                      << "3. Send test message\n"
                      << "4. Exit\n"
                      << "Enter your choice: ";
            int choice;
            std::cin >> choice;
            std::cin.ignore();

            switch (choice) {
                case 1: set_test_message(); break;
                case 2: set_repeat_count(); break;
                case 3: send_test_message(); break;
                case 4: return;
                default: std::cout << "Invalid choice. Try again.\n";
            }
        }
    }
};

int main() {
    Emitter emitter;
    emitter.run();
    return 0;
}