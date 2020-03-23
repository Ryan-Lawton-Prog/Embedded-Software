/*! @file median.c
 *
 *  @brief Median filter.
 *
 *  This contains the functions for performing a median filter on half-word-sized data.
 *
 *  @author Ryan Lawton (12545341) - Ashley More (12545479)
 *  @date 2019-09-30
 */
/*!
**  @addtogroup median_module median module documentation
**  @{
*/
/* MODULE median */

// New types
#include "types.h"
#include "median.h"
#include "OS.h"

static int8_t partition(int16_t arr[], int8_t l, int8_t r);
static int16_t kthSmallestValue (int16_t array[], int8_t k, int8_t l, int8_t r);
static void swap(int16_t *a, int16_t *b) ;



int16_t Median_Filter(const int16_t array[], const uint32_t size)
{
  int16_t new_array[size];

  for (uint8_t i; i < size; i++)
    new_array[i] = array[i];

  if (size % 2)
  {
    return kthSmallestValue(new_array, size/2, 0, size-1);
  }
  else
  {
    int16_t medianAvg = 0;
    medianAvg += kthSmallestValue(new_array, size/2 + 1, 0, size-1);
    medianAvg += kthSmallestValue(new_array, size/2, 0, size-1);
    return medianAvg / 2;
  }
}



/*! @brief Finds the kthSmallestValue through a recursive process (Quick Select; O(n) run time)
 *
 *  @param array is an array half-words for which the median is sought.
 *  @param k is the length of the array (or partition).
 *  @param l is the beginning of the array (or partition).
 *  @param r is the end of the array (or partition).
 *  @return kthSmallestValue
 */
int16_t kthSmallestValue (int16_t array[], int8_t k, int8_t l, int8_t r)
{
  if (k > 0 && k <= r - l + 1)
  {
    int8_t pivet = partition(array, l, r);

    if (pivet - 1 == k - 1)
      return array[pivet];
    if (pivet - 1 > k - 1)
      return kthSmallestValue(array, k, l, pivet - 1);

    return kthSmallestValue(array,  k - pivet + l - 1, pivet + 1, r);
  }

  return 0;
}



/*! @brief Partitions Array for pivoting
 *
 *  @param array is an array half-words for which the median is sought.
 *  @param l is the beginning of the array (or partition).
 *  @param r is the end of the array (or partition).
 *  @return index of array pivet
 */
int8_t partition(int16_t array[], int8_t l, int8_t r)
{
  int8_t x = array[r], i = l;

  for (int j = l; j <= r - 1; j++)
  {
    if (array[j] <= x)
    {
      swap(&array[i], &array[j]);
      i++;
    }
  }

  swap(&array[i], &array[r]);
  return i;
}



/*! @brief swaps params a and b
 *
 *  @param a is a halfword.
 *  @param b is a halfword.
 */
void swap(int16_t *a, int16_t *b)
{
    int16_t temp = *a;
    *a = *b;
    *b = temp;
}

/*!
** @}
*/
