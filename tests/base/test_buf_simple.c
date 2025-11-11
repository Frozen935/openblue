#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "base/bt_buf.h"

static void test_head_tailroom(void)
{
    /* Stack simple buf of size 128 */
    BT_BUF_SIMPLE_DEFINE(simple, 128);

    /* Initially len=0, headroom=0, tailroom=size */
    assert(simple.len == 0);
    assert(bt_buf_simple_headroom(&simple) == 0);
    assert(bt_buf_simple_tailroom(&simple) == 128);

    /* Reserve headroom 16 */
    bt_buf_simple_init(&simple, 16);
    assert(bt_buf_simple_headroom(&simple) == 16);
    assert(bt_buf_simple_tailroom(&simple) == 112);
    assert(bt_buf_simple_max_len(&simple) == 112);

    /* Add 20 bytes, then check tailroom reduces */
    uint8_t pattern[20];
    for (int i = 0; i < 20; i++) pattern[i] = (uint8_t)i;
    bt_buf_simple_add_mem(&simple, pattern, sizeof(pattern));
    assert(simple.len == 20);
    assert(bt_buf_simple_tailroom(&simple) == 92);

    /* Remove 5 bytes from tail */
    bt_buf_simple_remove_mem(&simple, 5);
    assert(simple.len == 15);
    assert(bt_buf_simple_tailroom(&simple) == 97);

    /* Push 3 bytes at head */
    uint8_t headv[3] = {0xAA, 0xBB, 0xCC};
    bt_buf_simple_push_mem(&simple, headv, 3);
    assert(simple.len == 18);
    assert(bt_buf_simple_headroom(&simple) == 13);

    /* Pull those 3 back */
    uint8_t *old = (uint8_t *)bt_buf_simple_pull_mem(&simple, 3);
    assert(old[0] == 0xAA && old[1] == 0xBB && old[2] == 0xCC);
    assert(bt_buf_simple_headroom(&simple) == 16);
}

static void test_endian_helpers(void)
{
    BT_BUF_SIMPLE_DEFINE(buf, 64);
    bt_buf_simple_init(&buf, 0);

    /* Add LE/BE values and remove them from tail */
    bt_buf_simple_add_u8(&buf, 0x11);
    bt_buf_simple_add_le16(&buf, 0x2233);
    bt_buf_simple_add_be16(&buf, 0x4455);
    bt_buf_simple_add_le32(&buf, 0x66778899);
    bt_buf_simple_add_be32(&buf, 0xAABBCCDD);

    assert(bt_buf_simple_remove_be32(&buf) == 0xAABBCCDD);
    assert(bt_buf_simple_remove_le32(&buf) == 0x66778899);
    assert(bt_buf_simple_remove_be16(&buf) == 0x4455);
    assert(bt_buf_simple_remove_le16(&buf) == 0x2233);
    assert(bt_buf_simple_remove_u8(&buf) == 0x11);
}

static void test_init_with_data_and_clone(void)
{
    uint8_t raw[32];
    for (int i = 0; i < 32; i++) raw[i] = (uint8_t)(0xF0 + i);
    struct bt_buf_simple bs;
    bt_buf_simple_init_with_data(&bs, raw, sizeof(raw));
    assert(bs.len == 32);

    /* Clone state */
    struct bt_buf_simple clone;
    bt_buf_simple_clone(&bs, &clone);
    assert(clone.len == bs.len);
    assert(clone.__buf == bs.__buf);
    assert(clone.data == bs.data);

    /* Tail pointer */
    uint8_t *tail = bt_buf_simple_tail(&bs);
    assert(tail == bs.data + bs.len);
}

int main(void)
{
    printf("[buf_queue/test_buf_simple] head/tailroom...\n");
    test_head_tailroom();
    printf("[buf_queue/test_buf_simple] endian helpers...\n");
    test_endian_helpers();
    printf("[buf_queue/test_buf_simple] init_with_data + clone...\n");
    test_init_with_data_and_clone();
    printf("[buf_queue/test_buf_simple] all tests passed.\n");
    return 0;
}
