#ifndef RMS_STATISTICS_H
#define RMS_STATISTICS_H

#include <stdint.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
// Configuration
#define RMS_STATS_WINDOW_SIZE       40     // Number of samples for statistics
#define RMS_STATS_DEMAND_WINDOW     20      // Demand calculation window (15 minutes at 2s interval = 450, but using 50 for demo)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Struct ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
// Statistics structure for each RMS channel
typedef struct {
    float samples[RMS_STATS_WINDOW_SIZE];   // Ring buffer for samples
    uint16_t write_index;                   // Current write position
    uint16_t sample_count;                  // Number of valid samples (0 to WINDOW_SIZE)
    
    // Statistical values
    float min_value;
    float max_value;
    float avg_value;
    float demand_value;                     // Maximum of sliding window averages
    
    // Demand calculation
    float demand_window[RMS_STATS_DEMAND_WINDOW];
    uint16_t demand_index;
    uint16_t demand_count;
    
    uint8_t stats_valid;                    // 1 if statistics are valid
} rms_stats_t;

// Statistics for all RMS channels
typedef struct {
    rms_stats_t voltage_l1;     // g_RMS_Value[3]
    rms_stats_t voltage_l2;     // g_RMS_Value[4] 
    rms_stats_t voltage_l3;     // g_RMS_Value[5]
    rms_stats_t current_l1;     // g_RMS_Value[0]
    rms_stats_t current_l2;     // g_RMS_Value[1]
    rms_stats_t current_l3;     // g_RMS_Value[2]
} rms_all_stats_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototype ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
// Function prototypes
void RMS_Stats_Init(void);
void RMS_Stats_Update(void);
void RMS_Stats_Reset(void);
void RMS_Stats_ResetChannel(rms_stats_t *stats);
rms_all_stats_t* RMS_Stats_GetData(void);

// Helper functions
float RMS_Stats_GetChannelMin(uint8_t channel);
float RMS_Stats_GetChannelMax(uint8_t channel);
float RMS_Stats_GetChannelAvg(uint8_t channel);
float RMS_Stats_GetChannelDemand(uint8_t channel);

#endif /* RMS_STATISTICS_H */
