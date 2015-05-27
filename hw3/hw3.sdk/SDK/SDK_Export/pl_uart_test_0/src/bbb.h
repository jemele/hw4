// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
// The bbb/zed serial protocol definitions.
#ifndef _bbb_h_
#define _bbb_h_

// All fields are transmitted in network byte order.
typedef struct {

    // "Magically" identifies the protocol.
    u8 magic;
    // A version number, if we want to be forward thinking.
    u8 version;
    // The id of the encapsulated message.
    u8 id;
} bbb_header_t;

#define bbb_header_magic_value 0x42
#define bbb_header_version_value 0

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
};

// All fields are transmitted in network byte order.
typedef struct {
    s16 rate;
} bbb_id_drive_straight_t;

typedef struct {
    u8 bumper;
    u8 wall;
} bbb_id_sensor_data_t;

#endif
