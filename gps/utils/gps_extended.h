/* Copyright (c) 2013-2017, 2020 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
Changes from Qualcomm Innovation Center are provided under the following license:

Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted (subject to the limitations in the
disclaimer below) provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef GPS_EXTENDED_H
#define GPS_EXTENDED_H

/**
 * @file
 * @brief C++ declarations for GPS types
 */

#include <gps_extended_c.h>
#if defined(USE_GLIB) || defined(OFF_TARGET)
#include <string.h>
#endif
#include <string>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



struct LocPosMode
{
    LocPositionMode mode;
    LocGpsPositionRecurrence recurrence;
    uint32_t min_interval;
    uint32_t preferred_accuracy;
    uint32_t preferred_time;
    bool share_position;
    char credentials[14];
    char provider[8];
    GnssPowerMode powerMode;
    uint32_t timeBetweenMeasurements;
    LocPosMode(LocPositionMode m, LocGpsPositionRecurrence recr,
               uint32_t gap, uint32_t accu, uint32_t time,
               bool sp, const char* cred, const char* prov,
               GnssPowerMode pMode = GNSS_POWER_MODE_INVALID,
               uint32_t tbm = 0) :
        mode(m), recurrence(recr),
        min_interval(gap < GPS_MIN_POSSIBLE_FIX_INTERVAL_MS ?
                     GPS_MIN_POSSIBLE_FIX_INTERVAL_MS : gap),
        preferred_accuracy(accu), preferred_time(time),
        share_position(sp), powerMode(pMode),
        timeBetweenMeasurements(tbm) {
        memset(credentials, 0, sizeof(credentials));
        memset(provider, 0, sizeof(provider));
        if (NULL != cred) {
            memcpy(credentials, cred, sizeof(credentials)-1);
        }
        if (NULL != prov) {
            memcpy(provider, prov, sizeof(provider)-1);
        }
    }

    inline LocPosMode() :
        mode(LOC_POSITION_MODE_MS_BASED),
        recurrence(LOC_GPS_POSITION_RECURRENCE_PERIODIC),
        min_interval(GPS_DEFAULT_FIX_INTERVAL_MS),
        preferred_accuracy(50), preferred_time(120000),
        share_position(true), powerMode(GNSS_POWER_MODE_INVALID),
        timeBetweenMeasurements(GPS_DEFAULT_FIX_INTERVAL_MS) {
        memset(credentials, 0, sizeof(credentials));
        memset(provider, 0, sizeof(provider));
    }

    inline bool equals(const LocPosMode &anotherMode) const
    {
        return anotherMode.mode == mode &&
            anotherMode.recurrence == recurrence &&
            anotherMode.min_interval == min_interval &&
            anotherMode.preferred_accuracy == preferred_accuracy &&
            anotherMode.preferred_time == preferred_time &&
            anotherMode.powerMode == powerMode &&
            anotherMode.timeBetweenMeasurements == timeBetweenMeasurements &&
            !strncmp(anotherMode.credentials, credentials, sizeof(credentials)-1) &&
            !strncmp(anotherMode.provider, provider, sizeof(provider)-1);
    }

    void logv() const;
};

/*
* Encapsulates the parameters (client name, preferred subscription ID and preferred APN
* for backhaul connect.
*/
struct BackhaulContext {
    std::string clientName;
    uint16_t prefSub;
    std::string prefApn;
    uint16_t prefIpType;
};

/* Engine Debug data Information */

#define GNSS_MAX_SV_INFO_LIST_SIZE 176

typedef struct {
    int32_t jammerInd;
    // Jammer Indication
    int32_t agc;
    // Automatic gain control
} GnssJammerData;


typedef struct {
    uint16_t gnssSvId;
    /**<   GNSS SV ID. Range:
      - GPS --     1 to 32
      - GLONASS -- 65 to 96
      - SBAS --    120 to 158 and 183 to 191
      - QZSS --    193 to 197
      - BDS --     201 to 263
      - Galileo -- 301 to 336
      - NavIC --   401 to 414 */

    uint8_t type;
    /**<   Navigation data type */

    uint8_t src;
    /**<   Navigation data source.*/
    int32_t age;
    /**<   Age of navigation data.
      - Units: Seconds */
} GnssNavDataInfo;


typedef struct {
    uint32_t hours;
    /**<   Hours of timestamp */

    uint32_t mins;
    /**<   Minutes of timestamp */

    float secs;
    /**<   Seconds of timestamp */
} GnssTimeInfo;

typedef enum {
    GPS_XTRA_VALID_BIT     = 1 << 0,
    GLONASS_XTRA_VALID_BIT = 1 << 1,
    BDS_XTRA_VALID_BIT     = 1 << 2,
    GAL_XTRA_VALID_BIT     = 1 << 3,
    QZSS_XTRA_VALID_BIT    = 1 << 4,
    NAVIC_XTRA_VALID_BIT   = 1 << 5
} GnssXtraValidMaskBits;

typedef struct {

    uint8_t timeValid;
    /**<Time validity >*/

    uint16_t gpsWeek;
    /**<   Full GPS week */

    uint32_t gpsTowMs;
    /**<   GPS time of week.
      - Units: milliseconds  */

    uint8_t sourceOfTime;
    /**<   Source of the time information*/

    float clkTimeUnc;
    /**<   Single-sided maximum time bias uncertainty.
      - Units: milliseconds */

    float clkFreqBias;
    /**<   Receiver clock frequency bias. \n
      - Units -- ppb */

    float clkFreqUnc;
    /**<   Receiver clock frequency uncertainty. \n
      - Units -- ppb */

    uint8_t xoState;
    /**<   XO calibration. */

    uint32_t rcvrErrRecovery;
    /**<   Error recovery reason. */

    Gnss_LeapSecondInfoStructType leapSecondInfo;
    /**<   Leap second information. */

    std::vector<GnssJammerData> jammerData;
    /**<   Jammer indicator of each signal. */

    uint64_t jammedSignalsMask;
    /* Jammer Signal Mask */

    GnssTimeInfo epiTime;
    /**<   UTC Time associated with EPI. */

    uint8_t epiValidity;
    /**<   Epi validity >*/

    float epiLat;
    /**<   EPI Latitude. - Units: Radians
        valid if 0th bit set in epiValidity*/

    float epiLon;
    /**<   EPI Longitude. - Units: Radians
        valid if 0th bit set in epiValidity*/

    float epiAlt;
    /**<   EPI Altitude. - Units: Meters
        valid if 1st bit set in epiValidity*/

    float epiHepe;
    /**<   EPI Horizontal Estimated Position Error.
      - Units: Meters
        valid if 0th bit set in epiValidity*/

    float epiAltUnc;
    /**<   EPI Altitude Uncertainty.
      - Units: Meters
        valid if 1st bit set in epiValidity*/

    uint8_t epiSrc;
    /**<   EPI Source
        valid if 2nd bit set in epiValidity*/

    GnssTimeInfo bestPosTime;
    /**<   UTC Time associated with Best Position. */

    float bestPosLat;
    /**<   Best Position Latitude.
      - Units: Radians */

    float bestPosLon;
    /**<   Best Position Longitude.
      - Units: Radians */

    float bestPosAlt;
    /**<   Best Position Altitude.
      - Units: Meters */

    float bestPosHepe;
    /**<   Best Position Horizontal Estimated Position Error.
      - Units: Meters */

    float bestPosAltUnc;
    /**<   Best Position Altitude Uncertainty.
      - Units: Meters */

    GnssTimeInfo xtraInfoTime;
    /**<   UTC time when XTRA debug information was generated. */

    uint8_t xtraValidMask;
    /**<xtra valid mask>*/

    uint32_t gpsXtraAge;
    /**<   Age of GPS XTRA data.
      - Units: Seconds */

    uint32_t gloXtraAge;
    /**<   Age of GLONASS XTRA data.
      - Units: Seconds */

    uint32_t bdsXtraAge;
    /**<   Age of BDS XTRA data.
      - Units: Seconds */

    uint32_t galXtraAge;
    /**<   Age of GAL XTRA data.
      - Units: Seconds */

    uint32_t qzssXtraAge;
    /**<   Age of QZSS XTRA data.
      - Units: Seconds */

    uint32_t navicXtraAge;
    /**<   Age of NAVIC XTRA data.
      - Units: Seconds */

    uint32_t gpsXtraMask;
    /**<   Specifies the GPS SV mask.
      - SV ID mapping: SV 1 maps to bit 0. */

    uint32_t gloXtraMask;
    /**<   Specifies the GLONASS SV mask.
      - SV ID mapping: SV 65 maps to bit 0. */

    uint64_t bdsXtraMask;
    /**<   Specifies the BDS SV mask.
      - SV ID mapping: SV 201 maps to bit 0. */

    uint64_t galXtraMask;
    /**<   Specifies the Galileo SV mask.
      - SV ID mapping: SV 301 maps to bit 0. */

    uint8_t qzssXtraMask;
    /**<   Specifies the QZSS SV mask.
      - SV ID mapping: SV 193 maps to bit 0 */

    uint32_t navicXtraMask;
    /**<   Specifies the NAVIC SV mask.
      - SV ID mapping: SV 401 maps to bits 0. */

    GnssTimeInfo ephInfoTime;
    /**<   UTC time when ephemeris debug information was generated. */

    uint32_t gpsEphMask;
    /**<   Specifies the GPS SV mask.
      - SV ID mapping: SV 1 maps to bit 0. */

    uint32_t gloEphMask;
    /**<   Specifies the GLONASS SV mask.
      - SV ID mapping: SV 65 maps to bit 0. */

    uint64_t bdsEphMask;
    /**<   Specifies the BDS SV mask.
      - SV ID mapping: SV 201 maps to bit 0. */

    uint64_t galEphMask;
    /**<   Specifies the Galileo SV mask.
      - SV ID mapping: SV 301 maps to bit 0. */

    uint8_t qzssEphMask;
    /**<   Specifies the QZSS SV mask.
      - SV ID mapping: SV 193 maps to bit 0 */

    uint32_t navicEphMask;
    /**<   Specifies the NAVIC SV mask.
      - SV ID mapping: SV 401 maps to bits 0. */

    GnssTimeInfo healthInfoTime;
    /**<   UTC time when SV health information was generated. */

    uint32_t gpsHealthUnknownMask;
    /**<   Specifies the GPS SV mask.
      - SV ID mapping: SV 1 maps to bit 0. */

    uint32_t gloHealthUnknownMask;
    /**<   Specifies the GLONASS SV mask.
      - SV ID mapping: SV 65 maps to bit 0. */

    uint64_t bdsHealthUnknownMask;
    /**<   Specifies the BDS SV mask.
      - SV ID mapping: SV 201 maps to bit 0. */

    uint64_t galHealthUnknownMask;
    /**<   Specifies the Galileo SV mask.
      - SV ID mapping: SV 301 maps to bit 0. */

    uint8_t qzssHealthUnknownMask;
    /**<   Specifies the QZSS SV mask.
      - SV ID mapping: SV 193 maps to bit 0 */

    uint32_t navicHealthUnknownMask;
    /**<   Specifies the NAVIC SV mask.
      - SV ID mapping: SV 401 maps to bits 0. */

    uint32_t gpsHealthGoodMask;
    /**<   Specifies the GPS SV mask.
      - SV ID mapping: SV 1 maps to bit 0. */

    uint32_t gloHealthGoodMask;
    /**<   Specifies the GLONASS SV mask.
      - SV ID mapping: SV 65 maps to bit 0. */

    uint64_t bdsHealthGoodMask;
    /**<   Specifies the BDS SV mask.
      - SV ID mapping: SV 201 maps to bit 0. */

    uint64_t galHealthGoodMask;
    /**<   Specifies the Galileo SV mask.
      - SV ID mapping: SV 301 maps to bit 0. */

    uint8_t qzssHealthGoodMask;
    /**<   Specifies the QZSS SV mask.
      - SV ID mapping: SV 193 maps to bit 0 */

    uint32_t navicHealthGoodMask;
    /**<   Specifies the NAVIC SV mask.
      - SV ID mapping: SV 401 maps to bits 0. */

    uint32_t gpsHealthBadMask;
    /**<   Specifies the GPS SV mask.
      - SV ID mapping: SV 1 maps to bit 0. */

    uint32_t gloHealthBadMask;
    /**<   Specifies the GLONASS SV mask.
      - SV ID mapping: SV 65 maps to bit 0. */

    uint64_t bdsHealthBadMask;
    /**<   Specifies the BDS SV mask.
      - SV ID mapping: SV 201 maps to bit 0. */

    uint64_t galHealthBadMask;
    /**<   Specifies the Galileo SV mask.
      - SV ID mapping: SV 301 maps to bit 0. */

    uint8_t qzssHealthBadMask;
    /**<   Specifies the QZSS SV mask.
      - SV ID mapping: SV 193 maps to bit 0 */

    uint32_t navicHealthBadMask;
    /**<   Specifies the NAVIC SV mask.
      - SV ID mapping: SV 401 maps to bits 0. */

    GnssTimeInfo fixInfoTime;
    /**<   UTC time when fix information was generated. */

    uint32_t fixInfoMask;
    /**<   Fix Information Mask*/

    GnssTimeInfo navDataTime;
    /**<   UTC time when navigation data was generated. */

    GnssNavDataInfo navData[GNSS_MAX_SV_INFO_LIST_SIZE];
    /**<   Satellite navigation data. */

    GnssTimeInfo fixStatusTime;
    /**<   UTC time when fix status was generated. */

    uint32_t fixStatusMask;
    /**<   Fix Status Mask */

    uint32_t fixHepeLimit;
    /**<   Session HEPE Limit.
      - Units: Meters */
} GnssEngineDebugDataInfo;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GPS_EXTENDED_H */

