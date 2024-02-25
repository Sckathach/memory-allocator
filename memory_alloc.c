#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <setjmp.h>
#include "cmocka.h"
#include "memory_alloc.h"


struct memory_alloc_t m;

/* Initialize the memory allocator */
void memory_init() {
  for (int i = 0; i < DEFAULT_SIZE - 1; i++) {
    m.blocks[i] = i + 1;
  }
  m.blocks[DEFAULT_SIZE - 1] = NULL_BLOCK;
  m.first_block = 0;
  m.available_blocks = DEFAULT_SIZE;
}

/* Return the number of consecutive blocks starting from first */
int nb_consecutive_blocks(int first) {
  int index = first;
  int consecutive = 1;
  while (index != NULL_BLOCK) {
    if (m.blocks[index] - index == 1) {
      consecutive += 1;
    } else {
      return consecutive;
    }
    index = m.blocks[index];
  }
  return consecutive;
}

/* Return the number of consecutive blocks starting from first and making an entire loop */
int nb_consecutive_blocks_total() {
  int index = m.first_block;
  int last_index = m.first_block;
  int consecutive = 1;
  int max_consecutive = 1;
  while (index != NULL_BLOCK) {
    index = m.blocks[index];
    if (index - last_index == 1) {
      consecutive += 1;
    } else {
      if (consecutive >= max_consecutive) {
        max_consecutive = consecutive;
      }
      consecutive = 1;
    }
    last_index = index;
  }
  if (consecutive >= max_consecutive) {
    max_consecutive = consecutive;
  }
  return max_consecutive;
}

/* Reorder memory blocks */
int memory_reorder_aux() {
  int i = m.blocks[m.first_block];
  int j = m.first_block;
  int k = m.first_block;
  int c = 0;
  while (m.blocks[j] != NULL_BLOCK) {
    if (((i > j) && (m.blocks[i] > m.blocks[j])) || ((i < j) && (m.blocks[i] < m.blocks[j]))) {
      k = j;
      j = i;
      i = m.blocks[i];
    } else {
      c += 1;
      m.blocks[j] = m.blocks[i];
      m.blocks[i] = j;
      if (j == m.first_block) {
        m.first_block = i;
      } else {
        m.blocks[k] = i;
      }
      k = i;
      i = m.blocks[j];
    }
  }
  return c;
}
void memory_reorder(){
  int c = 0;
  do {
    c = memory_reorder_aux();
  } while (c > 0);
}

/* Allocate size bytes
 * return -1 in case of an error
 */
int memory_allocate(size_t size) {
  // if (nb_consecutive_blocks_total() < size) {
  //    memory_reorder();
  //    if (nb_consecutive_blocks_total() < size) {
  //      m.error_no = E_SHOULD_PACK;
  //      return -1;
  //    }
  // }
  int index = m.first_block;
  int last_index = index;
  int consecutive = 1;
  while (index != NULL_BLOCK) {
    consecutive = nb_consecutive_blocks(index);
    if (size <= 8 * consecutive) {
      if (index == m.first_block) {
          m.first_block = m.blocks[index + consecutive - 1];
      } else {
          m.blocks[last_index] = m.blocks[index + consecutive - 1];
      }
      m.available_blocks = m.available_blocks - consecutive;
      m.error_no = E_SUCCESS;
      initialize_buffer(index, size);
      return index;
    }
    last_index = index;
    index = m.blocks[index];
  }
  m.error_no = E_NOMEM;
  return -1;
}

/* Free the block of data starting at address */
void memory_free(int address, size_t size) {
  int last_start = m.first_block;
  int index = address;
  m.first_block = index;
  while (size + 8 > 0) {
    m.blocks[index] = index + 1;
    index += 1;
    size -= 8;
  }
  m.blocks[index] = last_start;
}

/* Print information on the available blocks of the memory allocator */
void memory_print() {
  printf("---------------------------------\n");
  printf("\tBlock size: %lu\n", sizeof(m.blocks[0]));
  printf("\tAvailable blocks: %lu\n", m.available_blocks);
  printf("\tFirst free: %d\n", m.first_block);
  printf("\tError_no: "); memory_error_print(m.error_no);
  printf("\tContent:  ");

  int i = m.first_block;
  while (i != NULL_BLOCK) {
    printf("[%d] -> ", i);
    i = m.blocks[i];
  }
  printf("NULL_BLOCK");

  printf("\n");
  printf("---------------------------------\n");
}


/* print the message corresponding to error_number */
void memory_error_print(enum memory_errno error_number) {
  switch(error_number) {
  case E_SUCCESS:
    printf("Success\n");
    break;
  case E_NOMEM:
    printf("Not enough memory\n");
    break;
  case  E_SHOULD_PACK:
    printf("Not enough contiguous blocks\n");
    break;
  default:
    printf("Unknown\n");
    break;
  }
}

/* Initialize an allocated buffer with zeros */
void initialize_buffer(int start_index, size_t size) {
  char* ptr = (char*)&m.blocks[start_index];
  for(int i=0; i<size; i++) {
    ptr[i]=0;
  }
}






/*************************************************/
/*             Test functions                    */
/*************************************************/

// We define a constant to be stored in a block which is supposed to be allocated:
// The value of this constant is such as it is different form NULL_BLOCK *and*
// it guarantees a segmentation vioaltion in case the code does something like
// m.blocks[A_B]
#define A_B INT32_MIN

/* Initialize m with all allocated blocks. So there is no available block */
void init_m_with_all_allocated_blocks() {
  struct memory_alloc_t m_init = {
    // 0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15
    {A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B, A_B},
    0,
    NULL_BLOCK,
    INT32_MIN // We initialize error_no with a value which we are sure that it cannot be set by the different memory_...() functions
  };
  m = m_init;
}

/* Test memory_init() */
void test_exo1_memory_init(){
  init_m_with_all_allocated_blocks();

  memory_init();

  // Check that m contains [0]->[1]->[2]->[3]->[4]->[5]->[6]->[7]->[8]->[9]->[10]->[11]->[12]->[13]->[14]->[15]->NULL_BLOCK
  assert_int_equal(m.first_block, 0);

  assert_int_equal(m.blocks[0], 1);
  assert_int_equal(m.blocks[1], 2);
  assert_int_equal(m.blocks[2], 3);
  assert_int_equal(m.blocks[3], 4);
  assert_int_equal(m.blocks[4], 5);
  assert_int_equal(m.blocks[5], 6);
  assert_int_equal(m.blocks[6], 7);
  assert_int_equal(m.blocks[7], 8);
  assert_int_equal(m.blocks[8], 9);
  assert_int_equal(m.blocks[9], 10);
  assert_int_equal(m.blocks[10], 11);
  assert_int_equal(m.blocks[11], 12);
  assert_int_equal(m.blocks[12], 13);
  assert_int_equal(m.blocks[13], 14);
  assert_int_equal(m.blocks[14], 15);
  assert_int_equal(m.blocks[15], NULL_BLOCK);
  
  assert_int_equal(m.available_blocks, DEFAULT_SIZE);

  // We do not care about value of m.error_no
}

/* Initialize m with some allocated blocks. The 10 available blocks are: [8]->[9]->[3]->[4]->[5]->[12]->[13]->[14]->[11]->[1]->NULL_BLOCK */
void init_m_with_some_allocated_blocks() {
  struct memory_alloc_t m_init = {
    // 0           1    2    3    4    5    6    7    8    9   10   11   12   13   14   15
    {A_B, NULL_BLOCK, A_B,   4,   5,  12, A_B, A_B,   9,   3, A_B,   1,  13,  14,  11, A_B},
    10,
    8,
    INT32_MIN // We initialize error_no with a value which we are sure that it cannot be set by the different memory_...() functions
  };
  m = m_init;
}

void init_perso(){
  struct memory_alloc_t m_init = {
    {A_B, 4, 3, 1, NULL_BLOCK},
    4,
    2,
    INT32_MIN
  };
  m = m_init;
}

/* Test nb_consecutive_block() at the beginning of the available blocks list */
void test_exo1_nb_consecutive_blocks_at_beginning_linked_list(){
  init_m_with_some_allocated_blocks();

  assert_int_equal(nb_consecutive_blocks(8), 2);
}

/* Test nb_consecutive_block() at the middle of the available blocks list */
void test_exo1_nb_consecutive_blocks_at_middle_linked_list(){
  init_m_with_some_allocated_blocks();
  memory_print();
  assert_int_equal(nb_consecutive_blocks(3), 3);
}

void test_perso(){
  init_m_with_some_allocated_blocks();
  memory_print();
  memory_reorder();
  memory_print();
}

/* Test nb_consecutive_block() at the end of the available blocks list */
void test_exo1_nb_consecutive_blocks_at_end_linked_list(){
  init_m_with_some_allocated_blocks();

  assert_int_equal(nb_consecutive_blocks(1), 1);
}

/* Test memory_allocate() when the blocks allocated are at the beginning of the linked list */
void test_exo1_memory_allocate_beginning_linked_list(){
  init_m_with_some_allocated_blocks();

  assert_int_equal(memory_allocate(16), 8);
  
  // We check that m contains [3]->[4]->[5]->[12]->[13]->[14]->[11]->[1]->NULL_BLOCK 
  assert_int_equal(m.first_block, 3);

  assert_int_equal(m.blocks[3], 4);
  assert_int_equal(m.blocks[4], 5);
  assert_int_equal(m.blocks[5], 12);
  assert_int_equal(m.blocks[12], 13);
  assert_int_equal(m.blocks[13], 14);
  assert_int_equal(m.blocks[14], 11);
  assert_int_equal(m.blocks[11], 1);
  assert_int_equal(m.blocks[1], NULL_BLOCK);
  
  assert_int_equal(m.available_blocks, 8);

  assert_int_equal(m.error_no, E_SUCCESS);
}

/* Test memory_allocate() when the blocks allocated are at in the middle of the linked list */
void test_exo1_memory_allocate_middle_linked_list(){
  init_m_with_some_allocated_blocks();

  assert_int_equal(memory_allocate(17), 3);
  
  // We check that m contains [8]->[9]->[12]->[13]->[14]->[11]->[1]->NULL_BLOCK 
  assert_int_equal(m.first_block, 8);

  assert_int_equal(m.blocks[8], 9);
  assert_int_equal(m.blocks[9], 12);
  assert_int_equal(m.blocks[12], 13);
  assert_int_equal(m.blocks[13], 14);
  assert_int_equal(m.blocks[14], 11);
  assert_int_equal(m.blocks[11], 1);
  assert_int_equal(m.blocks[1], NULL_BLOCK);
  
  assert_int_equal(m.available_blocks, 7);

  assert_int_equal(m.error_no, E_SUCCESS);
}

/* Test memory_allocate() when we ask for too many blocks ==> We get -1 return code and m.error_no is M_NOMEM */
void test_exo1_memory_allocate_too_many_blocks(){
  init_m_with_some_allocated_blocks();

  assert_int_equal(memory_allocate(256), -1);
  
  // We check that m does not change and still contains: [8]->[9]->[3]->[4]->[5]->[12]->[13]->[14]->[11]->[1]->NULL_BLOCK
  assert_int_equal(m.first_block, 8);

  assert_int_equal(m.blocks[8], 9);
  assert_int_equal(m.blocks[9], 3);
  assert_int_equal(m.blocks[3], 4);
  assert_int_equal(m.blocks[4], 5);
  assert_int_equal(m.blocks[5], 12);
  assert_int_equal(m.blocks[12], 13);
  assert_int_equal(m.blocks[13], 14);
  assert_int_equal(m.blocks[14], 11);
  assert_int_equal(m.blocks[11], 1);
  assert_int_equal(m.blocks[1], NULL_BLOCK);
  
  assert_int_equal(m.available_blocks, 10);

  assert_int_equal(m.error_no, E_NOMEM);
}

/* Test memory_free() */
void test_exo1_memory_free(){
  init_m_with_some_allocated_blocks();
  memory_free(6, 9);
  
  // We check that m contains: [6]->[7]->[8]->[9]->[3]->[4]->[5]->[12]->[13]->[14]->[11]->[1]->NULL_BLOCK
  assert_int_equal(m.first_block, 6);
  assert_int_equal(m.blocks[6], 7);
  assert_int_equal(m.blocks[7], 8);
  assert_int_equal(m.blocks[8], 9);
  assert_int_equal(m.blocks[9], 3);
  assert_int_equal(m.blocks[3], 4);
  assert_int_equal(m.blocks[4], 5);
  assert_int_equal(m.blocks[5], 12);
  assert_int_equal(m.blocks[12], 13);
  assert_int_equal(m.blocks[13], 14);
  assert_int_equal(m.blocks[14], 11);
  assert_int_equal(m.blocks[11], 1);
  assert_int_equal(m.blocks[1], NULL_BLOCK);
  
  assert_int_equal(m.available_blocks, 12);

  // We do not care about value of m.error_no
}

/* Test memory_reorder() */
void test_exo2_memory_reorder(){
  init_m_with_some_allocated_blocks();

  memory_reorder();
  
  // We check that m contains: [1]->[3]->[4]->[5]->[8]->[9]->[11]->[12]->[13]->[14]->NULL_BLOCK
  assert_int_equal(m.first_block, 1);

  assert_int_equal(m.blocks[1], 3);
  assert_int_equal(m.blocks[3], 4);
  assert_int_equal(m.blocks[4], 5);
  assert_int_equal(m.blocks[5], 8);
  assert_int_equal(m.blocks[8], 9);
  assert_int_equal(m.blocks[9], 11);
  assert_int_equal(m.blocks[11], 12);
  assert_int_equal(m.blocks[12], 13);
  assert_int_equal(m.blocks[13], 14);
  assert_int_equal(m.blocks[14], NULL_BLOCK);  
  assert_int_equal(m.available_blocks, 10);

  // We do not care about value of m.error_no
}

/* Test memory_reorder() leading to successful memory_allocate() because we find enough consecutive bytes */
void test_exo2_memory_reorder_leading_to_successful_memory_allocate(){
  init_m_with_some_allocated_blocks();

  assert_int_equal(memory_allocate(32), 11);
  // We check that m contains: [1]->[3]->[4]->[5]->[8]->[9]->NULL_BLOCK
  assert_int_equal(m.first_block, 1);

  assert_int_equal(m.blocks[1], 3);
  assert_int_equal(m.blocks[3], 4);
  assert_int_equal(m.blocks[4], 5);
  assert_int_equal(m.blocks[5], 8);
  assert_int_equal(m.blocks[8], 9);
  assert_int_equal(m.blocks[9], NULL_BLOCK);
  
  assert_int_equal(m.available_blocks, 6);

  assert_int_equal(m.error_no, E_SUCCESS);
}

/* Test memory_reorder() leading to failed memory_allocate() because not enough consecutive bytes */
void test_exo2_memory_reorder_leading_to_failed_memory_allocate(){
  init_m_with_some_allocated_blocks();

  assert_int_equal(memory_allocate(56), -1);

  // We check that m contains: [1]->[3]->[4]->[5]->[8]->[9]->[11]->[12]->[13]->[14]->NULL_BLOCK
  assert_int_equal(m.first_block, 1);

  assert_int_equal(m.blocks[1], 3);
  assert_int_equal(m.blocks[3], 4);
  assert_int_equal(m.blocks[4], 5);
  assert_int_equal(m.blocks[5], 8);
  assert_int_equal(m.blocks[8], 9);
  assert_int_equal(m.blocks[9], 11);
  assert_int_equal(m.blocks[11], 12);
  assert_int_equal(m.blocks[12], 13);
  assert_int_equal(m.blocks[13], 14);
  assert_int_equal(m.blocks[14], NULL_BLOCK);  
  assert_int_equal(m.available_blocks, 10);

  assert_int_equal(m.available_blocks, 10);

  assert_int_equal(m.error_no, E_SHOULD_PACK);
}

int main(int argc, char**argv) {
  const struct CMUnitTest tests[] = {
    /* a few tests for exercise 1.
     *
     * If you implemented correctly the functions, all these tests should be successfull
     * Of course this test suite may not cover all the tricky cases, and you are free to add
     * your own tests.
     */
    cmocka_unit_test(test_exo1_memory_init),
    cmocka_unit_test(test_exo1_nb_consecutive_blocks_at_beginning_linked_list),
    cmocka_unit_test(test_exo1_nb_consecutive_blocks_at_middle_linked_list),
    cmocka_unit_test(test_exo1_nb_consecutive_blocks_at_end_linked_list),
    cmocka_unit_test(test_exo1_memory_allocate_beginning_linked_list),
    cmocka_unit_test(test_exo1_memory_allocate_middle_linked_list),
    cmocka_unit_test(test_exo1_memory_allocate_too_many_blocks),
    cmocka_unit_test(test_exo1_memory_free),

    /* Run a few tests for exercise 2.
     *
     * If you implemented correctly the functions, all these tests should be successfull
     * Of course this test suite may not cover all the tricky cases, and you are free to add
     * your own tests.
     */

    cmocka_unit_test(test_exo2_memory_reorder),
    cmocka_unit_test(test_exo2_memory_reorder_leading_to_successful_memory_allocate),
    cmocka_unit_test(test_exo2_memory_reorder_leading_to_failed_memory_allocate)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
