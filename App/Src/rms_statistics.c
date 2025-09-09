/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Include ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include "rms_statistics.h"
#include "calculate_task.h"
#include <string.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static rms_all_stats_t g_rms_stats;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Prototype ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static void calculate_statistics(rms_stats_t *stats);
static void update_demand(rms_stats_t *stats, float new_value);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Public Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* :::::::::: Initialize RMS statistics :::::::: */
void RMS_Stats_Init(void)
{
    memset(&g_rms_stats, 0, sizeof(g_rms_stats));
    
    // Initialize each channel
    RMS_Stats_ResetChannel(&g_rms_stats.voltage_l1);
    RMS_Stats_ResetChannel(&g_rms_stats.voltage_l2);
    RMS_Stats_ResetChannel(&g_rms_stats.voltage_l3);
    RMS_Stats_ResetChannel(&g_rms_stats.current_l1);
    RMS_Stats_ResetChannel(&g_rms_stats.current_l2);
    RMS_Stats_ResetChannel(&g_rms_stats.current_l3);
}

/* :::::::::: Update statistics with new RMS values :::::::: */
void RMS_Stats_Update(void)
{
    // Update voltage statistics
    if (g_Phase_Active[0])  // L1 active
    {
        // Add voltage sample
        rms_stats_t *v_stats = &g_rms_stats.voltage_l1;
        v_stats->samples[v_stats->write_index] = g_RMS_Value[3];
        v_stats->write_index = (v_stats->write_index + 1) % RMS_STATS_WINDOW_SIZE;
        if (v_stats->sample_count < RMS_STATS_WINDOW_SIZE) {
            v_stats->sample_count++;
        }
        calculate_statistics(v_stats);
        update_demand(v_stats, g_RMS_Value[3]);
        
        // Add current sample
        rms_stats_t *i_stats = &g_rms_stats.current_l1;
        i_stats->samples[i_stats->write_index] = g_RMS_Value[0];
        i_stats->write_index = (i_stats->write_index + 1) % RMS_STATS_WINDOW_SIZE;
        if (i_stats->sample_count < RMS_STATS_WINDOW_SIZE) {
            i_stats->sample_count++;
        }
        calculate_statistics(i_stats);
        update_demand(i_stats, g_RMS_Value[0]);
    }
    
    if (g_Phase_Active[1])  // L2 active
    {
        // Add voltage sample
        rms_stats_t *v_stats = &g_rms_stats.voltage_l2;
        v_stats->samples[v_stats->write_index] = g_RMS_Value[4];
        v_stats->write_index = (v_stats->write_index + 1) % RMS_STATS_WINDOW_SIZE;
        if (v_stats->sample_count < RMS_STATS_WINDOW_SIZE) {
            v_stats->sample_count++;
        }
        calculate_statistics(v_stats);
        update_demand(v_stats, g_RMS_Value[4]);
        
        // Add current sample
        rms_stats_t *i_stats = &g_rms_stats.current_l2;
        i_stats->samples[i_stats->write_index] = g_RMS_Value[1];
        i_stats->write_index = (i_stats->write_index + 1) % RMS_STATS_WINDOW_SIZE;
        if (i_stats->sample_count < RMS_STATS_WINDOW_SIZE) {
            i_stats->sample_count++;
        }
        calculate_statistics(i_stats);
        update_demand(i_stats, g_RMS_Value[1]);
    }
    
    if (g_Phase_Active[2])  // L3 active
    {
        // Add voltage sample
        rms_stats_t *v_stats = &g_rms_stats.voltage_l3;
        v_stats->samples[v_stats->write_index] = g_RMS_Value[5];
        v_stats->write_index = (v_stats->write_index + 1) % RMS_STATS_WINDOW_SIZE;
        if (v_stats->sample_count < RMS_STATS_WINDOW_SIZE) {
            v_stats->sample_count++;
        }
        calculate_statistics(v_stats);
        update_demand(v_stats, g_RMS_Value[5]);
        
        // Add current sample
        rms_stats_t *i_stats = &g_rms_stats.current_l3;
        i_stats->samples[i_stats->write_index] = g_RMS_Value[2];
        i_stats->write_index = (i_stats->write_index + 1) % RMS_STATS_WINDOW_SIZE;
        if (i_stats->sample_count < RMS_STATS_WINDOW_SIZE) {
            i_stats->sample_count++;
        }
        calculate_statistics(i_stats);
        update_demand(i_stats, g_RMS_Value[2]);
    }
}

/* :::::::::: Reset all statistics :::::::: */
void RMS_Stats_Reset(void)
{
    RMS_Stats_Init();
}

/* :::::::::: Reset single channel statistics :::::::: */
void RMS_Stats_ResetChannel(rms_stats_t *stats)
{
    memset(stats, 0, sizeof(rms_stats_t));
    stats->min_value = 999999.0f;  // Initialize to large value
    stats->max_value = -999999.0f; // Initialize to small value
}

/* :::::::::: Get statistics data pointer :::::::: */
rms_all_stats_t* RMS_Stats_GetData(void)
{
    return &g_rms_stats;
}

/* :::::::::: Helper functions to get specific channel statistics :::::::: */
float RMS_Stats_GetChannelMin(uint8_t channel)
{
    switch(channel)
    {
        case 0: return g_rms_stats.voltage_l1.min_value;  // V1
        case 1: return g_rms_stats.voltage_l2.min_value;  // V2
        case 2: return g_rms_stats.voltage_l3.min_value;  // V3
        case 3: return g_rms_stats.current_l1.min_value;  // I1
        case 4: return g_rms_stats.current_l2.min_value;  // I2
        case 5: return g_rms_stats.current_l3.min_value;  // I3
        default: return 0.0f;
    }
}

float RMS_Stats_GetChannelMax(uint8_t channel)
{
    switch(channel)
    {
        case 0: return g_rms_stats.voltage_l1.max_value;  // V1
        case 1: return g_rms_stats.voltage_l2.max_value;  // V2
        case 2: return g_rms_stats.voltage_l3.max_value;  // V3
        case 3: return g_rms_stats.current_l1.max_value;  // I1
        case 4: return g_rms_stats.current_l2.max_value;  // I2
        case 5: return g_rms_stats.current_l3.max_value;  // I3
        default: return 0.0f;
    }
}

float RMS_Stats_GetChannelAvg(uint8_t channel)
{
    switch(channel)
    {
        case 0: return g_rms_stats.voltage_l1.avg_value;  // V1
        case 1: return g_rms_stats.voltage_l2.avg_value;  // V2
        case 2: return g_rms_stats.voltage_l3.avg_value;  // V3
        case 3: return g_rms_stats.current_l1.avg_value;  // I1
        case 4: return g_rms_stats.current_l2.avg_value;  // I2
        case 5: return g_rms_stats.current_l3.avg_value;  // I3
        default: return 0.0f;
    }
}

float RMS_Stats_GetChannelDemand(uint8_t channel)
{
    switch(channel)
    {
        case 0: return g_rms_stats.voltage_l1.demand_value;  // V1
        case 1: return g_rms_stats.voltage_l2.demand_value;  // V2
        case 2: return g_rms_stats.voltage_l3.demand_value;  // V3
        case 3: return g_rms_stats.current_l1.demand_value;  // I1
        case 4: return g_rms_stats.current_l2.demand_value;  // I2
        case 5: return g_rms_stats.current_l3.demand_value;  // I3
        default: return 0.0f;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Private Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* :::::::::: Calculate min, max, average for a channel :::::::: */
static void calculate_statistics(rms_stats_t *stats)
{
    if (stats->sample_count == 0) {
        stats->stats_valid = 0;
        return;
    }
    
    float sum = 0;
    float min_val = stats->samples[0];
    float max_val = stats->samples[0];
    
    // Calculate min, max, sum
    for (uint16_t i = 0; i < stats->sample_count; i++)
    {
        float value = stats->samples[i];
        sum += value;
        
        if (value < min_val) min_val = value;
        if (value > max_val) max_val = value;
    }
    
    stats->min_value = min_val;
    stats->max_value = max_val;
    stats->avg_value = sum / stats->sample_count;
    stats->stats_valid = 1;
}

/* :::::::::: Update demand calculation (sliding window maximum) :::::::: */
static void update_demand(rms_stats_t *stats, float new_value)
{
    // Add to demand window
    stats->demand_window[stats->demand_index] = new_value;
    stats->demand_index = (stats->demand_index + 1) % RMS_STATS_DEMAND_WINDOW;
    if (stats->demand_count < RMS_STATS_DEMAND_WINDOW) {
        stats->demand_count++;
    }
    
    // Find maximum in demand window
    if (stats->demand_count > 0)
    {
        float max_demand = stats->demand_window[0];
        for (uint16_t i = 1; i < stats->demand_count; i++)
        {
            if (stats->demand_window[i] > max_demand) {
                max_demand = stats->demand_window[i];
            }
        }
        stats->demand_value = max_demand;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End of the program ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
