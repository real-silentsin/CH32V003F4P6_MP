// ----------------------------------------------------------------------------
#include "sss_adc_libex.h"
// ----------------------------------------------------------------------------
uint32_t smooth_vdd = 0;
// ----------------------------------------------------------------------------
// Инициализация ADC, пины для измерения ДОЛЖНЫ быть настроены ДО вызова инициализации!
void ADC_PowerSafe_Init(void) {

    // 1. Включаем тактирование АЦП
    RCC_APB2PeriphClockCmd (RCC_APB2Periph_ADC1, ENABLE);

    // 2. Сброс регистров АЦП (критично для правильного перезапуска после сна)
    ADC_DeInit (ADC1);

    // 3. Настройка тактовой частоты АЦП (из вашего списка функций)
    // Делитель подбирается так, чтобы частота АЦП была в пределах даташита (обычно до 14-24 МГц)
    ADC_CLKConfig (ADC1, ADC_CLK_Div10);

    ADC_InitTypeDef ADC_InitStructure = {0};
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init (ADC1, &ADC_InitStructure);

    // 3. Включаем внутренний канал Vrefint
    // В этом MCU Vrefint включен постоянно на канале 15
    
    // 4. Включаем АЦП
    ADC_Cmd (ADC1, ENABLE);

    // Калибровка в данной модели MCU отсутствует/автоматизирована,
    // поэтому пропускаем блоки ResetCalibration/StartCalibration.
}
// ----------------------------------------------------------------------------
// Получение "чистого" измеренного значения в единицах ADC [0...4095]
uint16_t ADC_Get_Raw(uint8_t ch) {

    // Используем максимальное время выборки для стабильности
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_10Cycles);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    return ADC_GetConversionValue(ADC1);
}
// ----------------------------------------------------------------------------
// Вычисление реального напряжения питания (VDD) без Vref
uint32_t ADC_Get_VDD_mv(void) {

    uint16_t vref_raw = ADC_Get_Raw (ADC_CH_VREF);

    if (vref_raw == 0) { return 0; }

    // Формула: (Vref_mv * 4095) / Raw
    // Используем 64-битное приведение для точности промежуточного результата
    return (uint32_t)(((uint64_t)VREF_INT_MV * ADC_MAX_RAW) / vref_raw);
}
// ----------------------------------------------------------------------------
/* uint32_t ADC_Get_VDD_Filtered (void) {

    uint32_t current_vdd = ADC_Get_VDD_mv();
    if (smooth_vdd == 0)
        smooth_vdd = current_vdd;

    // EMA фильтр: 80% старого значения + 20% нового
    smooth_vdd = (smooth_vdd * 4 + current_vdd) / 5;
    return smooth_vdd;
} */
// ----------------------------------------------------------------------------
// Вычисление напряжения на произвольном канале в милливольтах
uint32_t ADC_Get_Voltage_mv(uint8_t ch) {

    uint32_t vdd = ADC_Get_VDD_mv();
    //uint32_t vdd = ADC_Get_VDD_Filtered();
    uint16_t raw = ADC_Get_Raw (ch);

    if (vdd == 0)
        return 0;

    // Формула: (Raw * VDD) / 4095
    return (uint32_t)(((uint64_t)raw * vdd) / ADC_MAX_RAW);
}
// ----------------------------------------------------------------------------
// Прямое вычисление напряжения на канале ch относительно Vrefint
uint32_t ADC_Get_Voltage_Direct_mv(uint8_t ch) {
    uint16_t raw_vref = ADC_Get_Raw (ADC_CH_VREF);  // Замер эталона
    uint16_t raw_ch = ADC_Get_Raw (ch);    // Замер целевого канала

    if (raw_vref == 0) return 0;

    // Математика: Vch = (1200 * RAW_ch) / RAW_vref
    // Используем 64-битное умножение, чтобы избежать переполнения
    return (uint32_t)(((uint64_t)VREF_INT_MV * raw_ch) / raw_vref);
}
// ----------------------------------------------------------------------------
// Вычисление среднего напряжения на произвольном канале в милливольтах
uint32_t ADC_Get_Average_Voltage_mv(uint8_t ch, uint8_t count) {

    if (count == 0) { return 0; }

    uint64_t raw_sum = 0;
    for (uint8_t i = 0; i < count; i++) {

        raw_sum += ADC_Get_Raw (ch);
    }

    uint16_t raw_avg = (uint16_t)(raw_sum / count);
    uint32_t vdd = ADC_Get_VDD_mv();

    return (uint32_t)(((uint64_t)raw_avg * vdd) / ADC_MAX_RAW);
}
// ----------------------------------------------------------------------------
