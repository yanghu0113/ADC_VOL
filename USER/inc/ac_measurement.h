#ifndef __AC_MEASUREMENT_H
#define __AC_MEASUREMENT_H

#include <stdint.h> // Use standard integer types
#include <stdbool.h> // For bool type

// --- Global Variables (declared extern) ---
extern volatile bool hlw8032_packet_ready; // Flag set by ISR

// --- Function Prototypes ---

/**
 * @brief Initializes the UART communication for HLW8032.
 */
void AC_Measurement_Init(void);

/**
 * @brief Processes a received HLW8032 packet from the internal buffer.
 *        Should be called from the main loop when hlw8032_packet_ready is true.
 */
void AC_Process_HLW8032_Packet(void);

/**
 * @brief Gets the last calculated RMS current from HLW8032 data.
 * @return float RMS current in Amperes. Returns last known value.
 */
float AC_GetCurrent(void);

/**
 * @brief Gets the last calculated RMS voltage from HLW8032 data.
 * @return float RMS voltage in Volts. Returns last known value.
 * @note Actual calculation needs to be implemented in AC_Process_HLW8032_Packet.
 */
float AC_GetVoltage(void);

/**
 * @brief Gets the last calculated Active Power from HLW8032 data.
 * @return float Active Power in Watts. Returns last known value.
 * @note Actual calculation needs to be implemented in AC_Process_HLW8032_Packet.
 */
float AC_GetPower(void);

/**
 * @brief Internal function (called by UART ISR) to store a received byte.
 * @param byte The received byte.
 */
void AC_Store_HLW8032_Byte(uint8_t byte);


#endif // __AC_MEASUREMENT_H
