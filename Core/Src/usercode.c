/*
   Copyright 2023 R212Warden

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <stdint.h>
#include <string.h>
#include "../st7735/st7735.h"
#include "../st7735/fonts.h"
#include "usercode.h"
#include "main.h"
#include "cmsis_os.h"


volatile int32_t btn_debounce[5];
volatile int32_t btn_state[5];
const int32_t btn_tresh = 4;

int32_t charging_finished = 0;
int32_t scope_recording[420];
int32_t counter, scope_ready;

#define TIMESCALE 18
#define TIMESCALE_PRESCALER_CHANGE 11 //more than 11
#define TIMESCALE_SAMPLING_RATE_CHANGE 7 //less than this is done using continuous ADC
#define TIMESCALE_MIN 2

const char timedivs[5 * TIMESCALE] = {
	'5', 'u', ' ', ' ', 0,
	'1', '0', 'u', ' ', 0,
	'2', '0', 'u', ' ', 0,
	'5', '0', 'u', ' ', 0,
	'1', '0', '0', 'u', 0,
	'2', '0', '0', 'u', 0,
	'5', '0', '0', 'u', 0,
	'1', 'm', ' ', ' ', 0,
	'2', 'm', ' ', ' ', 0,
	'5', 'm', ' ', ' ', 0,
	'1', '0', 'm', ' ', 0,
	'2', '0', 'm', ' ', 0,
	'5', '0', 'm', ' ', 0,
	'1', '0', '0', 'm', 0,
	'2', '0', '0', 'm', 0,
	'5', '0', '0', 'm', 0,
	'1', 's', ' ', ' ', 0,
	'2', 's', ' ', ' ', 0
};


const uint32_t samplingtime[TIMESCALE_SAMPLING_RATE_CHANGE] = {
	0,
	0,
	ADC_SAMPLETIME_1CYCLE_5,
	ADC_SAMPLETIME_7CYCLES_5,
	ADC_SAMPLETIME_13CYCLES_5,
	ADC_SAMPLETIME_55CYCLES_5,
	ADC_SAMPLETIME_239CYCLES_5
};


const float timescale_scale[TIMESCALE_SAMPLING_RATE_CHANGE] = {
	0,
	0,
	0.5000,
	0.8750,
	1.3462,
	1.0294,
	0.6944
};


const int32_t autoreloads[TIMESCALE] = {
	7,
	14,
	28,
	70,
	280,
	280,
	700,
	1400,
	2800,
	7000,
	14000,
	28000,
	1440,
	2800,
	5600,
	14000,
	28000,
	56000
};

void mainTaskFunction(void const * argument)
{
	/* USER CODE BEGIN 5 */
	/* Infinite loop */
	char buffer[16];
	int32_t compare = 1;
	float freq = 1, capacitance = 1, capacitance_lpf = 10;
	int32_t freq_div = 4, capacitance_disp, capacitance_disp_prev;
	int32_t perc_disp, perc_prev;

	int32_t timer_significant_change = 0, timer_overflows = 0;
	int32_t timer_previous, timer_tresh = 100, compare_lovval = 0;

	int32_t mode_selector = 0;
	float voltage = 1, voltage_lpf = 1;
	float battery_perc;
	int32_t adcr = 0, voltage_disp, voltage_disp_prev, voltage_prev;
	int32_t timeDiv = 10;
	int32_t setScopeParams = 1;
	int32_t setADCchargingChannel = 1;
	int32_t setADCvoltageProbeChannel = 0;

	getSupplyVoltageCounter = 0xFFFF; //trigger the calibration immediately

	for (;;) {

		compare = __HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_2);
		if (compare < 100) compare = 0;
		freq = ((56000000 / freq_div) / (float) compare);
		if (freq > 250000) freq = 500000;

		capacitance = (1440000 / 2) / ((470 + 1700) * freq);
		capacitance_lpf -= capacitance_lpf / 32;
		capacitance_lpf += capacitance / 32;

		switch (mode_selector) {
		case 0: //CAPACITANCE
			if ((compare > (timer_previous + timer_tresh)) || (compare < (timer_previous - timer_tresh))) {
				capacitance_lpf = capacitance; //antilag
			}
			timer_previous = compare;


			if (__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_UPDATE)) { //overflow detected
				timer_overflows++;
				compare_lovval = 0;
			}
			__HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);

			if (timer_overflows > 6) { //timer is overflowing, change psc
				TIM3->PSC = 1023;
				freq_div = 1024;
				timer_overflows = 0;
			}

			if (compare < 100) {
				compare_lovval++;
			}

			if (compare_lovval > 6) {//low value for a while crocodile
				TIM3->PSC = 3;
				freq_div = 4;
				compare_lovval = 0;
			}

			capacitance_disp = (int32_t) (capacitance_lpf * 1000);

			ST7735_WriteString(27, 0, "NANOFARADS", Font_11x18, ST7735_YELLOW, ST7735_BLACK);

			if (capacitance_disp != capacitance_disp_prev) {

				ST7735_FillRectangleFast(0, 30, 160, 26, ST7735_BLACK);

				itoa(capacitance_disp, buffer);
				//sprintf(buffer, "%d nF    ", capacitance_disp);

				int32_t middle = ((160 - 10 * user_strlen(buffer)) / 2) - 8;
				ST7735_WriteString(middle, 30, buffer, Font_16x26, ST7735_CYAN, ST7735_BLACK);
				capacitance_disp_prev = capacitance_disp;

			}
			break;
		case 1: //voltage
			ST7735_WriteString(30, 0, "MILIVOLTS", Font_11x18, ST7735_YELLOW, ST7735_BLACK);

			//HAL_ADC_Start(&hadc1);
			// int status = HAL_ADC_PollForConversion(&hadc1, 1000);
			HAL_ADC_PollForConversion(&hadc1, 10);
			adcr = HAL_ADC_GetValue(&hadc1);

			voltage = (float) adcr * ((adc_voltage / 4096.0)) * 1000;

			if ((voltage > (voltage_prev + 70)) || (voltage < (voltage_prev - 70))) {
				voltage_lpf = voltage; //antilag
			}
			voltage_prev = voltage * 1000;

			voltage_lpf -= voltage_lpf / 8;
			voltage_lpf += voltage / 8;

			voltage_disp = voltage_lpf;

			if (voltage_disp != voltage_disp_prev) {

				ST7735_FillRectangleFast(0, 30, 160, 26, ST7735_BLACK);
				//sprintf(buffer, "%d mV    ", voltage_disp);
				itoa(voltage_disp, buffer);

				int32_t middle = ((160 - 10 * user_strlen(buffer)) / 2) - 8;
				ST7735_WriteString(middle, 30, buffer, Font_16x26, ST7735_CYAN, ST7735_BLACK);
				voltage_disp_prev = voltage_disp;
			}

			break;

		case 2: //scope

			ST7735_WriteString(52, 0, "SCOPE", Font_11x18, ST7735_YELLOW, ST7735_BLACK);
			//sprintf(buffer, "%d  ", (int32_t)(timescale_scale[timeDiv]*1000));
			//ST7735_WriteString(0, 0, buffer, Font_11x18, ST7735_CYAN, ST7735_BLACK);
			ST7735_WriteString(0, 0, timedivs + 5 * timeDiv, Font_11x18, ST7735_CYAN, ST7735_BLACK);


			if (setScopeParams) {
				__HAL_TIM_SET_AUTORELOAD(&htim1, autoreloads[timeDiv]);

				if (timeDiv > TIMESCALE_PRESCALER_CHANGE)
					__HAL_TIM_SET_PRESCALER(&htim1, 49);
				else
					__HAL_TIM_SET_PRESCALER(&htim1, 0);

				if (timeDiv < TIMESCALE_SAMPLING_RATE_CHANGE) {
					MX_ADC1_myInit(ENABLE, samplingtime[timeDiv], ADC_SOFTWARE_START, ADC_CHANNEL_9);
				} else {
					MX_ADC1_myInit(DISABLE, ADC_SAMPLETIME_239CYCLES_5, ADC_EXTERNALTRIGCONV_T1_CC1, ADC_CHANNEL_9);
				}
				
				setScopeParams--;

				if (setScopeParams < 0) setScopeParams = 0;
			}
			int32_t *processedGraph;
			if (scope_ready) {
				//int32_t *triggerWaveform(int len, int maxlen, int *graph, int level, int time);
				if (timeDiv < TIMESCALE_SAMPLING_RATE_CHANGE)
					processedGraph = triggerWaveform(160, 420, scope_recording, 0xFFFF, 40.0*timescale_scale[timeDiv]);
				else
					processedGraph = triggerWaveform(160, 420, scope_recording, 0xFFFF, 40);	

				ST7735_FillRectangleFast(0, 20, 160, 60, ST7735_BLACK);
				ST7735_FillRectangle(40, 20, 1, 60, ST7735_RED);
				ST7735_FillRectangle(80, 20, 1, 60, ST7735_RED);
				ST7735_FillRectangle(120, 20, 1, 60, ST7735_RED);
				ST7735_FillRectangle(0, 50, 160, 1, ST7735_RED);

				float pixel;
				for (int i = 0; i < 160; i++) {
					
					if (timeDiv < TIMESCALE_SAMPLING_RATE_CHANGE){
						pixel = timescale_scale[timeDiv] * (float)i;
						if (pixel > 420) break;
						voltage = (float) processedGraph[(int)pixel] * 0.0145;
						
					}else{
						voltage = (float) processedGraph[i] * 0.0145;
					}
					
					//voltage = (float) processedGraph[i] * 0.0145;
					//voltage = (float)scope_recording[(int32_t)pixel] * 0.015; 
					ST7735_DrawPixel(i, 79 - (int32_t) voltage, ST7735_WHITE);
				}
				scope_ready = 0;
			}

			if (HAL_DMA_GetState(&hdma_adc1) == HAL_DMA_STATE_READY) {
				HAL_ADC_Start_DMA(&hadc1, scope_recording, 420);
			}

			break;
		case 3: //charging
			ST7735_WriteString(36, 0, "CHARGING", Font_11x18, ST7735_YELLOW, ST7735_BLACK);

			HAL_ADC_PollForConversion(&hadc1, 0);
			adcr = HAL_ADC_GetValue(&hadc1);

			voltage = (float) adcr * ((adc_voltage / 4096.0)) * 1000 * 2.0048;
			battery_perc = voltage_lpf * 0.083333 - 250;

			if (setADCchargingChannel) {
				voltage_lpf = voltage;
				setADCchargingChannel = 0;
			} else {
				voltage_lpf -= voltage_lpf / 16;
				voltage_lpf += voltage / 16;
			}


			int charge_enable_Pin = 1 << 17;

			if ((charging_finished == 0) && (battery_perc < 95) && !HAL_GPIO_ReadPin(GPIOA, charge_enable_Pin) && !HAL_GPIO_ReadPin(GPIOA, in_plugged_Pin)) {
				HAL_GPIO_WritePin(GPIOA, charge_enable_Pin, 1);
				ST7735_FillRectangleFast(0, 60, 160, 18, ST7735_BLACK);
			} else if ((battery_perc > 98) & !HAL_GPIO_ReadPin(GPIOA, in_plugged_Pin)) {
				HAL_GPIO_WritePin(GPIOA, charge_enable_Pin, 0);
				charging_finished = 1;
				ST7735_FillRectangleFast(0, 60, 160, 18, ST7735_BLACK);
				ST7735_WriteString(30, 60, "Finished!", Font_11x18, ST7735_GREEN, ST7735_BLACK);
			} else if (HAL_GPIO_ReadPin(GPIOA, in_plugged_Pin)) {
				HAL_GPIO_WritePin(GPIOA, charge_enable_Pin, 0);
				charging_finished = 0;
				ST7735_FillRectangleFast(0, 60, 160, 18, ST7735_BLACK);
				ST7735_WriteString(25, 60, "Unplugged!", Font_11x18, ST7735_RED, ST7735_BLACK);
			}



			voltage_disp = voltage_lpf;
			//voltage_disp = adc_voltage*1000;
			perc_disp = battery_perc;


			if (voltage_disp != voltage_disp_prev) {
				ST7735_FillRectangleFast(77, 19, 160 - 77, 18, ST7735_BLACK);

				itoa(voltage_disp, buffer);
				ST7735_WriteString(0, 19, "Volts: ", Font_11x18, ST7735_YELLOW, ST7735_BLACK);
				ST7735_WriteString(77, 19, buffer, Font_11x18, ST7735_CYAN, ST7735_BLACK);

				voltage_disp_prev = voltage_disp;
			}
			if (perc_disp != perc_prev) {
				ST7735_FillRectangleFast(66, 38, 160 - 66, 18, ST7735_BLACK);

				itoa(perc_disp, buffer);
				ST7735_WriteString(0, 38, "Perc: ", Font_11x18, ST7735_YELLOW, ST7735_BLACK);
				ST7735_WriteString(66, 38, buffer, Font_11x18, ST7735_CYAN, ST7735_BLACK);
				perc_prev = perc_disp;
			}
			osDelay(300);
		}


		if (btn_state[0]) { //POWER

			btn_state[0] = 0;
		}
		if (btn_state[4] && setADCchargingChannel) { //charger plugged
			mode_selector = 3;
			ST7735_FillScreen(ST7735_BLACK);

			//HAL_ADC_Stop_DMA(&hadc1);
			voltage_disp_prev = 0xffff;
			MX_ADC1_myInit(ENABLE, ADC_SAMPLETIME_239CYCLES_5, ADC_SOFTWARE_START, ADC_CHANNEL_6);
			HAL_ADC_Start(&hadc1);

		}
		if (btn_state[1]) { //MODE
			mode_selector++;
			setADCchargingChannel = 1;
			if (mode_selector > 2) mode_selector = 0;
			ST7735_FillScreen(ST7735_BLACK);

			switch (mode_selector) {
			case 0: //capacitance
				capacitance_disp_prev = 0xffff;
				break;
			case 1: //voltage
				voltage_prev = 0xffff;
				if (hdma_adc1.State != 0) {
					HAL_ADC_Stop_DMA(&hadc1);
				}
				MX_ADC1_myInit(ENABLE, ADC_SAMPLETIME_239CYCLES_5, ADC_SOFTWARE_START, ADC_CHANNEL_9);
				//there is no break statement on purpose (for now there is...)
				break;
			case 2: //scope

				MX_ADC1_myInit(DISABLE, ADC_SAMPLETIME_239CYCLES_5, ADC_EXTERNALTRIGCONV_T1_CC1, ADC_CHANNEL_9);

				HAL_ADC_Start_DMA(&hadc1, scope_recording, 420);
				__HAL_DMA_ENABLE_IT(&hdma_adc1, DMA_IT_TC);

				HAL_ADC_Start(&hadc1);


				break;
				/*
				case 3: //charging
					if(hdma_adc1.State != 0){
					HAL_ADC_Stop_DMA(&hadc1);
					}
					MX_ADC1_Init(ENABLE, ADC_SAMPLETIME_1CYCLE_5, ADC_SOFTWARE_START, ADC_CHANNEL_6);  
					HAL_ADC_Start(&hadc1);
				 */
			}
			setADCvoltageProbeChannel = 0;
			btn_state[1] = 0;
		}
		if (btn_state[2]) { //MINUS
			timeDiv--;
			if (timeDiv < TIMESCALE_MIN) timeDiv = TIMESCALE_MIN;
			setScopeParams++;
			btn_state[2] = 0;
		}
		if (btn_state[3]) { //PLUS
			timeDiv++;
			if (timeDiv > TIMESCALE) timeDiv = TIMESCALE;
			setScopeParams++;
			btn_state[3] = 0;
		}

		osDelay(200);
	}
	/* USER CODE END 5 */
}

void adcCalibratorFunction(void const * argument)
{
	while (1) {
		FunctionalState continuousmode;
		uint32_t samplingtime, trigger, channel, sum = 0;
		continuousmode = hadc1.Init.ContinuousConvMode;
		trigger = hadc1.Init.ExternalTrigConv;
		channel = adc_sConfig.Channel;
		samplingtime = adc_sConfig.SamplingTime;

		MX_ADC1_myInit(ENABLE, ADC_SAMPLETIME_41CYCLES_5, ADC_SOFTWARE_START, ADC_CHANNEL_VREFINT);
		HAL_ADC_Start(&hadc1);

		for (int i = 0; i < 192; i++) {
			if (i > 63)
				sum += HAL_ADC_GetValue(&hadc1);
			else
				sum = 0;
		}

		//adc_voltage_raw = sum/8;
		adc_voltage = (4096 * 1.2 * 128.0) / (float) sum;

		MX_ADC1_myInit(continuousmode, samplingtime, trigger, channel);
		HAL_ADC_Start(&hadc1);

		osDelay(1000);
	}
}

void buttonHandlerFunction(void const * argument)
{
	while (1) {
		if (btn_state[4]) { //charging
			if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15)) {
				btn_state[4] = 0; //charger unplugged
				charging_finished = 0;
			}
			HAL_ADC_Start(&hadc1);

		} else { //normal button function, also debounce 

			for (int32_t i = 0; i < 5; i++) {
				if ((HAL_GPIO_ReadPin(GPIOA, 1 << i)) == 0) { //read each button and debounce
					btn_debounce[i]++;
				} else {
					btn_debounce[i] = 0;
				}
			}

			for (int32_t i = 0; i < 4; i++) { //check if press exceeded treshold value
				if (btn_debounce[i] > btn_tresh) {
					btn_state[i]++;
					btn_debounce[i] = 0;
				}
			}
		}
		osDelay(10);
	}
}

void batteryMonitorFunction(void const * argument)
{
	while (1) {

		osDelay(100);
	}
}

int user_strlen(char *str)
{
	int i = 0;
	while (*(str + i)) i++;
	return i;
}

void btn_power_handler(void)
{
	return;
}

int32_t *triggerWaveform(int len, int maxlen, int *graph, int level, int time)
{ //len has to be greater than time
	int32_t min = 0xFFFF, max = -0xFFFF;
	int32_t midline, trigger = time, wrongtrigger = time;

	if (level == 0xFFFF) {
		for (int i = 0; i < maxlen; i++) { //find max min
			if (graph[i] < min) min = graph[i];
			if (graph[i] > max) max = graph[i];
		}
		midline = (max + min) / 2;
	} else {
		midline = level;
	}

	//midline = 2048;
	if (graph[time] > midline) { //graph starts above the trigger...
		for (int i = time; i < maxlen; i++) {
			if (graph[i] < midline) {
				wrongtrigger = i; //passed the midline, search from there
				break;
			}
		}
	}

	for (int i = wrongtrigger; i < maxlen; i++) {
		if (graph[i] > midline) {
			trigger = i; //done, triggered
			break;
		}
	}


	if ((trigger - time + len) < maxlen)
		return(int32_t *) (trigger + graph - time);
	else
		return graph;

}

int utoa(uint32_t val, char* buf)
{
	const unsigned int radix = 10;

	char* p;
	unsigned int a; //every digit
	int len;
	char* b; //start of the digit char
	char temp;
	unsigned int u;

	p = buf;

	u = (unsigned int) val;

	b = p;

	do {
		a = u % radix;
		u /= radix;

		*p++ = a + '0';

	} while (u > 0);

	len = (int) (p - buf);

	*p-- = 0;

	//swap
	do {
		temp = *p;
		*p = *b;
		*b = temp;
		--p;
		++b;

	} while (b < p);

	return len;
}

int itoa(int val, char* buf)
{
	const unsigned int radix = 10;

	char* p;
	unsigned int a; //every digit
	int len;
	char* b; //start of the digit char
	char temp;
	unsigned int u;

	p = buf;

	if (val < 0) {
		*p++ = '-';
		val = 0 - val;
	}
	u = (unsigned int) val;

	b = p;

	do {
		a = u % radix;
		u /= radix;

		*p++ = a + '0';

	} while (u > 0);

	len = (int) (p - buf);

	*p-- = 0;

	//swap
	do {
		temp = *p;
		*p = *b;
		*b = temp;
		--p;
		++b;

	} while (b < p);

	return len;
}

