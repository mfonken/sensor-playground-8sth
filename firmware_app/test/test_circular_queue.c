#include <unity.h>

#include "i2c_mock.h"
#include "circular_queue.h"

queue_t queue;

void setUp(void)
{
    queue_init(&queue);
}

void tearDown(void)
{
    
}

void test_init(void) 
{
    queue_init(&queue);
    TEST_ASSERT_EQUAL(queue.capacity, QUEUE_SIZE);
}

void test_enqueue(void) 
{
    sample_t A = {1};
    sample_t B = {2};
    sample_t C = {3};
    queue_enqueue(&queue, A);
    TEST_ASSERT_EQUAL(queue.tail, 1);
    queue_enqueue(&queue, B);
    TEST_ASSERT_EQUAL(queue.tail, 2);
    queue_enqueue(&queue, C);
    TEST_ASSERT_EQUAL(queue.tail, 3);
    TEST_ASSERT_EQUAL(queue.count, 3);

    // NULL checks
    // queue_enqueue(queue, NULL);
    // TEST_ASSERT_EQUAL(queue->count, 3);

    // queue_enqueue(NULL, A);
}

void test_dequeue(void) 
{
    sample_t A = {1};
    bool ret = queue_dequeue(&queue, &A);
    TEST_ASSERT_FALSE(ret);

    queue_enqueue(&queue, A);
    TEST_ASSERT_EQUAL(queue.count, 1);
    ret = queue_dequeue(&queue, &A);
    TEST_ASSERT_TRUE(ret);
    TEST_ASSERT_EQUAL(queue.count, 0);
    TEST_ASSERT_EQUAL(A.index, 1);

    ret = queue_dequeue(&queue, &A);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_EQUAL(queue.count, 0);

    for(int i = 0; i < QUEUE_SIZE; i++)
        queue_enqueue(&queue, A);
    TEST_ASSERT_EQUAL(queue.count, QUEUE_SIZE);
}

int main(void) 
{
    UNITY_BEGIN();
    RUN_TEST(test_init);
    RUN_TEST(test_enqueue);
    RUN_TEST(test_dequeue);
    return UNITY_END();
}