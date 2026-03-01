#include <unity.h>

#include "i2c_mock.h"
#include "circular_queue.h"

queue_t * queue;

typedef struct queue_item 
{
    int a;
    float b;
} queue_item_t;

void setUp(void)
{
    queue = queue_create();
}

void tearDown(void)
{
    
}

void test_create(void) 
{
    queue = queue_create();
    TEST_ASSERT_EQUAL(queue->capacity, QUEUE_SIZE);
}

void test_enqueue(void) 
{
    queue_item_t A = {1, 2.0};
    queue_item_t B = {-1, -2.0};
    queue_item_t C = {10000, 20000.1234567890};
    queue_enqueue(queue, &A);
    TEST_ASSERT_EQUAL(queue->tail, 1);
    queue_enqueue(queue, &B);
    TEST_ASSERT_EQUAL(queue->tail, 2);
    queue_enqueue(queue, &C);
    TEST_ASSERT_EQUAL(queue->tail, 3);
    TEST_ASSERT_EQUAL(queue->count, 3);

    // NULL checks
    queue_enqueue(queue, NULL);
    TEST_ASSERT_EQUAL(queue->count, 3);

    queue_enqueue(NULL, &A);
}

void test_dequeue(void) 
{
    queue_item_t * pA;
    bool ret = queue_dequeue(queue, &pA);
    TEST_ASSERT_FALSE(ret);

    queue_item_t A = {1, 2.0};
    queue_enqueue(queue, &A);
    TEST_ASSERT_EQUAL(queue->count, 1);
    ret = queue_dequeue(queue, &pA);
    TEST_ASSERT_TRUE(ret);
    TEST_ASSERT_EQUAL(queue->count, 0);
    TEST_ASSERT_EQUAL(pA->a, 1);
    TEST_ASSERT_EQUAL(pA->b, 2.0);

    ret = queue_dequeue(queue, &pA);
    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_EQUAL(queue->count, 0);

    for(int i = 0; i < QUEUE_SIZE + 1; i++)
        queue_enqueue(queue, &A);
    TEST_ASSERT_EQUAL(queue->count, QUEUE_SIZE);
}

int main(void) 
{
    UNITY_BEGIN();
    RUN_TEST(test_create);
    RUN_TEST(test_enqueue);
    RUN_TEST(test_dequeue);
    return UNITY_END();
}