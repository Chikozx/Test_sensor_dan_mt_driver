#include <driver/mcpwm_prelude.h>

#define SENSOR1 34
#define SENSOR2 35
#define SENSOR3 32
#define SENSOR4 33

#define MOTORA_EN 18
#define MOTORA_IN1 17
#define MOTORA_IN2 16

#define MOTORB_EN 27
#define MOTORB_IN3 14
#define MOTORB_IN4 12

extern bool is_line_follow;
extern mcpwm_cmpr_handle_t comparator_1;
extern mcpwm_cmpr_handle_t comparator_2;