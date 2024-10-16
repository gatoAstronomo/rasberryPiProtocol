#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <string>
#include <bitset>

// Assuming we're using WiringPi for GPIO
#include <wiringPi.h>

const int TX_PIN = 17;
const int RX_PIN = 18;
const int BAUD_RATE = 9600;
const int BIT_DURATION = 1000000 / BAUD_RATE; // in microseconds

class Packet {
public:
    uint8_t message_length;
    uint8_t future_use : 5;
    uint8_t error_check;
    uint8_t command;
    uint16_t packet_number;
    std::string message;

    Packet() : message_length(0), future_use(0), error_check(0), command(0), packet_number(0) {}

    std::string serialize() const {
        std::string data;
        data.push_back(message_length);
        data.push_back((future_use << 3) | (error_check >> 5));
        data.push_back((error_check << 3) | command);
        data.push_back(packet_number >> 8);
        data.push_back(packet_number & 0xFF);
        data += message;
        return data;
    }

    void deserialize(const std::string& data) {
        if (data.length() < 5) return;
        message_length = data[0];
        future_use = (data[1] >> 3) & 0x1F;
        error_check = ((data[1] & 0x07) << 5) | ((data[2] >> 3) & 0x1F);
        command = data[2] & 0x07;
        packet_number = (static_cast<uint16_t>(data[3]) << 8) | data[4];
        message = data.substr(5);
    }

    uint8_t calculate_parity() const {
        std::bitset<8> parity;
        for (char c : serialize()) {
            parity ^= std::bitset<8>(c);
        }
        return parity.to_ulong() & 0xFF;
    }
};

void send_bit(bool bit) {
    digitalWrite(TX_PIN, bit ? HIGH : LOW);
    delayMicroseconds(BIT_DURATION);
}

bool receive_bit() {
    delayMicroseconds(BIT_DURATION / 2);  // Sample in the middle of the bit
    bool bit = digitalRead(RX_PIN) == HIGH;
    delayMicroseconds(BIT_DURATION / 2);
    return bit;
}

void send_byte(uint8_t byte) {
    send_bit(0);  // Start bit
    for (int i = 0; i < 8; ++i) {
        send_bit(byte & (1 << i));
    }
    send_bit(1);  // Stop bit
}

uint8_t receive_byte() {
    while (digitalRead(RX_PIN) == HIGH);  // Wait for start bit
    delayMicroseconds(BIT_DURATION * 1.5);  // Skip start bit
    
    uint8_t byte = 0;
    for (int i = 0; i < 8; ++i) {
        if (receive_bit()) {
            byte |= (1 << i);
        }
    }
    
    while (!receive_bit());  // Wait for stop bit
    return byte;
}

#endif // PROTOCOL_H