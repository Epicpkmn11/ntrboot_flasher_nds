#include "device.h"

enum return_codes_t {ALL_OK, FAT_MOUNT_FAILED, FILE_OPEN_FAILED, FILE_IO_FAILED, INJECT_OR_DUMP_FAILED, NO_BACKUP_FOUND}; 

return_codes_t InjectFIRM(flashcart_core::Flashcart* cart, bool twl, bool isDevMode);
return_codes_t DumpFlash(flashcart_core::Flashcart* cart);
return_codes_t RestoreFlash(flashcart_core::Flashcart* cart);
