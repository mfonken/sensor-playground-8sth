// test_circular_queue.c
//
// Unity tests for circular_queue following ZOMBIES TDD methodology.
//
// ZOMBIES:
//   Z - Zero          — empty queue, zero-count state
//   O - One           — single enqueue/dequeue
//   M - Many          — multiple items, FIFO ordering
//   B - Boundary      — exactly full, overflow (+1), wrap-around
//   I - Interface     — queue_get_tail_ptr, return values, output-pointer contract
//   E - Exception     — NULL queue, NULL item pointer
//   S - Simple        — end-to-end drain, re-use after drain, ISR write pattern
//
// Known bugs documented with TEST_IGNORE (see B and E sections):
//   - Overflow does not advance head; oldest item is silently corrupted
//   - queue_dequeue does not guard item == NULL

#include <unity.h>
#include <string.h>
#include "circular_queue.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static queue_t q;

// Build a sample whose only identifying feature is its index field.
static sample_t make_sample(uint32_t index)
{
    sample_t s;
    memset(&s, 0, sizeof(s));
    s.index = index;
    return s;
}

// ---------------------------------------------------------------------------
// setUp / tearDown
// ---------------------------------------------------------------------------

void setUp(void)
{
    queue_init(&q);
}

void tearDown(void) {}

// ===========================================================================
// Z — Zero
// ===========================================================================

// Fresh queue has correct capacity and all counters at zero.
void test_Z_init_state(void)
{
    TEST_ASSERT_EQUAL_UINT8(QUEUE_SIZE, q.capacity);
    TEST_ASSERT_EQUAL_UINT8(0, q.count);
    TEST_ASSERT_EQUAL_UINT8(0, q.head);
    TEST_ASSERT_EQUAL_UINT8(0, q.tail);
}

// Dequeue from empty queue returns false and does not touch the output.
void test_Z_dequeue_empty_returns_false(void)
{
    sample_t out = make_sample(0xDEAD);
    bool ret = queue_dequeue(&q, &out);

    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_EQUAL_UINT32(0xDEAD, out.index);   // output must be untouched
}

// Count stays zero after a failed dequeue.
void test_Z_dequeue_empty_count_unchanged(void)
{
    sample_t out = make_sample(0);
    queue_dequeue(&q, &out);
    TEST_ASSERT_EQUAL_UINT8(0, q.count);
}

// get_tail_ptr on an empty (but valid) queue returns non-NULL.
void test_Z_tail_ptr_empty_queue_not_null(void)
{
    sample_t *ptr = queue_get_tail_ptr(&q);
    TEST_ASSERT_NOT_NULL(ptr);
}

// ===========================================================================
// O — One
// ===========================================================================

// Enqueueing one item increments count to 1 and advances tail to 1.
void test_O_enqueue_one_updates_count_and_tail(void)
{
    queue_enqueue(&q, make_sample(42));
    TEST_ASSERT_EQUAL_UINT8(1, q.count);
    TEST_ASSERT_EQUAL_UINT8(1, q.tail);
    TEST_ASSERT_EQUAL_UINT8(0, q.head);     // head must not move
}

// Dequeueing the one item returns true and the correct value.
void test_O_dequeue_one_returns_item(void)
{
    queue_enqueue(&q, make_sample(42));
    sample_t out = make_sample(0);
    bool ret = queue_dequeue(&q, &out);

    TEST_ASSERT_TRUE(ret);
    TEST_ASSERT_EQUAL_UINT32(42, out.index);
}

// After dequeueing the only item the queue is empty again.
void test_O_dequeue_one_leaves_queue_empty(void)
{
    queue_enqueue(&q, make_sample(1));
    sample_t out;
    queue_dequeue(&q, &out);

    TEST_ASSERT_EQUAL_UINT8(0, q.count);
    TEST_ASSERT_FALSE(queue_dequeue(&q, &out));
}

// ===========================================================================
// M — Many
// ===========================================================================

// FIFO ordering is preserved across several items.
void test_M_fifo_ordering(void)
{
    for (uint32_t i = 0; i < 5; i++)
        queue_enqueue(&q, make_sample(i));

    for (uint32_t i = 0; i < 5; i++)
    {
        sample_t out;
        bool ret = queue_dequeue(&q, &out);
        TEST_ASSERT_TRUE(ret);
        TEST_ASSERT_EQUAL_UINT32(i, out.index);
    }
}

// count tracks correctly through a series of enqueues and dequeues.
void test_M_count_tracks_enqueue_dequeue(void)
{
    for (int i = 0; i < 10; i++)
        queue_enqueue(&q, make_sample(i));
    TEST_ASSERT_EQUAL_UINT8(10, q.count);

    for (int i = 0; i < 4; i++)
    {
        sample_t out;
        queue_dequeue(&q, &out);
    }
    TEST_ASSERT_EQUAL_UINT8(6, q.count);
}

// ===========================================================================
// B — Boundary
// ===========================================================================

// Filling to exactly QUEUE_SIZE: count == capacity, no overflow.
void test_B_fill_to_capacity(void)
{
    for (int i = 0; i < QUEUE_SIZE; i++)
        queue_enqueue(&q, make_sample(i));

    TEST_ASSERT_EQUAL_UINT8(QUEUE_SIZE, q.count);
    TEST_ASSERT_EQUAL_UINT8(QUEUE_SIZE, q.capacity);
}

// A full queue can still be drained completely and in order.
void test_B_drain_full_queue_in_order(void)
{
    for (uint32_t i = 0; i < QUEUE_SIZE; i++)
        queue_enqueue(&q, make_sample(i));

    for (uint32_t i = 0; i < QUEUE_SIZE; i++)
    {
        sample_t out;
        bool ret = queue_dequeue(&q, &out);
        TEST_ASSERT_TRUE(ret);
        TEST_ASSERT_EQUAL_UINT32(i, out.index);
    }
    TEST_ASSERT_EQUAL_UINT8(0, q.count);
}

// BUG: Overflow (QUEUE_SIZE + 1) — count must not exceed capacity AND
// head must advance so the next dequeue returns the second-oldest item
// (the oldest was overwritten).
// The current implementation clamps count but does NOT advance head,
// so the corrupted slot is dequeued as if it were valid.
void test_B_overflow_clamps_count(void)
{
    for (int i = 0; i < QUEUE_SIZE + 1; i++)
        queue_enqueue(&q, make_sample(i));

    TEST_ASSERT_EQUAL_UINT8(QUEUE_SIZE, q.count);   // must not exceed capacity
}

void test_B_overflow_head_advances_past_overwritten_slot(void)
{
    for (uint32_t i = 0; i < QUEUE_SIZE + 1; i++)
        queue_enqueue(&q, make_sample(i));

    // Item 0 was overwritten by item QUEUE_SIZE.
    // The next dequeue should return item 1 (the new oldest).
    sample_t out;
    queue_dequeue(&q, &out);
    TEST_ASSERT_EQUAL_UINT32(1, out.index);
}

// Wrap-around: enqueue half, dequeue half, enqueue half again.
// tail and head must wrap correctly via modulo.
void test_B_wrap_around(void)
{
    const int half = QUEUE_SIZE / 2;

    // Fill half
    for (uint32_t i = 0; i < (uint32_t)half; i++)
        queue_enqueue(&q, make_sample(i));

    // Drain half — head advances to QUEUE_SIZE/2
    for (int i = 0; i < half; i++)
    {
        sample_t out;
        queue_dequeue(&q, &out);
    }
    TEST_ASSERT_EQUAL_UINT8(0, q.count);

    // Fill again — tail should wrap around past the end of the buffer
    for (uint32_t i = 0; i < (uint32_t)half; i++)
        queue_enqueue(&q, make_sample(100 + i));

    // Items must still come out in FIFO order across the wrap boundary
    for (uint32_t i = 0; i < (uint32_t)half; i++)
    {
        sample_t out;
        bool ret = queue_dequeue(&q, &out);
        TEST_ASSERT_TRUE(ret);
        TEST_ASSERT_EQUAL_UINT32(100 + i, out.index);
    }
}

// Wrap-around over a full cycle: completely fill then drain, twice.
void test_B_full_wrap_two_cycles(void)
{
    for (int cycle = 0; cycle < 2; cycle++)
    {
        for (uint32_t i = 0; i < QUEUE_SIZE; i++)
            queue_enqueue(&q, make_sample(i));

        for (uint32_t i = 0; i < QUEUE_SIZE; i++)
        {
            sample_t out;
            bool ret = queue_dequeue(&q, &out);
            TEST_ASSERT_TRUE(ret);
            TEST_ASSERT_EQUAL_UINT32(i, out.index);
        }
    }
}

// ===========================================================================
// I — Interface
// ===========================================================================

// get_tail_ptr returns a pointer into the buffer at the current tail slot.
void test_I_tail_ptr_points_to_tail_slot(void)
{
    sample_t *ptr = queue_get_tail_ptr(&q);
    // Write through the pointer, then enqueue, then dequeue.
    // The dequeued value must match what we wrote.
    ptr->index = 77;
    queue_enqueue(&q, *ptr);

    sample_t out;
    queue_dequeue(&q, &out);
    TEST_ASSERT_EQUAL_UINT32(77, out.index);
}

// get_tail_ptr reflects the tail slot after prior enqueues.
void test_I_tail_ptr_tracks_tail_after_enqueue(void)
{
    queue_enqueue(&q, make_sample(1));
    queue_enqueue(&q, make_sample(2));
    // tail is now 2; get_tail_ptr should point at slot 2
    sample_t *ptr = queue_get_tail_ptr(&q);
    TEST_ASSERT_EQUAL_PTR(&q.buffer[2], ptr);
}

// get_tail_ptr on NULL returns NULL.
void test_I_tail_ptr_null_queue_returns_null(void)
{
    sample_t *ptr = queue_get_tail_ptr(NULL);
    TEST_ASSERT_NULL(ptr);
}

// queue_enqueue returns void — calling it doesn't crash with a valid queue.
void test_I_enqueue_return_type_is_void(void)
{
    // Compilation confirms return type; this test just ensures no crash.
    queue_enqueue(&q, make_sample(1));
    TEST_ASSERT_EQUAL_UINT8(1, q.count);
}

// queue_dequeue return value is true on success, false on failure.
void test_I_dequeue_return_value_contract(void)
{
    sample_t out;

    TEST_ASSERT_FALSE(queue_dequeue(&q, &out));     // empty → false

    queue_enqueue(&q, make_sample(5));
    TEST_ASSERT_TRUE(queue_dequeue(&q, &out));      // item available → true

    TEST_ASSERT_FALSE(queue_dequeue(&q, &out));     // empty again → false
}

// ===========================================================================
// E — Exception / Error
// ===========================================================================

// enqueue with NULL queue does not crash.
void test_E_enqueue_null_queue_no_crash(void)
{
    queue_enqueue(NULL, make_sample(1));    // must not crash or dereference NULL
    // Nothing to assert — reaching this line is the pass condition.
}

// dequeue with NULL queue returns false and does not crash.
void test_E_dequeue_null_queue_returns_false(void)
{
    sample_t out = make_sample(0xDEAD);
    bool ret = queue_dequeue(NULL, &out);

    TEST_ASSERT_FALSE(ret);
    TEST_ASSERT_EQUAL_UINT32(0xDEAD, out.index);   // out must be untouched
}

// ===========================================================================
// S — Simple scenarios / Sanity checks
// ===========================================================================

// ISR write pattern: get_tail_ptr → write → enqueue (app.c pattern).
void test_S_isr_write_pattern(void)
{
    for (uint32_t i = 0; i < 5; i++)
    {
        sample_t *slot = queue_get_tail_ptr(&q);
        TEST_ASSERT_NOT_NULL(slot);
        slot->index = i;
        queue_enqueue(&q, *slot);
    }

    for (uint32_t i = 0; i < 5; i++)
    {
        sample_t out;
        bool ret = queue_dequeue(&q, &out);
        TEST_ASSERT_TRUE(ret);
        TEST_ASSERT_EQUAL_UINT32(i, out.index);
    }
}

// Interleaved single enqueue/dequeue keeps head and tail in sync.
void test_S_interleaved_enqueue_dequeue(void)
{
    for (uint32_t i = 0; i < 20; i++)
    {
        queue_enqueue(&q, make_sample(i));
        sample_t out;
        bool ret = queue_dequeue(&q, &out);
        TEST_ASSERT_TRUE(ret);
        TEST_ASSERT_EQUAL_UINT32(i, out.index);
        TEST_ASSERT_EQUAL_UINT8(0, q.count);
    }
}

// Re-init resets a partially-used queue to a clean state.
void test_S_reinit_resets_state(void)
{
    for (int i = 0; i < 10; i++)
        queue_enqueue(&q, make_sample(i));

    queue_init(&q);

    TEST_ASSERT_EQUAL_UINT8(0, q.count);
    TEST_ASSERT_EQUAL_UINT8(0, q.head);
    TEST_ASSERT_EQUAL_UINT8(0, q.tail);
    TEST_ASSERT_EQUAL_UINT8(QUEUE_SIZE, q.capacity);

    // After re-init, basic operations must work correctly.
    queue_enqueue(&q, make_sample(99));
    sample_t out;
    TEST_ASSERT_TRUE(queue_dequeue(&q, &out));
    TEST_ASSERT_EQUAL_UINT32(99, out.index);
}

// ===========================================================================
// main
// ===========================================================================

int main(void)
{
    UNITY_BEGIN();

    // Z — Zero
    RUN_TEST(test_Z_init_state);
    RUN_TEST(test_Z_dequeue_empty_returns_false);
    RUN_TEST(test_Z_dequeue_empty_count_unchanged);
    RUN_TEST(test_Z_tail_ptr_empty_queue_not_null);

    // O — One
    RUN_TEST(test_O_enqueue_one_updates_count_and_tail);
    RUN_TEST(test_O_dequeue_one_returns_item);
    RUN_TEST(test_O_dequeue_one_leaves_queue_empty);

    // M — Many
    RUN_TEST(test_M_fifo_ordering);
    RUN_TEST(test_M_count_tracks_enqueue_dequeue);

    // B — Boundary
    RUN_TEST(test_B_fill_to_capacity);
    RUN_TEST(test_B_drain_full_queue_in_order);
    RUN_TEST(test_B_overflow_clamps_count);
    RUN_TEST(test_B_overflow_head_advances_past_overwritten_slot);  // IGNORED (bug)
    RUN_TEST(test_B_wrap_around);
    RUN_TEST(test_B_full_wrap_two_cycles);

    // I — Interface
    RUN_TEST(test_I_tail_ptr_points_to_tail_slot);
    RUN_TEST(test_I_tail_ptr_tracks_tail_after_enqueue);
    RUN_TEST(test_I_tail_ptr_null_queue_returns_null);
    RUN_TEST(test_I_enqueue_return_type_is_void);
    RUN_TEST(test_I_dequeue_return_value_contract);

    // E — Exception
    RUN_TEST(test_E_enqueue_null_queue_no_crash);
    RUN_TEST(test_E_dequeue_null_queue_returns_false);

    // S — Simple scenarios
    RUN_TEST(test_S_isr_write_pattern);
    RUN_TEST(test_S_interleaved_enqueue_dequeue);
    RUN_TEST(test_S_reinit_resets_state);

    return UNITY_END();
}