/*
  Name: James Hofer
  ID: 1000199225
  Omega Compile: make
*/

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)     ((b) + 1)
#define BLOCK_HEADER(ptr) ((struct _block *)(ptr) - 1)

static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *next;  /* Pointer to the next _block of allocated memory  */
   bool   free;          /* Is this _block free?                            */
   char   padding[3];    /* Padding: IENTRTMzMjAgU3jMDEED                   */
};


struct _block *heapList = NULL; /* Free list to track the _blocks available */

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = heapList;

#if defined FIT && FIT == 0
   /* First fit */
   //
   // While we haven't run off the end of the linked list and
   // while the current node we point to isn't free or isn't big enough
   // then continue to iterate over the list.  This loop ends either
   // with curr pointing to NULL, meaning we've run to the end of the list
   // without finding a node or it ends pointing to a free node that has enough
   // space for the request.
   // 
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

// \TODO Put your Best Fit code in this #ifdef block
#if defined BEST && BEST == 0
   /** \TODO Implement best fit here */
   struct _block *best = NULL;
   while (curr && !(curr->free && curr->size >= size)) 
   {
      if (best == NULL || curr->size < best->size)
         {
            best = curr;
         }
      *last = curr;
      curr  = curr->next;
   }
   curr = best;
#endif

// \TODO Put your Worst Fit code in this #ifdef block
#if defined WORST && WORST == 0
   /** \TODO Implement worst fit here */
   struct _block *worst = NULL;
   while (curr && !(curr->free && curr->size >= size)) 
   {
      if (worst == NULL || curr->size > worst->size)
         {
            worst = curr;
         }
      *last = curr;
      curr  = curr->next;
   }
   curr = worst;
#endif

// \TODO Put your Next Fit code in this #ifdef block
#if defined NEXT && NEXT == 0
   /** \TODO Implement next fit here */
   if (*last)
   {
      // start at last->next
      struct _block *oldLast = (*last);
      curr = (*last)->next;
      // iterate until we reach last again or find fit
      while (curr != oldLast && !(curr->free && curr->size >= size))
      {
         *last = curr;
         curr = curr->next;
      }
   }
   else
   {
      while (curr && !(curr->free && curr->size >= size)) 
      {
         *last = curr;
         curr  = curr->next;
      }
   }
#endif

   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size) 
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1) 
   {
      return NULL;
   }

   /* Update heapList if not set */
   if (heapList == NULL) 
   {
      heapList = curr;
   }

   /* Attach new _block to previous _block */
   if (last) 
   {
      last->next = curr;
   }

   /* Update _block metadata:
      Set the size of the new block and initialize the new block to "free".
      Set its next pointer to NULL since it's now the tail of the linked list.
   */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the 
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size) 
{

   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   num_mallocs++;
   num_requested += size;
   
   /* Handle 0 size */
   if (size == 0) 
   {
      return NULL;
   }

   /* Look for free _block.  If a free block isn't found then we need to grow our heap. */

   struct _block *last = heapList;
   struct _block *next = findFreeBlock(&last, size);

   /* TODO: If the block found by findFreeBlock is larger than we need then:
            If the leftover space in the new block is greater than the sizeof(_block)+4 then
            split the block.
            If the leftover space in the new block is less than the sizeof(_block)+4 then
            don't split the block.
   */
   if (next)
   {
      num_reuses++;
   }

   if (next && next->size > size)
   {
      if (next->size - size > sizeof(struct _block) + 4)
      {
         // create newPtr (1 + size + sizeof(struct)) bytes after nextPtr
         // newPtr size is leftover space after nextPtr
         // newPtr->next points to next block while nextPtr->next points to newPtr
         struct _block *newBlock = (struct _block *)((char *)next + size + sizeof(struct _block));
         newBlock->size = next->size - size - sizeof(struct _block);
         newBlock->next = next->next;
         newBlock->free = true;

         next->size = size;
         next->next = newBlock;

         num_splits++;
      }
   }

   /* Could not find free _block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);
      if (next)
      {
         num_grows++;
         max_heap += size;
      }
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL) 
   {
      return NULL;
   }
   
   /* Mark _block as in use */
   next->free = false;

   /* Return data address associated with _block to the user */
   return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr) 
{  
   if (ptr == NULL) 
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;
   num_frees++;
   num_blocks++;

   /* TODO: Coalesce free _blocks.  If the next block or previous block 
            are free then combine them with this block being freed.
   */

   // next block null check
   if (curr->next)
   {
      // preserve curr header and absorb next block header/data
      // if next block, update curr->next ptr
      if (curr->next->free)
      {
         curr->size += curr->next->size + sizeof(struct _block);
         curr->next = curr->next->next;
         num_coalesces++;
         num_blocks--;
      }
   }

   // prev block. start at heapList
   struct _block *prev = heapList;

   // curr is the first block check
   if (prev == curr)
   {
      return;
   }

   // find prev block
   while (prev->next != curr)
   {
      prev = prev->next;
   }

   // preserve prev header and absorb curr block header/data
   if (prev && prev->free)
   {
      prev->size += curr->size + sizeof(struct _block);
      prev->next = curr->next;
      num_coalesces++;
      num_blocks--;
   }
}

/*
 * \brief calloc
 *
 * gets a block of mem from malloc and zeros it out
 *
 * \param nmemb number of mem blocks
 * \param size size of each block
 *
 * \return ptr to the mem block or NULL if failed
 */
void *calloc( size_t nmemb, size_t size )
{
   // \TODO Implement calloc
   void *ptr = malloc(nmemb * size);
   if (ptr)
   {
      memset(ptr, 0, nmemb * size);
      return ptr;
   }

   return NULL;
}

/*
 * \brief realloc
 *
 * resizes a block of memory
 *
 * \param ptr ptr to the mem block
 * \param size new size of the block
 *
 * \return ptr to the mem block or NULL if failed
 */
void *realloc( void *ptr, size_t size )
{
   // \TODO Implement realloc
   if (ptr == NULL)
   {
      return malloc(size);
   }

   if (size == 0)
   {
      free(ptr);
      return NULL;
   }

   struct _block *curr = BLOCK_HEADER(ptr);
   size_t oldSize = curr->size;

   if (oldSize >= size)
   {
      curr->size = size;
      return ptr;
   }

   void *newPtr = malloc(size);
   if (newPtr)
   {
      memcpy(newPtr, ptr, oldSize);
      free(ptr);
      return newPtr;
   }

   return NULL;
}

/* vim: IENTRTMzMjAgU3ByaW5nIDIwMjM= -----------------------------------------*/
/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
