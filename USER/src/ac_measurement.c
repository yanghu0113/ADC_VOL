#include "ac_measurement.h"
#include "hlw_uart_driver.h" // Use the dedicated HLW UART driver
#include "config.h"          // For HLW_UART_BAUDRATE
#include <stdio.h>           // For debugging printf
#include <string.h>          // For memcpy

// --- Defines ---
#define HLW8032_PACKET_SIZE 24
#define HLW8032_CURRENT_REG_INDEX 15 // Index of first byte of Current REG (0-based)
#define HLW8032_VOLTAGE_REG_INDEX 6  // Index of first byte of Voltage REG
#define HLW8032_POWER_REG_INDEX   18 // Index of first byte of Power REG
#define HLW8032_CHECKSUM_INDEX    23 // Index of Checksum REG

// --- Global Variables ---
static uint8_t hlw8032_rx_packet_buffer[HLW8032_PACKET_SIZE];
static uint8_t hlw8032_rx_byte_count = 0;
volatile bool hlw8032_packet_ready = false; // Flag set by ISR when 24 bytes received

// Storage for calculated values
static float ac_rms_current = 0.0f;
static float ac_rms_voltage = 0.0f;
static float ac_active_power = 0.0f;
// Add calibration factors if needed (from datasheet)
// static float current_coeff = 1.0f; // Example: Amps per LSB
// static float voltage_coeff = 1.0f; // Example: Volts per LSB
// static float power_coeff = 1.0f;   // Example: Watts per LSB


// --- Initialization ---

/**
 * @brief Initializes the UART communication for HLW8032.
 */
void AC_Measurement_Init(void)
{
    hlw8032_rx_byte_count = 0;
    hlw8032_packet_ready = false;
    memset(hlw8032_rx_packet_buffer, 0, HLW8032_PACKET_SIZE);

    // Initialize UART2 for HLW8032
    if (!HLW_UART_Init(HLW_UART_BAUDRATE)) {
         printf("Error: HLW8032 UART Init Failed!\r\n");
         // Handle error appropriately
    } else {
         printf("HLW8032 UART Initialized.\r\n");
    }
}

// --- Data Processing ---

/**
 * @brief Calculates the checksum for the HLW8032 packet.
 * @param buffer Pointer to the 24-byte packet buffer.
 * @return Calculated 8-bit checksum.
 */
static uint8_t Calculate_HLW8032_Checksum(const uint8_t* buffer)
{
    uint16_t sum = 0;
    for (int i = 0; i < HLW8032_CHECKSUM_INDEX; i++) { // Sum first 23 bytes
        sum += buffer[i];
    }
    return (uint8_t)(sum & 0xFF); // Return lower 8 bits
}

/**
 * @brief Processes a received HLW8032 packet from the buffer.
 *        Validates checksum and parses data.
 */
void AC_Process_HLW8032_Packet(void)
{
    // Make a local copy to avoid race conditions if ISR modifies buffer during processing
    uint8_t local_buffer[HLW8032_PACKET_SIZE];
    memcpy(local_buffer, hlw8032_rx_packet_buffer, HLW8032_PACKET_SIZE);

    // Validate checksum
    uint8_t received_checksum = local_buffer[HLW8032_CHECKSUM_INDEX];
    uint8_t calculated_checksum = Calculate_HLW8032_Checksum(local_buffer);

    if (received_checksum == calculated_checksum) {
        // Checksum OK - Parse data
        // Data is typically 24-bit, MSB first
        uint32_t raw_voltage = ((uint32_t)local_buffer[HLW8032_VOLTAGE_REG_INDEX] << 16) |
                               ((uint32_t)local_buffer[HLW8032_VOLTAGE_REG_INDEX + 1] << 8) |
                               local_buffer[HLW8032_VOLTAGE_REG_INDEX + 2];

        uint32_t raw_current = ((uint32_t)local_buffer[HLW8032_CURRENT_REG_INDEX] << 16) |
                               ((uint32_t)local_buffer[HLW8032_CURRENT_REG_INDEX + 1] << 8) |
                               local_buffer[HLW8032_CURRENT_REG_INDEX + 2];

        uint32_t raw_power = ((uint32_t)local_buffer[HLW8032_POWER_REG_INDEX] << 16) |
                             ((uint32_t)local_buffer[HLW8032_POWER_REG_INDEX + 1] << 8) |
                             local_buffer[HLW8032_POWER_REG_INDEX + 2];

        // TODO: Convert raw values to actual Volts, Amps, Watts using datasheet coefficients
        // These are placeholders - replace with actual conversion formulas from datasheet!
        float voltage_coeff = 0.01f; // Example: Replace with actual V/LSB
        float current_coeff = 0.001f; // Example: Replace with actual A/LSB
        float power_coeff = 0.01f;   // Example: Replace with actual W/LSB

        ac_rms_voltage = (float)raw_voltage * voltage_coeff;
        ac_rms_current = (float)raw_current * current_coeff;
        ac_active_power = (float)raw_power * power_coeff;

        // Optional: Recalculate current from Power and Voltage if needed (e.g., if current reading is less reliable)
        // if (ac_rms_voltage > 1.0f) { // Avoid division by zero or small voltage
        //    ac_rms_current = ac_active_power / ac_rms_voltage; // Assumes Power Factor = 1
        // }

        // printf("HLW Packet OK: V=%.2fV, I=%.3fA, P=%.2fW\n", ac_rms_voltage, ac_rms_current, ac_active_power);

    } else {
        // Checksum error
        printf("HLW Checksum Error! Recv: 0x%02X, Calc: 0x%02X\n", received_checksum, calculated_checksum);
        // Optionally clear stored values or keep last known good values
        // ac_rms_current = 0.0f;
    }
}


// --- Public Getter Functions ---

/**
 * @brief Gets the last calculated RMS current.
 * @return float RMS current in Amperes.
 */
float AC_GetCurrent(void)
{
    // Returns the latest value updated by AC_Process_HLW8032_Packet
    return ac_rms_current;
}

/**
 * @brief Gets the last calculated RMS voltage.
 * @return float RMS voltage in Volts.
 */
float AC_GetVoltage(void)
{
    // Returns the latest value updated by AC_Process_HLW8032_Packet
    return ac_rms_voltage;
}

/**
 * @brief Gets the last calculated Active Power.
 * @return float Active Power in Watts.
 */
float AC_GetPower(void)
{
     // Returns the latest value updated by AC_Process_HLW8032_Packet
    return ac_active_power;
}

// --- Internal ISR Helper ---
/**
 * @brief Stores a received byte from HLW8032 UART ISR.
 *        Sets packet ready flag when 24 bytes are received.
 * @param byte The received byte.
 */
void AC_Store_HLW8032_Byte(uint8_t byte)
{
    if (hlw8032_rx_byte_count < HLW8032_PACKET_SIZE) {
        hlw8032_rx_packet_buffer[hlw8032_rx_byte_count++] = byte;
        if (hlw8032_rx_byte_count >= HLW8032_PACKET_SIZE) {
            hlw8032_packet_ready = true; // Signal main loop to process
            hlw8032_rx_byte_count = 0; // Reset for next packet
        }
    } else {
        // Buffer overflow or synchronization issue, reset count
        hlw8032_rx_byte_count = 0;
        // Optionally log an error
    }
}
