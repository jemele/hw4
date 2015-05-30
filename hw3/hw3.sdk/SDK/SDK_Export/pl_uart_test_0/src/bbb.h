// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
// The bbb/zed serial protocol definitions.
#ifndef _bbb_h_
#define _bbb_h_

#include <stdint.h>

// All fields are transmitted in host byte order.
typedef struct {

    // "Magically" identifies the protocol.
    uint8_t magic;
    // A version number, if we want to be forward thinking.
    uint8_t version;
    // The id of the encapsulated message.
    uint8_t id;
} bbb_header_t;

#define bbb_header_magic_value 0x13
#define bbb_header_version_value 0x37

// Valid message identifiers.
enum bbb_id {

    // Drive forward.
    // Please poll the sensor data when driving.
    // Returns ack when message completes.
    bbb_id_drive_straight   = 0,

    // Read and return the sensor data.
    bbb_id_sensor_read      = 1,

    // A sensor data report.
    bbb_id_sensor_data      = 2,

    // A general acknowledgement.
    // Used to indicate the command completed.
    bbb_id_ack              = 3,

    // Turn left (CCW) 90 degrees.
    bbb_id_rotate_left      = 4,

    // Turn right (CW) 90 degress.
    bbb_id_rotate_right     = 5,

    bbb_id_end,
    bbb_id_begin = bbb_id_drive_straight,
};

// All fields are transmitted in host byte order.
typedef struct {
    int16_t rate;
} bbb_id_drive_straight_t;

typedef struct {
    uint8_t bumper;
    uint8_t wall;
    int16_t rate;
    uint8_t direction;
    int32_t x;
    int32_t y;
} bbb_id_sensor_data_t;

#endif
