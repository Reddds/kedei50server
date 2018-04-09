#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "date_time_thread.h"

void* doTimeShow(void *arg);
//void* delegate_set_text(dk_control *control, char *new_text);
//void (*delegate_set_text)(dk_control *control, char *new_text);

pthread_t tid;

extern pthread_mutex_t lock_draw;

enum date_time_comb_tag date_time_comb = DT_COMB_NONE;
enum date_time_time_fmt_tag date_time_time_fmt = DT_TM_HH_0MM;
enum date_time_date_fmt_tag date_time_date_fmt = DT_DT_DD_MM_YYYY;
bool need_change_time_format = false;

dk_control *time_control = NULL;
dk_control *date_control = NULL;

bool dt_show_seconds = false;

char *time_format_str = NULL;
char *date_format_str = NULL;
char date_time_format_str[50];

void create_time_thread()
{
	int err = pthread_create(&tid, NULL, &doTimeShow, NULL);
    if (err != 0)
        printf("\ncan't create thread :[%s]", strerror(err));
    else
        printf("\n Thread created successfully\n");
}

void get_date_time_format_str()
{
	switch(date_time_time_fmt)
	{
		//  17:05:46 
		case DT_TM_HH_0MM_0SS:
			time_format_str = "%H:%M:%S";
			dt_show_seconds = true;
			break;
		//  5:05:46 PM
		case DT_TM_II_0MM_0SS:
			time_format_str = "%I:%M:%S %p";
			dt_show_seconds = true;
			break;
			
		//  17:05
		case DT_TM_HH_0MM:
			time_format_str = "%H:%M";
			dt_show_seconds = false;
			break;
		//  5:05 PM
		case DT_TM_II_0MM:
			time_format_str = "%I:%M %p";
			dt_show_seconds = false;
			break;
	}

	switch(date_time_date_fmt)
	{
	// 1 	"12.04.2018"
		case DT_DT_DD_MM_YYYY:
			date_format_str = "%d.%m.%Y";
			break;
	// 2 	"12.04.18"
		case DT_DT_DD_MM_YY:
			date_format_str = "%d.%m.%y";
			break;

	// 10    "12 апр 2018"
		case DT_DT_DD_MMM_YYYY:
			date_format_str = "%d %b %Y";
			break;
	// 11    "12 апр 18"
		case DT_DT_DD_MMM_YY:
			date_format_str = "%d %b %y";
			break;
	// 12    "12 апреля 2018"
		case DT_DT_DD_MMMM_YYYY:
			date_format_str = "%d %B %Y";
			break;
	// 13    "12 апреля 18"
		case DT_DT_DD_MMMM_YY:
			date_format_str = "%d %B %y";
			break;

	// 20    "12 апр"
		case DT_DT_DD_MMM:
			date_format_str = "%d %b";
			break;
	// 21    "12 апреля"
		case DT_DT_DD_MMMM:
			date_format_str = "%d %B";
			break;

	// 30    "Четв. 12.04.2018"
		case DT_DT_WWW_DD_MM_YYYY:
			date_format_str = "%a. %d.%m.%Y";
			break;
	// 31    "Четв. 12.04.18"
		case DT_DT_WWW_DD_MM_YY:
			date_format_str = "%a. %d.%m.%y";
			break;
	// 32    "Четв. 12 апр 2018"
		case DT_DT_WWW_DD_MMM_YYYY:
			date_format_str = "%a. %d %b %Y";
			break;
	// 33    "Четв. 12 апр 18"
		case DT_DT_WWW_DD_MMM_YY:
			date_format_str = "%a. %d %b %y";
			break;
	// 34    "Четв. 12 апреля 2018"
		case DT_DT_WWW_DD_MMMM_YYYY:
			date_format_str = "%a. %d %B %Y";
			break;
	// 35    "Четв. 12 апреля 18"
		case DT_DT_WWW_DD_MMMM_YY:
			date_format_str = "%a. %d %B %y";
			break;

	// 36    "Четв. 12 апр"
		case DT_DT_WWW_DD_MMM:
			date_format_str = "%a. %d %b";
			break;
	// 37    "Четв. 12 апреля"
		case DT_DT_WWW_DD_MMMM:
			date_format_str = "%a. %d %B";
			break;
		

	// 38    "Четверг, 12.04.2018"
		case DT_DT_WWWW_DD_MM_YYYY:
			date_format_str = "%A, %d.%m.%Y";
			break;
	// 39    "Четверг, 12.04.18"
		case DT_DT_WWWW_DD_MM_YY:
			date_format_str = "%A, %d.%m.%y";
			break;
	// 40    "Четверг, 12 апр 2018"
		case DT_DT_WWWW_DD_MMM_YYYY:
			date_format_str = "%A, %d %b %Y";
			break;
	// 41    "Четверг, 12 апр 18"
		case DT_DT_WWWW_DD_MMM_YY:
			date_format_str = "%A, %d %b %y";
			break;
	// 42    "Четверг, 12 апреля 2018"
		case DT_DT_WWWW_DD_MMMM_YYYY:
			date_format_str = "%A, %d %B %Y";
			break;
	// 43    "Четверг, 12 апреля 18"
		case DT_DT_WWWW_DD_MMMM_YY:
			date_format_str = "%A, %d %B %y";
			break;

	// 44    "Четверг, 12 апр"
		case DT_DT_WWWW_DD_MMM:
			date_format_str = "%A, %d %b";
			break;
	// 45    "Четверг, 12 апреля"
		case DT_DT_WWWW_DD_MMMM:
			date_format_str = "%A, %d %B";
			break;

	}
	switch(date_time_comb)
	{
		case DT_COMB_TIME_BEFORE_DATE:
			sprintf(date_time_format_str, "%s %s", time_format_str, date_format_str);
			break;
		case DT_COMB_DATE_BEFORE_TIME:
			sprintf(date_time_format_str, "%s %s", date_format_str, time_format_str);
			break;
		default:
			break;
	}

	printf("Time format: '%s', date format: %s\n", time_format_str, date_format_str);
}
// time thread
void* doTimeShow(void *arg)
{
	
	//dk_control *time_control = (dk_control *)arg;
    unsigned long i = 0;
    //pthread_t id = pthread_self();

//    if(pthread_equal(id,tid[0]))
//    {
//        printf("\n Time thread start. Control id = %u\n", time_control->id);
//    }
//    else
//    {
//        printf("\n Second thread processing\n");
//    }
	char dt_buf[100];
	//buf[8] = 0;
	uint8_t last_second = 100;
	uint8_t last_minute = 100;
	uint8_t last_day = 100;

	//locale_t locale = newlocale(LC_TIME_MASK, const char * locale, locale_t base);
	
	while(true)//!!!!!!
	{
		usleep(200000);
		if(need_change_time_format)
		{
			need_change_time_format = false;
			get_date_time_format_str();
			last_minute = 100;
			last_day = 100;			
		}
		if(time_control == NULL && date_control == NULL)
			continue;
		time_t t = time(NULL);
		struct tm *tm = localtime(&t);

		if(last_second == tm->tm_sec)
			continue;
		last_second = tm->tm_sec;
		// If not need to show seconds
		// Show time on minute change
		if(!dt_show_seconds && tm->tm_min == last_minute)
		{
			continue;
		}
		last_minute = tm->tm_min;
		
		switch(date_time_comb)
		{
			case DT_COMB_NONE:
				continue;
			case DT_COMB_ONLY_TIME:
				if(time_control == NULL)
					continue;
				strftime(dt_buf, sizeof(dt_buf), time_format_str, tm);
				pthread_mutex_lock(&lock_draw);
				local_set_text(time_control, dt_buf);
				pthread_mutex_unlock(&lock_draw);
				break;
			// time in time_id, date in date_id
			case DT_COMB_TIME_AND_DATE:
				if(time_control == NULL || date_control == NULL)
					continue;
				pthread_mutex_lock(&lock_draw);
				strftime(dt_buf, sizeof(dt_buf), time_format_str, tm);
				local_set_text(time_control, dt_buf);
				if(last_day != tm->tm_mday)
				{
					int stres = strftime(dt_buf, sizeof(dt_buf), date_format_str, tm);
					if(stres <= 0)
					{
						printf("Date string len = 0\n");
						perror("Error Date string");
					}
					else
					{
						printf("date = %s\n", dt_buf);
						local_set_text(date_control, dt_buf);
					}
				}
				pthread_mutex_unlock(&lock_draw);
				last_day = tm->tm_mday;
				break;
			case DT_COMB_ONLY_DATE:
				if(date_control == NULL)
					continue;
				if(last_day == tm->tm_mday)
					continue;
				last_day = tm->tm_mday;
				strftime(dt_buf, sizeof(dt_buf), date_format_str, tm);
				pthread_mutex_lock(&lock_draw);
				local_set_text(date_control, dt_buf);
				pthread_mutex_unlock(&lock_draw);
				break;
			case DT_COMB_TIME_BEFORE_DATE:
			case DT_COMB_DATE_BEFORE_TIME:
				if(time_control == NULL)
					continue;
				strftime(dt_buf, sizeof(dt_buf), date_time_format_str, tm);
				pthread_mutex_lock(&lock_draw);
				local_set_text(time_control, dt_buf);
				pthread_mutex_unlock(&lock_draw);
				last_day = tm->tm_mday;
				break;
		}
		i++;
	}
	printf("Exit time thread\n");
    return NULL;
}

