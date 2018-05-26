#ifndef SATE_TIME_THREAD_H
#define SATE_TIME_THREAD_H

#include <stdint.h>
#include <stdbool.h>

#include "dk_controls.h"

#define MAX_SYS_CONTROLS 10

#define KNRM  "\x1B[0m"

#define KBOLD  	"\x1B[1m"
#define KITALIC "\x1B[3m"
#define KULINE  "\x1B[4m"
#define KSTRSTH "\x1B[9m"


#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

enum date_time_comb_tag
{
	DT_COMB_NONE = 0,
	DT_COMB_ONLY_TIME = 1,
	// time in time_id, date in date_id
	DT_COMB_TIME_AND_DATE = 2,
	DT_COMB_ONLY_DATE = 3,
	DT_COMB_TIME_BEFORE_DATE = 4,
	DT_COMB_DATE_BEFORE_TIME = 5
};

enum date_time_time_fmt_tag
{
//  5:05:46 PM
	DT_TM_II_0MM_0SS = 1,
//  17:05:46
	DT_TM_HH_0MM_0SS = 2,
	
//  5:05 PM
	DT_TM_II_0MM = 10,
//  17:05
	DT_TM_HH_0MM = 11
};

enum date_time_date_fmt_tag
{
// 1 	"12.04.2018"
	DT_DT_DD_MM_YYYY = 1,
// 2 	"12.04.18"
	DT_DT_DD_MM_YY = 2,

// 10    "12 апр 2018"
	DT_DT_DD_MMM_YYYY = 10,
// 11    "12 апр 18"
	DT_DT_DD_MMM_YY = 11,
// 12    "12 апреля 2018"
	DT_DT_DD_MMMM_YYYY = 12,
// 13    "12 апреля 18"
 	DT_DT_DD_MMMM_YY = 13,

// 20    "12 апр"
	DT_DT_DD_MMM = 20,
// 21    "12 апреля"
	DT_DT_DD_MMMM = 21,

// 30    "Четв. 12.04.2018"
	DT_DT_WWW_DD_MM_YYYY = 30,
// 31    "Четв. 12.04.18"
	DT_DT_WWW_DD_MM_YY = 31,
// 32    "Четв. 12 апр 2018"
	DT_DT_WWW_DD_MMM_YYYY = 32,
// 33    "Четв. 12 апр 18"
	DT_DT_WWW_DD_MMM_YY = 33,
// 34    "Четв. 12 апреля 2018"
	DT_DT_WWW_DD_MMMM_YYYY = 34,
// 35    "Четв. 12 апреля 18"
	DT_DT_WWW_DD_MMMM_YY = 35,

// 36    "Четв. 12 апр"
	DT_DT_WWW_DD_MMM = 36,
// 37    "Четв. 12 апреля"
	DT_DT_WWW_DD_MMMM = 37,
	

// 38    "Четверг, 12.04.2018"
	DT_DT_WWWW_DD_MM_YYYY = 38,
// 39    "Четверг, 12.04.18"
	DT_DT_WWWW_DD_MM_YY = 39,
// 40    "Четверг, 12 апр 2018"
	DT_DT_WWWW_DD_MMM_YYYY = 40,
// 41    "Четверг, 12 апр 18"
	DT_DT_WWWW_DD_MMM_YY = 41,
// 42    "Четверг, 12 апреля 2018"
	DT_DT_WWWW_DD_MMMM_YYYY = 42,
// 43    "Четверг, 12 апреля 18"
	DT_DT_WWWW_DD_MMMM_YY = 43,

// 44    "Четверг, 12 апр"
	DT_DT_WWWW_DD_MMM = 44,
// 45    "Четверг, 12 апреля"
	DT_DT_WWWW_DD_MMMM = 45,

};

void create_time_thread();
dk_control *local_set_text(uint16_t control_id, char *new_text);

#endif
