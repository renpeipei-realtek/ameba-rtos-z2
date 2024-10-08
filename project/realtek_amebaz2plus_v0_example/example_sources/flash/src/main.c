/*
 * Copyright(c) 2007 - 2019 Realtek Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "flash_api.h"
#include "device_lock.h"

// Decide starting flash address for storing application data
// User should pick address carefully to avoid corrupting image section

#define FLASH_APP_BASE    0xFF000
static void flash_test_task(void *param)
{
	dbg_printf("\r\n   FLASH DEMO   \r\n");

	flash_t flash;
	uint32_t address = FLASH_APP_BASE;

#if 1
	uint32_t val32_to_write = 0x13572468;
	uint32_t val32_to_read;
	int loop = 0;
	int result = 0;

	for (loop = 0; loop < 10; loop++) {
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_read_word(&flash, address, &val32_to_read);
		dbg_printf("Read Data 0x%x \r\n", val32_to_read);
		flash_erase_sector(&flash, address);
		flash_write_word(&flash, address, val32_to_write);
		flash_read_word(&flash, address, &val32_to_read);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		dbg_printf("Read Data 0x%x \r\n", val32_to_read);

		// verify result
		result = (val32_to_write == val32_to_read) ? 1 : 0;
		//printf("\r\nResult is %s \r\n", (result) ? "success" : "fail");
		dbg_printf("\r\nResult is %s \r\n", ((result) ? "success" : "fail"));
		result = 0;
	}

#else
#define VERIFY_SIZE 256
	int SECTOR_SIZE = 16;

	uint8_t writedata[VERIFY_SIZE];
	uint8_t readdata[VERIFY_SIZE];
	uint8_t verifydata = 0;
	int loop = 0;
	int index = 0;
	int sectorindex = 0;
	int result = 0;
	int resultsector = 0;
	int testloop = 0;

	for (testloop = 0; testloop < 1; testloop++) {
		address = FLASH_APP_BASE;
		for (sectorindex = 0; sectorindex < 0x300; sectorindex++) {
			result = 0;
			//address += SECTOR_SIZE;
			device_mutex_lock(RT_DEV_LOCK_FLASH);
			flash_erase_sector(&flash, address);
			device_mutex_unlock(RT_DEV_LOCK_FLASH);
			//dbg_printf("Address = %x \n", address);
			for (loop = 0; loop < SECTOR_SIZE; loop++) {
				for (index = 0; index < VERIFY_SIZE; index++) {
					writedata[index] = verifydata + index;
				}
				device_mutex_lock(RT_DEV_LOCK_FLASH);
				flash_stream_write(&flash, address, VERIFY_SIZE, writedata);
				flash_stream_read(&flash, address, VERIFY_SIZE, readdata);
				device_mutex_unlock(RT_DEV_LOCK_FLASH);

				for (index = 0; index < VERIFY_SIZE; index++) {
					//dbg_printf("Address = %x, Writedata = %x, Readdata = %x \n",address,writedata[index],readdata[index]);
					if (readdata[index] != writedata[index]) {
						dbg_printf("Error: Loop = %d, Address = %x, Writedata = %x, Readdata = %x \r\n", testloop, address, writedata[index], readdata[index]);
					} else {
						result++;
						//dbg_printf(ANSI_COLOR_BLUE"Correct: Loop = %d, Address = %x, Writedata = %x, Readdata = %x \n"ANSI_COLOR_RESET,testloop,address,writedata[index],readdata[index]);
					}
				}
				address += VERIFY_SIZE;
			}
			if (result == (VERIFY_SIZE * SECTOR_SIZE)) {
				//dbg_printf("Sector %d Success \n", sectorindex);
				resultsector++;
			}
		}
		if (resultsector == 0x300) {
			dbg_printf("Test Loop %d Success \r\n", testloop);
		}
		resultsector = 0;
		verifydata++;
	}
	//dbg_printf("%d Sector Success \n", resultsector);

	dbg_printf("Test Done \r\n");

#endif
	vTaskDelete(NULL);
}

int main(void)
{
	if (xTaskCreate(flash_test_task, ((const char *)"flash_test_task"), 1024, NULL, (tskIDLE_PRIORITY + 1), NULL) != pdPASS) {
		dbg_printf("\n\r%s xTaskCreate(flash_test_task) failed", __FUNCTION__);
	}
	vTaskStartScheduler();
	while (1) {
		vTaskDelay((1000 / portTICK_RATE_MS));
	}
}
