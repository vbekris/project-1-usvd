#ifndef HP_FILE_STRUCTS_H
#define HP_FILE_STRUCTS_H

#include "record.h"

/**
 * @file hp_file_structs.h
 * @brief Data structures for heap file management
 */

/* -------------------------------------------------------------------------- */
/*                              Data Structures                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Heap file header containing metadata about the file organization
 */
typedef struct HeapFileHeader {
    int blocks_num;
    char file_type[8];
    int currentblockid;
     

} HeapFileHeader;


/**
 * @brief struct for the metadata of each block in the heap file
 */
typedef struct HeapFileBlockMetadata {
    int record_count;
    int next_block_id;
} HeapFileBlockMetadata;


/**
 * @brief Iterator for scanning through records in a heap file
 */
typedef struct HeapFileIterator{
    int file_handle; //Handle of the heap file to iterate over
    HeapFileHeader* header_info; //Pointer to heap file metadata
    int search_id; // Record ID to filter during iteration
    int current_block; //το εξεταζόμενο μπλοκ κατά την αναζητηση
    int current_record; // η εξεταζόμενη εγγραφή


} HeapFileIterator;

#endif /* HP_FILE_STRUCTS_H */
