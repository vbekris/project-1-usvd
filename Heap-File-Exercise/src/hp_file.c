#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/bf.h"
#include "../include/hp_file_structs.h"
#include "../include/record.h"

#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return 0;        \
    }                         \
  }

int HeapFile_Create(const char* fileName)
{
  int filehandler;
  
  // Δημιουργούμε νέο αρχείο blocks καλώντας μέσω της CALL_BF για έλεγχο λαθών
  CALL_BF(BF_CreateFile(fileName));

  //Ανοίγουμε το αρχείο που δημιουργήσαμε και αποθηκεύουμε τον χειρηστή στη μεταβλητή filehandler
  //πάλι με CALL_BF για έλεγχο πιθανών λαθών κατά το άνοιγμα
  CALL_BF(BF_OpenFile(fileName, &filehandler));

  //Δημιουργουμε το πρώτο Block για να αποθηκευσουμε εκεί το header του heap
  BF_Block *headerblock;
  BF_Block_Init(headerblock); //δημιουργια του μπλοκ

  CALL_BF(BF_AllocateBlock(filehandler, headerblock)); // ενταξη του στο heap
  
  //παιρνουμε pointer στα data του μπλοκ για να κανουμε αρχικοποιηση των δεδομενων του header
  char* tmp = BF_Block_GetData(headerblock);
  HeapFileHeader *header = (HeapFileHeader*) tmp;
  header->blocks_num = 1; //το μπλοκ του header
  header->currentblockid = 0; // invalid τιμη καθως ακομα δεν υπαρχει block δεδομενων
  
  //το block γινεται dirty αφου υπέστη αλλαγες
  //και υστερα unpin  αφου ολοκληρώσαμε τις διεργασιες, για να αποθηκευτουν οι αλλαγες στο αρχειο
  BF_Block_SetDirty(headerblock);
  CALL_BF(BF_UnpinBlock(headerblock));

  //απελευθέρωση του μπλοκ απο την μνήμη αφού εχει αποθηκευτει
  BF_Block_Destroy(headerblock);

  //κλείσιμο του ααρχείου
  CALL_BF(BF_CloseFile(filehandler));
  return 1;
}

int HeapFile_Open(const char *fileName, int *file_handle, HeapFileHeader** header_info)
{

void* data ;
BF_Block *block;          // arxikopoiw to block
BF_Block_Init(&block);
HeapFileHeader* header;    // arxikopoiw to header pointer


  CALL_BF(BF_OpenFile(fileName, file_handle));  //anoigma arxeiou kai apothikeusi tou file handle

  CALL_BF(BF_GetBlock(*file_handle,0,block));   //pairnw to prwto block pou periexei to header

  data = BF_Block_GetData(block);                   //pairnw ta dedomena tou block

  header = malloc(sizeof(HeapFileHeader)); //desmeyw mnimi gia to struct
  
  memcpy(header, data, sizeof(HeapFileHeader)); //antigrafo ta dedomena apo to block sto header
  
  if(strcmp(header->file_type,"heap") != 0){ //elegxw an einai heap file
      CALL_BF(BF_UnpinBlock(block));           // an den einai heap file, kanw unpin to block,
      BF_Block_Destroy(&block);          // apodesmeyw to block
      free(header);               // apodesmeyw to header 
    return 0; // den einai heap file
  }
  
  //an ftasei edw simainei oti einai heap file kai akolouthw idia diadikasia apodesmeyshs
  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  *header_info = header;
  return 1;
}

int HeapFile_Close(int file_handle, HeapFileHeader *hp_info)
{

  free(hp_info); // απελευθερωση του header απο τη μνημη (για τη malloc που ειχε γινει στην open)
  CALL_BF(BF_CloseFile(file_handle)); // κλεισιμο του αρχειου, επιστρεφει 0 αν ειχε μεινει καποιο block pinned
  return 1;
}

int HeapFile_InsertRecord(int file_handle, HeapFileHeader *hp_info, const Record record)
{   
BF_Block *block;
BF_Block_Init(&block);
BF_GetBlock(file_handle, hp_info->currentblockid, block);
char* data = BF_Block_GetData(block);


//HeapFileBlockMetadata *mdata = malloc(sizeof(HeapFileBlockMetadata)); // desmeyw mnimi gia to struct
HeapFileBlockMetadata *mdata = (HeapFileBlockMetadata*)(data + BF_BLOCK_SIZE - sizeof(HeapFileBlockMetadata)); //pairnw ta metadata tou block, deixnw ekei diladi
int max_records = (BF_BLOCK_SIZE - sizeof(HeapFileBlockMetadata)) / sizeof(Record); // ypologismos megistou arithmou eggrafwn pou mporei na periexei to block


  if(max_records - mdata->record_count > 0){ // an to block den einai gemato
      // eisagwgh eggrafhs sto block
      Record* rec_ptr = (Record*)(data + mdata->record_count * sizeof(Record));
      memcpy(rec_ptr, &record, sizeof(Record));
      mdata->record_count += 1;
      mdata->next_block_id = -1; // den yparxei epomeno block

      BF_Block_SetDirty(block);
      CALL_BF(BF_UnpinBlock(block));
      BF_Block_Destroy(&block);
      return 1;
  }
  if(max_records - mdata->record_count ==0 ){
      // to block einai gemato, prepei na dhmiourghthei neo block
      BF_Block_SetDirty(block);
      CALL_BF(BF_UnpinBlock(block));
      BF_Block_Destroy(&block);
      // dhmiourgia neou block
      BF_Block *new_block;
      BF_Block_Init(&new_block);
      CALL_BF(BF_AllocateBlock(file_handle, new_block));
      char* new_data = BF_Block_GetData(new_block);
      // eisagwgh eggrafhs sto neo block
      Record* rec_ptr = (Record*)(new_data);
      memcpy(rec_ptr, &record, sizeof(Record));
      // arxikopoihsh metadata tou neou block
      HeapFileBlockMetadata *new_mdata = (HeapFileBlockMetadata*)(new_data + BF_BLOCK_SIZE - sizeof(HeapFileBlockMetadata));
      new_mdata->record_count = 1;
      new_mdata->next_block_id = -1; // arxika den yparxei epomeno block
      hp_info->currentblockid += 1; // allagh tou current block id sto epomeno block
      hp_info->blocks_num += 1; // auxisi tou arithmou twn blocks sto header
      return 1;
    }
  else{
      // kapoio sfalma
      CALL_BF(BF_UnpinBlock(block));
      BF_Block_Destroy(&block);
      return 0;
    }
}


HeapFileIterator HeapFile_CreateIterator(    int file_handle, HeapFileHeader* header_info, int id)
{
  HeapFileIterator out;
  return out;
}


int HeapFile_GetNextRecord(    HeapFileIterator* heap_iterator, Record** record)
{
    * record=NULL;
    return 1;
}

