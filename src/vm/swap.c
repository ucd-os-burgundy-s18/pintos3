#include "vm/swap.h"

void swap_init()
{
  global_swap_block = block_get_role(BLOCK_SWAP);
}
