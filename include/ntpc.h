/**
 * @file ntpc.h
 *
 * Network time protocol client.
 */

#pragma once

/**
 * Initialize NTP client.
 *
 * @param timezone Timezone name
 * @param daylight Daylight saving time flag
 * @see man 3 tzset
 */
void ntpc_init(const char *timezone, int daylight);

/**
 * Synchronize time with NTP server.
 *
 * @param timeout Timeout (s)
 * @return True if time is synchronized, false otherwise.
 */
bool ntpc_sync(uint32_t timeout);

/**
 * Convert RTC clock value to time in seconds.
 *
 * @param rtc_clk RTC slow clock (ticks)
 * @return Time in seconds.
 */
time_t ntpc_rtctime(uint64_t rtc_clk);
