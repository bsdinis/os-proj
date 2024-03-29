/* =============================================================================
 *
 * queue.c
 *
 * =============================================================================
 *
 * Copyright (C) Stanford University, 2006. All Rights Reserved.
 * Author: Chi Cao Minh
 *
 * =============================================================================
 *
 * For the license of bayes/sort.h and bayes/sort.c, please see the header
 * of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of ssca2, please see ssca2/COPYRIGHT
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/mt19937ar.c and lib/mt19937ar.h, please see the
 * header of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/rbtree.h and lib/rbtree.c, please see
 * lib/LEGALNOTICE.rbtree and lib/LICENSE.rbtree
 * 
 * ------------------------------------------------------------------------
 * 
 * Unless otherwise noted, the following license applies to STAMP files:
 * 
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *   * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 *   * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 *   * Neither the name of Stanford University nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 */


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "queue.h"


struct queue {
  long pop; /* points before element to pop */
  long push;
  long capacity;
  void** elements;
};

enum config {
  QUEUE_GROWTH_FACTOR = 2,
};


/* =============================================================================
 * queue_alloc
 * =============================================================================
 */
queue_t*
queue_alloc (long initCapacity)
{
  queue_t* queuePtr = (queue_t*)malloc(sizeof(queue_t));

  if (queuePtr) {
    long capacity = ((initCapacity < 2) ? 2 : initCapacity);
    queuePtr->elements = (void**)malloc(capacity * sizeof(void*));
    if (queuePtr->elements == NULL) {
      free(queuePtr);
      return NULL;
    }
    queuePtr->pop   = capacity - 1;
    queuePtr->push   = 0;
    queuePtr->capacity = capacity;
  }

  return queuePtr;
}


/* =============================================================================
 * queue_free
 * =============================================================================
 */
void
queue_free (queue_t* queuePtr)
{
  free(queuePtr->elements);
  free(queuePtr);
}


/* =============================================================================
 * queue_isEmpty
 * =============================================================================
 */
bool_t
queue_isEmpty (queue_t* queuePtr)
{
  long pop   = queuePtr->pop;
  long push   = queuePtr->push;
  long capacity = queuePtr->capacity;

  return (((pop + 1) % capacity == push) ? TRUE : FALSE);
}


/* =============================================================================
 * queue_clear
 * =============================================================================
 */
void
queue_clear (queue_t* queuePtr)
{
  queuePtr->pop = queuePtr->capacity - 1;
  queuePtr->push = 0;
}


/* =============================================================================
 * queue_push
 * =============================================================================
 */
bool_t
queue_push (queue_t* queuePtr, void* dataPtr)
{
  long pop   = queuePtr->pop;
  long push   = queuePtr->push;
  long capacity = queuePtr->capacity;

  assert(pop != push);

  /* Need to resize */
  long newPush = (push + 1) % capacity;
  if (newPush == pop) {

    long newCapacity = capacity * QUEUE_GROWTH_FACTOR;
    void** newElements = (void**)malloc(newCapacity * sizeof(void*));
    if (newElements == NULL) {
      return FALSE;
    }

    long dst = 0;
    void** elements = queuePtr->elements;
    if (pop < push) {
      long src;
      for (src = (pop + 1); src < push; src++, dst++) {
        newElements[dst] = elements[src];
      }
    } else {
      long src;
      for (src = (pop + 1); src < capacity; src++, dst++) {
        newElements[dst] = elements[src];
      }
      for (src = 0; src < push; src++, dst++) {
        newElements[dst] = elements[src];
      }
    }

    free(elements);
    queuePtr->elements = newElements;
    queuePtr->pop   = newCapacity - 1;
    queuePtr->capacity = newCapacity;
    push = dst;
    newPush = push + 1; /* no need modulo */
  }

  queuePtr->elements[push] = dataPtr;
  queuePtr->push = newPush;

  return TRUE;
}


/* =============================================================================
 * queue_pop
 * =============================================================================
 */
void*
queue_pop (queue_t* queuePtr)
{
  long pop   = queuePtr->pop;
  long push   = queuePtr->push;
  long capacity = queuePtr->capacity;

  long newPop = (pop + 1) % capacity;
  if (newPop == push) {
    return NULL;
  }

  void* dataPtr = queuePtr->elements[newPop];
  queuePtr->pop = newPop;

  return dataPtr;
}


/* =============================================================================
 * TEST_QUEUE
 * =============================================================================
 */
#ifdef TEST_QUEUE

#include <assert.h>
#include <stdio.h>


static void
printQueue (queue_t* queuePtr)
{
  long  push   = queuePtr->push;
  long  pop   = queuePtr->pop;
  long  capacity = queuePtr->capacity;
  void** elements = queuePtr->elements;

  printf("[");

  long i;
  for (i = ((pop + 1) % capacity); i != push; i = ((i + 1) % capacity)) {
    printf("%li ", *(long*)elements[i]);
  }
  puts("]");
}


static void
insertData (queue_t* queuePtr, long* dataPtr)
{
  printf("Inserting %li: ", *dataPtr);
  assert(queue_push(queuePtr, dataPtr));
  printQueue(queuePtr);
}


int
main ()
{
  queue_t* queuePtr;
  random_t* randomPtr;
  long data[] = {3, 1, 4, 1, 5};
  long numData = sizeof(data) / sizeof(data[0]);
  long i;

  randomPtr = random_alloc();
  assert(randomPtr);
  random_seed(randomPtr, 0);

  puts("Starting tests...");

  queuePtr = queue_alloc(-1);

  assert(queue_isEmpty(queuePtr));
  for (i = 0; i < numData; i++) {
    insertData(queuePtr, &data[i]);
  }
  assert(!queue_isEmpty(queuePtr));

  for (i = 0; i < numData; i++) {
    long* dataPtr = (long*)queue_pop(queuePtr);
    printf("Removing %li: ", *dataPtr);
    printQueue(queuePtr);
  }
  assert(!queue_pop(queuePtr));
  assert(queue_isEmpty(queuePtr));

  puts("All tests passed.");

  for (i = 0; i < numData; i++) {
    insertData(queuePtr, &data[i]);
  }
  for (i = 0; i < numData; i++) {
    printf("Shuffle %li: ", i);
    queue_shuffle(queuePtr, randomPtr);
    printQueue(queuePtr);
  }
  assert(!queue_isEmpty(queuePtr));

  queue_free(queuePtr);

  return 0;
}


#endif /* TEST_QUEUE */


/* =============================================================================
 *
 * End of queue.c
 *
 * =============================================================================
 */

