/**
  ******************************************************************************
  * @file    plib035_i2c.c
  *
  * @brief   Файл содержит реализацию функций для работы с I2C
  *
  * @author  НИИЭТ, Богдан Колбов <kolbov@niiet.ru>
  *
  ******************************************************************************
  * @attention
  *
  * ДАННОЕ ПРОГРАММНОЕ ОБЕСПЕЧЕНИЕ ПРЕДОСТАВЛЯЕТСЯ «КАК ЕСТЬ», БЕЗ КАКИХ-ЛИБО
  * ГАРАНТИЙ, ЯВНО ВЫРАЖЕННЫХ ИЛИ ПОДРАЗУМЕВАЕМЫХ, ВКЛЮЧАЯ ГАРАНТИИ ТОВАРНОЙ
  * ПРИГОДНОСТИ, СООТВЕТСТВИЯ ПО ЕГО КОНКРЕТНОМУ НАЗНАЧЕНИЮ И ОТСУТСТВИЯ
  * НАРУШЕНИЙ, НО НЕ ОГРАНИЧИВАЯСЬ ИМИ. ДАННОЕ ПРОГРАММНОЕ ОБЕСПЕЧЕНИЕ
  * ПРЕДНАЗНАЧЕНО ДЛЯ ОЗНАКОМИТЕЛЬНЫХ ЦЕЛЕЙ И НАПРАВЛЕНО ТОЛЬКО НА
  * ПРЕДОСТАВЛЕНИЕ ДОПОЛНИТЕЛЬНОЙ ИНФОРМАЦИИ О ПРОДУКТЕ, С ЦЕЛЬЮ СОХРАНИТЬ ВРЕМЯ
  * ПОТРЕБИТЕЛЮ. НИ В КАКОМ СЛУЧАЕ АВТОРЫ ИЛИ ПРАВООБЛАДАТЕЛИ НЕ НЕСУТ
  * ОТВЕТСТВЕННОСТИ ПО КАКИМ-ЛИБО ИСКАМ, ЗА ПРЯМОЙ ИЛИ КОСВЕННЫЙ УЩЕРБ, ИЛИ
  * ПО ИНЫМ ТРЕБОВАНИЯМ, ВОЗНИКШИМ ИЗ-ЗА ИСПОЛЬЗОВАНИЯ ПРОГРАММНОГО ОБЕСПЕЧЕНИЯ
  * ИЛИ ИНЫХ ДЕЙСТВИЙ С ПРОГРАММНЫМ ОБЕСПЕЧЕНИЕМ.
  *
  * <h2><center>&copy; 2018 ОАО "НИИЭТ"</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "plib035_i2c.h"

/** @addtogroup Peripheral
  * @{
  */

/** @addtogroup I2C
  * @{
  */

/** @defgroup I2C_Private Приватные данные
  * @{
  */

/** @defgroup I2C_Private_Defines Приватные константы
  * @{
  */

/**
  * @}
  */

/** @defgroup I2C_Private_Functions Приватные функции
  * @{
  */

/**
  * @brief   Настройка частоты SCL врежиме FS
  * @param   FSFreq  Желаемое значение частоты в ГЦ
  * @param   I2CFreq  Значение частоты  тактового сигнала I2C в ГЦ
  * @retval  void
  */
void I2C_FSFreqConfig(uint32_t FSFreq, uint32_t I2CFreq)
{
    uint32_t freq_calc = I2CFreq / (4 * FSFreq);

    I2C_FSDivLowConfig(freq_calc & 0x7F);
    I2C_FSDivHighConfig(freq_calc >> 7);
}

/**
  * @brief   Настройка частоты SCL врежиме HS
  * @param   HSFreq  Желаемое значение частоты в ГЦ
  * @param   I2CFreq  Значение частоты  тактового сигнала I2C в ГЦ
  * @retval  void
  */
void I2C_HSFreqConfig(uint32_t HSFreq, uint32_t I2CFreq)
{
    uint32_t freq_calc = I2CFreq / (3 * HSFreq);

    I2C_HSDivLowConfig(freq_calc & 0x0F);
    I2C_HSDivHighConfig(freq_calc >> 4);
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/******************* (C) COPYRIGHT 2018 NIIET *****END OF FILE****/
