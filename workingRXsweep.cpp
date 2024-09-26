#include <libbladeRF.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>

#define FREQUENCY 925e6  // Frequency in Hz
#define SAMPLE_RATE 2e6  // Sample rate in Hz
#define NUM_SAMPLES 1600 // Number of samples
#define MAX_ADC_VALUE 2047.0 // Maximum value for 12-bit ADC
#define FULL_SCALE_VOLTAGE 0.625 // Full scale voltage in V peak for ADC
#define LOAD_IMPEDANCE 50.0 // Load impedance in Ohms
#define NOISE_FLOOR -8.5 // Noise Floor at -8.5 dBm

int main() {
    int status;
    struct bladerf *dev = nullptr;
    unsigned int start_freq = 325e6, end_freq = 1025e6, step_freq = 25e6;
    // Open the device
    status = bladerf_open(&dev, nullptr);
    if (status != 0) {
        std::cerr << "Failed to open device: " << bladerf_strerror(status) << std::endl;
        return -1;
    }

    status = bladerf_set_gain_mode(dev, BLADERF_CHANNEL_RX(0), BLADERF_GAIN_MANUAL);
    if (status != 0) {
        std::cerr << "Failed to disable AGC: " << bladerf_strerror(status) << std::endl;
        // Handle error (e.g., cleanup and exit)
    }

    for (unsigned int frequency = start_freq; frequency <= end_freq; frequency += step_freq) {
    // Set frequency and sample rate
    status = bladerf_set_frequency(dev, BLADERF_CHANNEL_RX(0), frequency);
    status |= bladerf_set_sample_rate(dev, BLADERF_CHANNEL_RX(0), SAMPLE_RATE, nullptr);
    if (status != 0) {
        std::cerr << "Failed to configure device: " << bladerf_strerror(status) << std::endl;
        bladerf_close(dev);
        return -1;
    }
    std::cout << "Monitoring Frequency: " << frequency << std::endl;
    // Enable RX module
    status = bladerf_enable_module(dev, BLADERF_CHANNEL_RX(0), true);
    if (status != 0) {
        std::cerr << "Failed to enable RX module: " << bladerf_strerror(status) << std::endl;
        bladerf_close(dev);
        return -1;
    }

    // Configure synchronization
    status = bladerf_sync_config(dev,
                                 BLADERF_RX_X1,
                                 BLADERF_FORMAT_SC16_Q11,
                                 64,
                                 1024,
                                 16,
                                 10000);
    if (status != 0) {
        std::cerr << "Failed to configure sync: " << bladerf_strerror(status) << std::endl;
        bladerf_enable_module(dev, BLADERF_CHANNEL_RX(0), false);
        bladerf_close(dev);
        return -1;
    }

    std::vector<int16_t> samples(NUM_SAMPLES * 2); // 2x for I and Q components
    double totalPowerdBm = 0;

    // Open a file in write mode 
  /*  std::ofstream outFile("IQ_Samples.txt");

    if (!outFile.is_open()) {
        std::cerr << "Failed to open file for writing." << std::endl;
        // Handle the error (e.g., cleanup and exit)
        return -1; // Exit or handle the error appropriately
    }
*/
    // Receive a batch of samples
status = bladerf_sync_rx(dev, samples.data(), NUM_SAMPLES, nullptr, 5000);
if (status != 0) {
    std::cerr << "Failed to receive samples: " << bladerf_strerror(status) << std::endl;
} else {
    // Process each sample to calculate its power in dBm and print it
    for (size_t i = 0; i < NUM_SAMPLES; ++i) {
        double I_voltage = (samples[2 * i] / MAX_ADC_VALUE) * FULL_SCALE_VOLTAGE / sqrt(2); // Convert to Vrms
        double Q_voltage = (samples[2 * i + 1] / MAX_ADC_VALUE) * FULL_SCALE_VOLTAGE / sqrt(2); // Convert to Vrms

        double powerWatts = (pow(I_voltage, 2) + pow(Q_voltage, 2)) / LOAD_IMPEDANCE;
        double powerdBm = 10 * log10(powerWatts) + 30; // Convert to dBm

        //std::cout << "Power of sample " << i << " = " << powerdBm << " dBm" << std::endl;
        totalPowerdBm+=powerdBm;
    }
}

    if (NUM_SAMPLES > 0) {
        double avgPowerdBm = totalPowerdBm / NUM_SAMPLES;
        std::cout << "Average Power: " << avgPowerdBm << " dBm" << std::endl;
        if (avgPowerdBm > NOISE_FLOOR+15){
            std::cout << "Signal Detected Above Threshold" << std::endl;
        }
    } else {
        std::cerr << "No samples were processed." << std::endl;
    }
    }
    // Cleanup
    bladerf_enable_module(dev, BLADERF_CHANNEL_RX(0), false);
    bladerf_close(dev);

    return 0;
}
