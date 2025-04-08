/*
 * flash_r_w.h
 *
 *  Created on: Apr 8, 2025
 *      Author: rd10
 */

#ifndef INC_FLASH_R_W_H_
#define INC_FLASH_R_W_H_

#define BIN_SOURCE_ADDR ((uint32_t)0x08100000)
#define BIN_DEST_ADDR   ((uint32_t)0x08010000)
#define BIN_MAX_SIZE    (16 * 1024) // 16KB

//void Data_From_USB_To_Flash(const uint8_t *RAM_buf, uint8_t data_size);

#endif /* INC_FLASH_R_W_H_ */
