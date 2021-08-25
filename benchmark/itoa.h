/*
 * itoa.h
 *
 *  Created on: 2015��8��18��
 *      Author: liwei
 */

#ifndef _ITOA_H_
#define _ITOA_H_
#include <cstdint>
int I2A_16(uint16_t i, char* a);
int I2A_32(uint32_t i, char* a);
int I2A_64(uint64_t i, char* a);
#define sitoa(i, a) ((i >= 0) ? I2A_16(i, a) : (a[0] = '-', 1 + I2A_16(~(i) + 1, a + 1)))
#define sutoa(i, a) I2A_16(i, a)
#define itoa(i, a) ((i >= 0) ? I2A_32(i, a) : (a[0] = '-', 1 + I2A_32(~(i) + 1, a + 1)))
#define utoa(i, a) I2A_32(i, a)
#define ltoa(i, a) ((i >= 0) ? I2A_64(i, a) : (a[0] = '-', 1 + I2A_64(~(i) + 1, a + 1)))
#define ultoa(i, a) I2A_64(i, a)
#endif /* _ITOA_H_ */
