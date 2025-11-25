#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include <base/byteorder.h>
#include <base/bt_buf.h>
#include <base/bt_assert.h>

void bt_buf_simple_init_with_data(struct bt_buf_simple *buf, void *data, size_t size)
{
	buf->__buf = data;
	buf->data = data;
	buf->size = size;
	buf->len = size;
}

void bt_buf_simple_reserve(struct bt_buf_simple *buf, size_t reserve)
{
	__ASSERT_NO_MSG(buf);
	__ASSERT_NO_MSG(buf->len == 0U);

	buf->data = buf->__buf + reserve;
}

void bt_buf_simple_clone(const struct bt_buf_simple *original, struct bt_buf_simple *clone)
{
	memcpy(clone, original, sizeof(struct bt_buf_simple));
}

void *bt_buf_simple_add(struct bt_buf_simple *buf, size_t len)
{
	uint8_t *tail = bt_buf_simple_tail(buf);

	__ASSERT_NO_MSG(bt_buf_simple_tailroom(buf) >= len);

	buf->len += len;
	return tail;
}

void *bt_buf_simple_add_mem(struct bt_buf_simple *buf, const void *mem, size_t len)
{
	return memcpy(bt_buf_simple_add(buf, len), mem, len);
}

uint8_t *bt_buf_simple_add_u8(struct bt_buf_simple *buf, uint8_t val)
{
	uint8_t *u8;

	u8 = bt_buf_simple_add(buf, 1);
	*u8 = val;

	return u8;
}

void bt_buf_simple_add_le16(struct bt_buf_simple *buf, uint16_t val)
{
	sys_put_le16(val, bt_buf_simple_add(buf, sizeof(val)));
}

void bt_buf_simple_add_be16(struct bt_buf_simple *buf, uint16_t val)
{
	sys_put_be16(val, bt_buf_simple_add(buf, sizeof(val)));
}

void bt_buf_simple_add_le24(struct bt_buf_simple *buf, uint32_t val)
{
	sys_put_le24(val, bt_buf_simple_add(buf, 3));
}

void bt_buf_simple_add_be24(struct bt_buf_simple *buf, uint32_t val)
{
	sys_put_be24(val, bt_buf_simple_add(buf, 3));
}

void bt_buf_simple_add_le32(struct bt_buf_simple *buf, uint32_t val)
{
	sys_put_le32(val, bt_buf_simple_add(buf, sizeof(val)));
}

void bt_buf_simple_add_be32(struct bt_buf_simple *buf, uint32_t val)
{
	sys_put_be32(val, bt_buf_simple_add(buf, sizeof(val)));
}

void bt_buf_simple_add_le40(struct bt_buf_simple *buf, uint64_t val)
{
	sys_put_le40(val, bt_buf_simple_add(buf, 5));
}

void bt_buf_simple_add_be40(struct bt_buf_simple *buf, uint64_t val)
{
	sys_put_be40(val, bt_buf_simple_add(buf, 5));
}

void bt_buf_simple_add_le48(struct bt_buf_simple *buf, uint64_t val)
{
	sys_put_le48(val, bt_buf_simple_add(buf, 6));
}

void bt_buf_simple_add_be48(struct bt_buf_simple *buf, uint64_t val)
{
	sys_put_be48(val, bt_buf_simple_add(buf, 6));
}

void bt_buf_simple_add_le64(struct bt_buf_simple *buf, uint64_t val)
{
	sys_put_le64(val, bt_buf_simple_add(buf, sizeof(val)));
}

void bt_buf_simple_add_be64(struct bt_buf_simple *buf, uint64_t val)
{
	sys_put_be64(val, bt_buf_simple_add(buf, sizeof(val)));
}

void *bt_buf_simple_remove_mem(struct bt_buf_simple *buf, size_t len)
{
	__ASSERT_NO_MSG(buf->len >= len);

	buf->len -= len;
	return buf->data + buf->len;
}

uint8_t bt_buf_simple_remove_u8(struct bt_buf_simple *buf)
{
	uint8_t val;
	void *ptr;

	ptr = bt_buf_simple_remove_mem(buf, sizeof(val));
	val = *(uint8_t *)ptr;

	return val;
}

uint16_t bt_buf_simple_remove_le16(struct bt_buf_simple *buf)
{
	void *ptr;

	ptr = bt_buf_simple_remove_mem(buf, sizeof(uint16_t));
	return sys_get_le16(ptr);
}

uint16_t bt_buf_simple_remove_be16(struct bt_buf_simple *buf)
{
	void *ptr;

	ptr = bt_buf_simple_remove_mem(buf, sizeof(uint16_t));
	return sys_get_be16(ptr);
}

uint32_t bt_buf_simple_remove_le24(struct bt_buf_simple *buf)
{
	void *ptr;

	ptr = bt_buf_simple_remove_mem(buf, 3);
	return sys_get_le24(ptr);
}

uint32_t bt_buf_simple_remove_be24(struct bt_buf_simple *buf)
{
	void *ptr;

	ptr = bt_buf_simple_remove_mem(buf, 3);
	return sys_get_be24(ptr);
}

uint32_t bt_buf_simple_remove_le32(struct bt_buf_simple *buf)
{
	void *ptr;

	ptr = bt_buf_simple_remove_mem(buf, sizeof(uint32_t));
	return sys_get_le32(ptr);
}

uint32_t bt_buf_simple_remove_be32(struct bt_buf_simple *buf)
{
	void *ptr;

	ptr = bt_buf_simple_remove_mem(buf, sizeof(uint32_t));
	return sys_get_be32(ptr);
}

uint64_t bt_buf_simple_remove_le40(struct bt_buf_simple *buf)
{
	void *ptr;

	ptr = bt_buf_simple_remove_mem(buf, 5);
	return sys_get_le40(ptr);
}

uint64_t bt_buf_simple_remove_be40(struct bt_buf_simple *buf)
{
	void *ptr;

	ptr = bt_buf_simple_remove_mem(buf, 5);
	return sys_get_be40(ptr);
}

uint64_t bt_buf_simple_remove_le48(struct bt_buf_simple *buf)
{
	void *ptr;

	ptr = bt_buf_simple_remove_mem(buf, 6);
	return sys_get_le48(ptr);
}

uint64_t bt_buf_simple_remove_be48(struct bt_buf_simple *buf)
{
	void *ptr;

	ptr = bt_buf_simple_remove_mem(buf, 6);
	return sys_get_be48(ptr);
}

uint64_t bt_buf_simple_remove_le64(struct bt_buf_simple *buf)
{
	void *ptr;

	ptr = bt_buf_simple_remove_mem(buf, sizeof(uint64_t));
	return sys_get_le64(ptr);
}

uint64_t bt_buf_simple_remove_be64(struct bt_buf_simple *buf)
{
	void *ptr;

	ptr = bt_buf_simple_remove_mem(buf, sizeof(uint64_t));
	return sys_get_be64(ptr);
}

void *bt_buf_simple_push(struct bt_buf_simple *buf, size_t len)
{
	__ASSERT_NO_MSG(bt_buf_simple_headroom(buf) >= len);

	buf->data -= len;
	buf->len += len;
	return buf->data;
}

void *bt_buf_simple_push_mem(struct bt_buf_simple *buf, const void *mem, size_t len)
{
	return memcpy(bt_buf_simple_push(buf, len), mem, len);
}

void bt_buf_simple_push_le16(struct bt_buf_simple *buf, uint16_t val)
{
	sys_put_le16(val, bt_buf_simple_push(buf, sizeof(val)));
}

void bt_buf_simple_push_be16(struct bt_buf_simple *buf, uint16_t val)
{
	sys_put_be16(val, bt_buf_simple_push(buf, sizeof(val)));
}

void bt_buf_simple_push_u8(struct bt_buf_simple *buf, uint8_t val)
{
	uint8_t *data = bt_buf_simple_push(buf, 1);

	*data = val;
}

void bt_buf_simple_push_le24(struct bt_buf_simple *buf, uint32_t val)
{
	sys_put_le24(val, bt_buf_simple_push(buf, 3));
}

void bt_buf_simple_push_be24(struct bt_buf_simple *buf, uint32_t val)
{
	sys_put_be24(val, bt_buf_simple_push(buf, 3));
}

void bt_buf_simple_push_le32(struct bt_buf_simple *buf, uint32_t val)
{
	sys_put_le32(val, bt_buf_simple_push(buf, sizeof(val)));
}

void bt_buf_simple_push_be32(struct bt_buf_simple *buf, uint32_t val)
{
	sys_put_be32(val, bt_buf_simple_push(buf, sizeof(val)));
}

void bt_buf_simple_push_le40(struct bt_buf_simple *buf, uint64_t val)
{
	sys_put_le40(val, bt_buf_simple_push(buf, 5));
}

void bt_buf_simple_push_be40(struct bt_buf_simple *buf, uint64_t val)
{
	sys_put_be40(val, bt_buf_simple_push(buf, 5));
}

void bt_buf_simple_push_le48(struct bt_buf_simple *buf, uint64_t val)
{
	sys_put_le48(val, bt_buf_simple_push(buf, 6));
}

void bt_buf_simple_push_be48(struct bt_buf_simple *buf, uint64_t val)
{
	sys_put_be48(val, bt_buf_simple_push(buf, 6));
}

void bt_buf_simple_push_le64(struct bt_buf_simple *buf, uint64_t val)
{
	sys_put_le64(val, bt_buf_simple_push(buf, sizeof(val)));
}

void bt_buf_simple_push_be64(struct bt_buf_simple *buf, uint64_t val)
{
	sys_put_be64(val, bt_buf_simple_push(buf, sizeof(val)));
}

void *bt_buf_simple_pull(struct bt_buf_simple *buf, size_t len)
{
	__ASSERT_NO_MSG(buf->len >= len);

	buf->len -= len;
	return buf->data += len;
}

void *bt_buf_simple_pull_mem(struct bt_buf_simple *buf, size_t len)
{
	void *data = buf->data;

	__ASSERT_NO_MSG(buf->len >= len);

	buf->len -= len;
	buf->data += len;

	return data;
}

uint8_t bt_buf_simple_pull_u8(struct bt_buf_simple *buf)
{
	uint8_t val;

	val = buf->data[0];
	bt_buf_simple_pull(buf, 1);

	return val;
}

uint16_t bt_buf_simple_pull_le16(struct bt_buf_simple *buf)
{
	uint16_t val;

	val = sys_get_le16(buf->data);
	bt_buf_simple_pull(buf, sizeof(val));

	return val;
}

uint16_t bt_buf_simple_pull_be16(struct bt_buf_simple *buf)
{
	uint16_t val;

	val = sys_get_be16(buf->data);
	bt_buf_simple_pull(buf, sizeof(val));

	return val;
}

uint32_t bt_buf_simple_pull_le24(struct bt_buf_simple *buf)
{
	uint32_t val;

	val = sys_get_le24(buf->data);
	bt_buf_simple_pull(buf, 3);

	return val;
}

uint32_t bt_buf_simple_pull_be24(struct bt_buf_simple *buf)
{
	uint32_t val;

	val = sys_get_be24(buf->data);
	bt_buf_simple_pull(buf, 3);

	return val;
}

uint32_t bt_buf_simple_pull_le32(struct bt_buf_simple *buf)
{
	uint32_t val;

	val = sys_get_le32(buf->data);
	bt_buf_simple_pull(buf, sizeof(val));

	return val;
}

uint32_t bt_buf_simple_pull_be32(struct bt_buf_simple *buf)
{
	uint32_t val;

	val = sys_get_be32(buf->data);
	bt_buf_simple_pull(buf, sizeof(val));

	return val;
}

uint64_t bt_buf_simple_pull_le40(struct bt_buf_simple *buf)
{
	uint64_t val;

	val = sys_get_le40(buf->data);
	bt_buf_simple_pull(buf, 5);

	return val;
}

uint64_t bt_buf_simple_pull_be40(struct bt_buf_simple *buf)
{
	uint64_t val;

	val = sys_get_be40(buf->data);
	bt_buf_simple_pull(buf, 5);

	return val;
}

uint64_t bt_buf_simple_pull_le48(struct bt_buf_simple *buf)
{
	uint64_t val;

	val = sys_get_le48(buf->data);
	bt_buf_simple_pull(buf, 6);

	return val;
}

uint64_t bt_buf_simple_pull_be48(struct bt_buf_simple *buf)
{
	uint64_t val;

	val = sys_get_be48(buf->data);
	bt_buf_simple_pull(buf, 6);

	return val;
}

uint64_t bt_buf_simple_pull_le64(struct bt_buf_simple *buf)
{
	uint64_t val;

	val = sys_get_le64(buf->data);
	bt_buf_simple_pull(buf, sizeof(val));

	return val;
}

uint64_t bt_buf_simple_pull_be64(struct bt_buf_simple *buf)
{
	uint64_t val;

	val = sys_get_be64(buf->data);
	bt_buf_simple_pull(buf, sizeof(val));

	return val;
}

size_t bt_buf_simple_headroom(const struct bt_buf_simple *buf)
{
	return buf->data - buf->__buf;
}

size_t bt_buf_simple_tailroom(const struct bt_buf_simple *buf)
{
	return buf->size - bt_buf_simple_headroom(buf) - buf->len;
}

uint16_t bt_buf_simple_max_len(const struct bt_buf_simple *buf)
{
	return buf->size - bt_buf_simple_headroom(buf);
}