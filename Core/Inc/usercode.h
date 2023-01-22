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


#ifndef USERCODE_H
#define USERCODE_H

#ifdef __cplusplus
extern "C" {
#endif

    int utoa(uint32_t val, char* buf);
    int itoa(int val, char* buf);
    void button_poll(void);
    void btn_power_handler(void);
    int user_strlen(char *str);
    void readScope(void);
    int32_t *triggerWaveform(int len, int maxlen, int *graph, int level, int time);
    //void user_main_code(void);
    void getSupplyVoltage(void);
    
    void mainTaskFunction(void const * argument);
    void buttonHandlerFunction(void const * argument);
    void batteryMonitorFunction(void const * argument);
    void adcCalibratorFunction(void const * argument);
    
    int32_t scope_recording[420];
    int32_t scope_ready;
    
    int32_t adc_voltage_raw;
    float adc_voltage;

#ifdef __cplusplus
}
#endif

#endif /* USERCODE_H */

