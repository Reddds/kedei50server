#ifndef DL_CONTROLS_H
#define DL_CONTROLS_H

#include <stdint.h>

#include <stdbool.h>
/*
 * 1 - label within rectangle
 * 2 - text box
 * 
 * 102 - text box for finger
 * 
 * 10 - button
 * 11 - image button
 * 
 * 110 - button for finger
 * 111 - image button for finger
 * 
 * 20 - check box
 * 25 - radio button
 * 
 * 120 - check box for finger
 * 125 - radio button for finger
 *
 *	50 - static image
 *
 * */
typedef enum 
{
	CT_LABEL = 1,
	CT_TEXT_BOX = 2,
	
	CT_FINGER_TEXT_BOX = 102,
	
	
	CT_BUTTON = 10,
	CT_IMAGE_BUTTON = 11,
	
	CT_FINGER_BUTTON = 110,
	CT_FINGER_IMAGE_BUTTON = 111,
	
	
	CT_CHECK_BOX = 20,
	CT_RADIO = 25,
	
	CT_FINGER_CHECK_BOX = 120,
	CT_FINGER_RADIO = 125,

	CT_STATIC_IMAGE = 50,

	CT_PANEL = 200 // container!
}control_types;


typedef struct 
{
	uint16_t id;
	control_types type;
	uint16_t left;
	uint16_t top;
	uint16_t width;
	uint16_t height;
	// if control need memory it must use this pointers for alloc
	// for proper free
	void *control_data;
	void *control_data1;
	void *control_data2;
	void *control_data3;
	void *control_data4;
}dk_control;

void init_controls();
dk_control *find_control(uint16_t id);
dk_control *get_control_at(uint16_t pos);
int get_controls_count();
dk_control *add_control(uint16_t id, uint16_t parent_id, control_types type,
	uint16_t left, uint16_t top,
	uint16_t width, uint16_t height,
	void *control_data);
bool delete_control(uint16_t id);
bool delete_all_controls();

#endif
