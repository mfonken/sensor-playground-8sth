// i2c_mock.h

#include <stdint.h> 
#include <stddef.h> 
#include <stdbool.h> 

#define I2C_ADDR_ACCEL 0x1A // Returns 0 on success, non-zero on error. 
int i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len); 
int i2c_write_reg(uint8_t addr, uint8_t reg, const uint8_t *buf, size_t len); 

// For testing: advances internal "time" and updates sensor data. 
// Not something you would normally have in real firmware, but here to drive the simulation. 
void i2c_mock_step(void);
