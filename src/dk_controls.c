#include <stdlib.h>
#include <stdio.h>
#include "dk_controls.h"

typedef struct {
  dk_control *array;
  size_t used;
  size_t size;
} ControlArray;

ControlArray root_controls;

void initArray(ControlArray *a, size_t initialSize)
{
	a->array = (dk_control *)malloc(initialSize * sizeof(dk_control));
	a->used = 0;
	a->size = initialSize;
}

dk_control *insertArray(ControlArray *a, dk_control element) {
	// a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
	// Therefore a->used can go up to a->size 
	if (a->used == a->size)
	{
		a->size *= 2;
		a->array = (dk_control *)realloc(a->array, a->size * sizeof(dk_control));
	}
	a->array[a->used++] = element;
	return &a->array[a->used - 1];
}


void free_control_mem(dk_control *control)
{
	if(control->control_data != NULL)
		free(control->control_data);
	if(control->control_data1 != NULL)
		free(control->control_data1);
	if(control->control_data2 != NULL)
		free(control->control_data2);
	if(control->control_data3 != NULL)
		free(control->control_data3);
	if(control->control_data4 != NULL)
		free(control->control_data4);
}

void clearArray(ControlArray *a)
{
	for(int i = 0; i < a->used; i++)
	{
		free_control_mem(&a->array[i]);
	}
	a->used = 0;
}

void freeArray(ControlArray *a)
{
	clearArray(a);
	free(a->array);
	a->array = NULL;
	a->used = a->size = 0;
}


void deleteInArray(ControlArray *a, int pos)
{
	if(pos >= a->used)
		return;
	free_control_mem(&a->array[pos]);
	// shift to gap
	for(;pos < a->used - 1; pos++)
		a->array[pos] = a->array[pos - 1];
	a->used--;
	
}

dk_control *findArray(ControlArray *a, uint16_t id)
{
	for(int i = 0; i < a->used; i++)
	{
		if(a->array[i].id == id)
			return &a->array[i];
	}
	return NULL;
}

dk_control *findArrayByParent(ControlArray *a, uint16_t parent_id)
{
	for(int i = 0; i < a->used; i++)
	{
		if(a->array[i].parent_id == parent_id)
			return &a->array[i];
	}
	return NULL;
}

bool isPointInControl(dk_control *control, uint16_t x, uint16_t y)
{
	control_position_t abs_pos = get_abs_control_pos(control);
	return abs_pos.left <= x && abs_pos.left + abs_pos.width > x
			&& abs_pos.top <= y && abs_pos.top + abs_pos.height > y;
}



dk_control *findArrayByPoint(ControlArray *a, dk_control *control,
	uint16_t x, uint16_t y)
{
	// find first control
	//dk_control *control = NULL;
	if(control == NULL)
	{
		for(int i = 0; i < a->used; i++)
		{
			if(a->array[i].visible && isPointInControl(&a->array[i], x, y))
			{
				control = &a->array[i];
				break;
			}
		}
		if(control == NULL)
			return NULL;
	}

	for(int i = 0; i < a->used; i++)
	{
		if(a->array[i].visible && a->array[i].parent_id == control->id
			&& isPointInControl(&a->array[i], x, y))
		{
			return findArrayByPoint(a, &a->array[i], x, y);
		}
	}
	return control;
}

int findPosInArray(ControlArray *a, uint16_t id)
{
	for(int i = 0; i < a->used; i++)
	{
		if(a->array[i].id == id)
			return i;
	}
	return -1;
}

dk_control *find_control(uint16_t id)
{
	return(findArray(&root_controls, id));
}

int get_controls_count()
{
	return root_controls.used;
}

dk_control *get_control_at(uint16_t pos)
{
	if(pos >= root_controls.used)
		return NULL;
	return &root_controls.array[pos];
}

void init_controls()
{
	initArray(&root_controls, 10);
}

dk_control *add_control(uint16_t id, uint16_t parent_id, control_types type,
	uint16_t left, uint16_t top,
	uint16_t width, uint16_t height,
	bool visible,
	void *control_data)
{
	printf("Adding control 111\n");
	if(id == 0)
	{
		printf("Control with id = 0 is 'desktop'!\n");
		free(control_data);
		return NULL;
	}
	
	if(find_control(id) != NULL)
	{
		printf("Control with id = %d already exist!\n", id);
		free(control_data);
		return NULL;
	}

	dk_control *parent_control = NULL;
	if(parent_id > 0)
	{
		parent_control = find_control(parent_id);
		if(parent_control == NULL)
		{
			printf("Parent control with id = %d not exist!\n", parent_id);
			free(control_data);
			return NULL;			
		}
		if(parent_control->type != CT_PANEL)
		{
			printf("Parent control is not container!\n");
			free(control_data);
			return NULL;						
		}
	}
	printf("Parent OK\n");
	dk_control control;	
	control.id = id;
	control.parent_id = parent_id;
	control.type = type;
	control.left = left;
	control.top = top;
	control.width = width;
	control.height = height;
	control.visible = visible;
	control.control_data = control_data;
	control.control_data1 = NULL;
	control.control_data2 = NULL;
	control.control_data3 = NULL;
	control.control_data4 = NULL;

	return insertArray(&root_controls, control);
}

bool delete_control(uint16_t id)
{
	if(id == 0)
		return false;
	int pos = findPosInArray(&root_controls, id);
	if(pos < 0)
		return false;
	deleteInArray(&root_controls, pos);
	return true;
}

bool delete_all_controls()
{
	clearArray(&root_controls);
	return true;
}

control_position_t get_abs_control_pos(dk_control *control)
{
	control_position_t abs_pos;
	abs_pos.left = control->left;
	abs_pos.top = control->top;
	abs_pos.width = control->width;
	abs_pos.height = control->height;

	while(control->parent_id > 0)
	{
		control = find_control(control->parent_id);
		if(control == NULL)
		{
			printf("Parent error!\n");
			break;
		}
		abs_pos.left += control->left;
		abs_pos.top += control->top;
	}
	return abs_pos;
}

dk_control *find_control_by_point(uint16_t x, uint16_t y)
{
	return findArrayByPoint(&root_controls, NULL, x, y);
}
