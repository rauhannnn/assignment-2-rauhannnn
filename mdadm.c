#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mdadm.h"
#include "jbod.h"

int mount_check = 0;

int mdadm_mount(void) {
  // Complete your code here
  if (mount_check == 1) {
    return -1;
  }
  else {
    uint32_t mount = JBOD_MOUNT<<12; // Shifts the JBOD_MOUNT command into bits 12-19
    int mounting = jbod_operation(mount, NULL);
    if (mounting == 0) {
      mount_check = 1;
      return 1;
    }
    else {
      return -1;
    }
  }
}

int mdadm_unmount(void) {
  //Complete your code here
  if (mount_check == 0) {
    return -1;
  }
  else {
    uint32_t unmount = JBOD_UNMOUNT<<12; // Shifts the JBOD_UNMOUNT command into bits 12-19
    int unmounting = jbod_operation(unmount, NULL);
    if (unmounting == 0) {
      mount_check = 0;
      return 1;
    }
    else {
      return -1;
    }
  }
}

int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf)  {
  //Complete your code here

  //Handling invalid test cases
  if (start_addr + read_len > 1048576) {
    return -1;
  }
  if (read_len > 1024) {
    return -2;
  }
  if (mount_check == 0) {
    return -3;
  }
  if ((read_buf == NULL) && (read_len != 0)) {
    return -4;
  }

  uint32_t byte_count = 0; // This is a counter that keeps track of bytes read
  uint32_t current_addr = start_addr; // Current address for reading in each iteration
  
  while (byte_count < read_len) {
    uint32_t disk_id = current_addr / 65536;
    uint32_t block_id = (current_addr % 65536) / 256;
    uint32_t disk_pos = disk_id;
    uint32_t block_pos = block_id << 4;
    uint32_t disk_op = disk_pos|(JBOD_SEEK_TO_DISK << 12);
    uint32_t block_op = block_pos|(JBOD_SEEK_TO_BLOCK << 12);
    uint32_t read_op = disk_pos|block_pos|(JBOD_READ_BLOCK << 12);
    uint32_t block_offset = (current_addr % 65536) % 256; // Block offset
    uint32_t bytes_to_read = 256 - block_offset; // Bytes in the block available to read from the start position to the end of the block
    uint32_t bytes_left = read_len - byte_count; // Remaining bytes to read
    uint8_t temp_array[256]; // temp array 

    // Determining how many bytes to read in each iteration
    if (bytes_to_read > bytes_left) {
      bytes_to_read = bytes_left;
    }

    // We must first seek to the correct disk and block.
    int seek_to_disk = jbod_operation(disk_op, NULL);
    int seek_to_block = jbod_operation(block_op, NULL);

    // Reading the block into temp_array
    int read_block = jbod_operation(read_op, temp_array);

    // Checking whether any of the three jbod operations failed
    if ((seek_to_disk < 0) || (seek_to_block < 0) || (read_block < 0)) {
      return -4;
    }

    // Copying necessary values from temp_array to read_buf
    memcpy(read_buf + byte_count, temp_array + block_offset, bytes_to_read);

    // Update the current address and total bytes read for the next iteration 
    current_addr = current_addr + bytes_to_read;
    byte_count = byte_count + bytes_to_read;
  }
  return read_len;
}