/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * This code is an adaptation of the Lee algorithm's implementation originally included in the STAMP Benchmark
 * by Stanford University.
 *
 * The original copyright notice is included below.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) Stanford University, 2006. All Rights Reserved.
 * Author: Chi Cao Minh
 *
 * =============================================================================
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
 *
 * grid.c
 *
 * =============================================================================
 */


#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "pthread_wrappers.h"

#include "coordinate.h"
#include "grid.h"
#include "lib/types.h"
#include "lib/vector.h"


const unsigned long CACHE_LINE_SIZE = 32UL;


/* =============================================================================
 * grid_alloc
 * =============================================================================
 */
grid_t* grid_alloc (long width, long height, long depth){
  grid_t* gridPtr;

  gridPtr = (grid_t*)malloc(sizeof(grid_t));
  if (gridPtr) {
    gridPtr->width = width;
    gridPtr->height = height;
    gridPtr->depth = depth;
    long n = width * height * depth;
    long* points_unaligned = (long*)malloc(n * sizeof(long) + CACHE_LINE_SIZE);
    assert(points_unaligned);
    gridPtr->locks = malloc(sizeof(pthread_mutex_t));
    Pthread_mutex_init(abort_exec, "grid_alloc: failed to init mutex", gridPtr->locks, NULL);
    /*
    gridPtr->locks_unaligned = (pthread_mutex_t *)malloc(n * sizeof(pthread_mutex_t) + CACHE_LINE_SIZE);
    assert(gridPtr->locks_unaligned);*/
    gridPtr->points_unaligned = points_unaligned;

    /**
     * Pointer black magic explained:
     * we want the arrays to be cache aligned so we add enough elements to have another line
     * by casting the pointer to an unsigned long, we can transform it
     * we then move the pointer to the next multiple of CACHE_LINE_SIZE
     * (since it is a multiple of 2, by &ing w/ ~(CACHE_LINE_SIZE -1) 
     * we get it to the previous multiple and then add CACHE_LINE SIZE to
     * get it to the next multiple)
     *
     * Enjoy nice block aligned arrays and low miss rates
     */
    gridPtr->points = (long*)((char*)(((unsigned long)points_unaligned
                     & ~(CACHE_LINE_SIZE-1)))
                 + CACHE_LINE_SIZE);
    /*
    gridPtr->locks = (pthread_mutex_t *)((char*)(((unsigned long)gridPtr->locks_unaligned
                     & ~(CACHE_LINE_SIZE-1)))
                 + CACHE_LINE_SIZE);
                 */
    memset(gridPtr->points, GRID_POINT_EMPTY, (n * sizeof(long)));

    /*
    for (long i = 0; i < n; i++) {
      if (Pthread_mutex_init(print_error, "grid_alloc: failed to init mutex", &gridPtr->locks[i], NULL)) {
        grid_free(gridPtr);
        return NULL;
      }
    }
    */
  }

  return gridPtr;
}

/* =============================================================================
 * grid_free
 * =============================================================================
 */
void grid_free (grid_t* gridPtr){
  free(gridPtr->points_unaligned);
  //free(gridPtr->locks_unaligned);
  free(gridPtr->locks);
  free(gridPtr);
}


/* =============================================================================
 * grid_copy
 * =============================================================================
 */
void grid_copy (grid_t* dstGridPtr, grid_t* srcGridPtr){
  assert(srcGridPtr->width == dstGridPtr->width);
  assert(srcGridPtr->height == dstGridPtr->height);
  assert(srcGridPtr->depth == dstGridPtr->depth);

  long n = srcGridPtr->width * srcGridPtr->height * srcGridPtr->depth;
  memcpy(dstGridPtr->points, srcGridPtr->points, (n * sizeof(long)));
}


/* =============================================================================
 * grid_isPointValid
 * =============================================================================
 */
bool_t grid_isPointValid (grid_t* gridPtr, long x, long y, long z){
  if (x < 0 || x >= gridPtr->width ||
    y < 0 || y >= gridPtr->height ||
    z < 0 || z >= gridPtr->depth)
  {
    return FALSE;
  }

  return TRUE;
}


/* =============================================================================
 * grid_getPointRef
 * =============================================================================
 */
long* grid_getPointRef (grid_t* gridPtr, long x, long y, long z){
  return &(gridPtr->points[(z * gridPtr->height + y) * gridPtr->width + x]);
}


/* =============================================================================
 * grid_getPointIndices
 * =============================================================================
 */
void grid_getPointIndices (grid_t* gridPtr, long* gridPointPtr, long* xPtr, long* yPtr, long* zPtr){
  long height = gridPtr->height;
  long width = gridPtr->width;
  long area = height * width;
  long index3d = (gridPointPtr - gridPtr->points); // pointer - base
  (*zPtr) = index3d / area;
  long index2d = index3d % area;
  (*yPtr) = index2d / width;
  (*xPtr) = index2d % width;
}


/* =============================================================================
 * grid_getPoint
 * =============================================================================
 */
long grid_getPoint (grid_t* gridPtr, long x, long y, long z){
  return *grid_getPointRef(gridPtr, x, y, z);
}


/* =============================================================================
 * grid_isPointEmpty
 * =============================================================================
 */
bool_t grid_isPointEmpty (grid_t* gridPtr, long x, long y, long z){
  long value = grid_getPoint(gridPtr, x, y, z);
  return ((value == GRID_POINT_EMPTY) ? TRUE : FALSE);
}


/* =============================================================================
 * grid_isPointFull
 * =============================================================================
 */
bool_t grid_isPointFull (grid_t* gridPtr, long x, long y, long z){
  long value = grid_getPoint(gridPtr, x, y, z);
  return ((value == GRID_POINT_FULL) ? TRUE : FALSE);
}


/* =============================================================================
 * grid_setPoint
 * =============================================================================
 */
void grid_setPoint (grid_t* gridPtr, long x, long y, long z, long value){
  (*grid_getPointRef(gridPtr, x, y, z)) = value;
}

/* =============================================================================
 * grid_lockPoint
 * =============================================================================
 */
bool_t grid_lockPoint (grid_t* gridPtr, long x, long y, long z){
  fprintf(stderr, "locking point (%ld, %ld, %ld) ------------ ", x, y, z);
  if (Pthread_mutex_lock(print_error, "grid_lockPoint: failed to lock mutex", &gridPtr->locks[(z * gridPtr->height + y) * gridPtr->width + x])) 
    return FALSE;

  fprintf(stderr, "got lock\n");
  return TRUE;
}

/* =============================================================================
 * grid_unlockPoint
 * =============================================================================
 */
bool_t grid_unlockPoint (grid_t* gridPtr, long x, long y, long z){
  fprintf(stderr, "unlocking point (%ld, %ld, %ld) ------------ ", x, y, z);
  if (Pthread_mutex_unlock(print_error, "grid_unlockPoint: failed to unlock mutex", &gridPtr->locks[(z * gridPtr->height + y) * gridPtr->width + x])) 
    return FALSE;

  fprintf(stderr, "released lock\n");
  return TRUE;
}

/* =============================================================================
 * grid_lockPointPtr
 * =============================================================================
 */
bool_t grid_lockPointPtr (grid_t* gridPtr, long *gridPointPtr){
  fprintf(stderr, "locking point %p ------------ ", (void *) gridPointPtr);
  long diff = (gridPointPtr - gridPtr->points);
  if (Pthread_mutex_lock(abort_exec, "grid_lockPoint: failed to lock mutex", &gridPtr->locks[diff]))
    return FALSE;

  fprintf(stderr, "got lock\n");
  return TRUE;
}

/* =============================================================================
 * grid_unlockPointPtr
 * =============================================================================
 */
bool_t grid_unlockPointPtr (grid_t* gridPtr, long *gridPointPtr){
  fprintf(stderr, "unlocking point %p ------------ ", (void *) gridPointPtr);
  long diff = (gridPointPtr - gridPtr->points);
  if (Pthread_mutex_unlock(abort_exec, "grid_unlockPoint: failed to unlock mutex", &gridPtr->locks[diff])) 
    return FALSE;

  fprintf(stderr, "released lock\n");
  return TRUE;
}

/* =============================================================================
 * grid_addPath
 * =============================================================================
 */
void grid_addPath (grid_t* gridPtr, vector_t* pointVectorPtr){
  long i;
  long n = vector_getSize(pointVectorPtr);

  for (i = 0; i < n; i++) {
    coordinate_t* coordinatePtr = (coordinate_t*)vector_at(pointVectorPtr, i);
    long x = coordinatePtr->x;
    long y = coordinatePtr->y;
    long z = coordinatePtr->z;
    grid_setPoint(gridPtr, x, y, z, GRID_POINT_FULL);
  }
}


/* =============================================================================
 * grid_addPath_Ptr
 * =============================================================================
 */
void grid_addPath_Ptr (grid_t* gridPtr, vector_t* pointVectorPtr){
  long i;
  long n = vector_getSize(pointVectorPtr);

  for (i = 1; i < (n-1); i++) {
    long* gridPointPtr = (long*)vector_at(pointVectorPtr, i);
    *gridPointPtr = GRID_POINT_FULL; 
  }
}

/* =============================================================================
 * compare two positions
 * =============================================================================
 */
int compare_positions(const void * a, const void *b) 
{
  /* this works ou quite nicely, because of the way the grid is mapped to mem */
  /* this orders points (a.x, a.y, a.z) and (b.x, b.y, b.z) with the predicate
   *
   * a < b iff
   * a.z < b.z || (a.z == b.z && a.y < b.y) || (a.z == b.z && a.y == b.y && a.x < b.x)
   */
  return (int) (((const long *) a)  - ((const long *) b));
}


/* =============================================================================
 * grid_checkPath_Ptr
 * =============================================================================
 */
bool_t grid_checkPath_Ptr(grid_t* gridPtr, vector_t* pointVectorPtr){
  long i;
  long n = vector_getSize(pointVectorPtr);

  vector_sort(pointVectorPtr, compare_positions);
  /*
  printf("==================== UNORDERED ===================\n");
  */
  for (i = 1; i < (n-1); i++) {
    long* gridPointPtr = (long*)vector_at(pointVectorPtr, i);
    /*
    long x, y, z;
    grid_getPointIndices(gridPtr, gridPointPtr, &x, &y, &z);
    printf("%lx %lx %lx\n", x, y, z);
    */
    
    if (*gridPointPtr == GRID_POINT_FULL) {
      return FALSE;
    }
  }



  /*
  printf("==================== ORDERED ===================\n");
  for (i = 1; i < (n-1); i++) {
    long* gridPointPtr = (long*)vector_at(pointVectorPtr, i);
    long x, y, z;
    grid_getPointIndices(gridPtr, gridPointPtr, &x, &y, &z);
    printf("%lx %lx %lx\n", x, y, z);
  }
  */

  return TRUE;

}

/* =============================================================================
 * grid_print
 * =============================================================================
 */
void grid_print (grid_t* gridPtr){
  grid_print_to_file(gridPtr, stdout);
}


/* =============================================================================
 * grid_print_to_file
 * =============================================================================
 */
void grid_print_to_file (grid_t* gridPtr, FILE *stream){
  assert(gridPtr);
  assert(stream);

  long x, y, z;
  long width = gridPtr->width;
  long height = gridPtr->height;
  long depth = gridPtr->depth;
  for (z = 0; z < depth; z++) {
   	fprintf(stream, "[z = %li]\n", z);
   	for (x = 0; x < width; x++) {
     	for (y = 0; y < height; y++) {
        fprintf(stream, "%4li", *grid_getPointRef(gridPtr, x, y, z));
      }
      fputc('\n', stream);
    }
    fputc('\n', stream);
  }
  fflush(stream);
}


/* =============================================================================
 *
 * End of grid.c
 *
 * =============================================================================
 */
