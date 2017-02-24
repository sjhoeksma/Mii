#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <SRAM.h>

extern unsigned int __heap_start;
extern int  __bss_end;
extern int  __data_start;
extern void *__brkval;
/*
 * The free list structure as maintained by the
 * avr-libc memory allocation routines.
 */
struct __freelist {
  size_t sz;
  struct __freelist *nx;
};

/* The head of the free list structure */
extern struct __freelist *__flp;

/* Calculates the size of the free list */
int freeListSize() {
  struct __freelist* current;
  int total = 0;

  for (current = __flp; current; current = current->nx) {
    total += 2; /* Add two bytes for the memory block's header  */
    total += (int) current->sz;
  }

  return total;
}

int freeMemory() {
  int free_memory;

  if ((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__heap_start);
  } else {
    free_memory = ((int)&free_memory) - ((int)__brkval);
    free_memory += freeListSize();
  }
  return free_memory;
}

//Check if a variable is inSRAM
byte inSRAM(const void *s) {
  if ((int)__brkval == 0) {
    // if no heap use from end of bss section
    return   (int *)s>=&__data_start && (int *)s<=&__bss_end ? 1 : 0;
  }
  // use from top of stack to heap
  return   (int *)s>=&__data_start && (int *)s<=(int *)__brkval ? 1 : 0;
}
