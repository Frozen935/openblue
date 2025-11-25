# SPDX-License-Identifier: Apache-2.0

# Base utilities (work queue, buffers, debug, etc.)
openblue_library()

openblue_library_sources(
  bt_atomic.c
  bt_buf.c
  bt_buf_simple.c
  utils.c
  queue/bt_queue.c
  bt_mem_pool.c
  bt_poll.c
  bt_work.c
  bt_debug.c
  log.c
)

# Headers under base/ are included via root include dir (${CMAKE_SOURCE_DIR})
# No extra link requirements here; aggregation happens at top-level.
