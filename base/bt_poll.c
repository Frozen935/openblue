/******************************************************************************
 *
 * Copyright (C) 2024 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/
#include <stdint.h>

#include <base/bt_poll.h>

#include <osdep/os.h>

static os_mutex_t lock = OS_MUTEX_INITIALIZER;

static inline void add_event(bt_dlist_t *events, struct bt_poll_event *event,
			     struct bt_poller *poller)
{
	bt_dlist_append(events, &event->_node);
}

/* must be called with interrupts locked */
static inline bool is_condition_met(struct bt_poll_event *event, uint32_t *state)
{
	switch (event->type) {
	case BT_POLL_TYPE_DATA_AVAILABLE:
		if (!bt_queue_is_empty(event->queue)) {
			*state = BT_POLL_STATE_FIFO_DATA_AVAILABLE;
			return true;
		}
		break;
	case BT_POLL_TYPE_SIGNAL:
		if (event->signal->signaled != 0U) {
			*state = BT_POLL_STATE_SIGNALED;
			return true;
		}
		break;
	case BT_POLL_TYPE_IGNORE:
		break;
	default:
		__ASSERT_MSG(false, "invalid event type (0x%x)\n", event->type);
		break;
	}

	return false;
}

/* must be called with interrupts locked */
static inline void register_event(struct bt_poll_event *event, struct bt_poller *poller)
{
	switch (event->type) {
	case BT_POLL_TYPE_DATA_AVAILABLE:
		__ASSERT_MSG(event->queue != NULL, "invalid queue\n");
		add_event(&event->queue->poll_events, event, poller);
		break;
	case BT_POLL_TYPE_SIGNAL:
		__ASSERT_MSG(event->signal != NULL, "invalid poll signal\n");
		add_event(&event->signal->poll_events, event, poller);
		break;
	case BT_POLL_TYPE_IGNORE:
		/* nothing to do */
		break;
	default:
		__ASSERT_MSG(false, "invalid event type\n");
		break;
	}

	event->poller = poller;
}

/* must be called with interrupts locked */
static inline void clear_event_registration(struct bt_poll_event *event)
{
	bool remove_event = false;

	event->poller = NULL;

	switch (event->type) {
	case BT_POLL_TYPE_DATA_AVAILABLE:
		__ASSERT_MSG(event->queue != NULL, "invalid queue\n");
		remove_event = true;
		break;
	case BT_POLL_TYPE_SIGNAL:
		__ASSERT_MSG(event->signal != NULL, "invalid poll signal\n");
		remove_event = true;
		break;
	case BT_POLL_TYPE_IGNORE:
		/* nothing to do */
		break;
	default:
		__ASSERT_MSG(false, "invalid event type\n");
		break;
	}
	if (remove_event && bt_dnode_is_linked(&event->_node)) {
		bt_dlist_remove(&event->_node);
	}
}

/* must be called with interrupts locked */
static inline void clear_event_registrations(struct bt_poll_event *events, int num_events)
{
	while (num_events--) {
		clear_event_registration(&events[num_events]);
	}
}

static bool event_match(struct bt_poll_event *event, uint32_t state)
{
	if (state == BT_POLL_STATE_SIGNALED && event->type == BT_POLL_TYPE_SIGNAL) {
		return true;
	} else if (state == BT_POLL_STATE_DATA_AVAILABLE &&
		   event->type == BT_POLL_TYPE_DATA_AVAILABLE) {
		return true;
	} else if (state == BT_POLL_STATE_MSGQ_DATA_AVAILABLE &&
		   event->type == BT_POLL_TYPE_MSGQ_DATA_AVAILABLE) {
		return true;
	} else if (state == BT_POLL_STATE_SEM_AVAILABLE &&
		   event->type == BT_POLL_TYPE_SEM_AVAILABLE) {
		return true;
	} else {
		return false;
	}
}

static inline void set_event_ready(struct bt_poll_event *event, uint32_t state)
{
	event->poller = NULL;
	event->state |= state;

	switch (event->type) {
	case BT_POLL_TYPE_DATA_AVAILABLE:
		break;
	case BT_POLL_TYPE_SIGNAL:
		if (event->signal->signaled != 0) {
			event->signal->signaled = 0;
		}
		break;
	}
}

static inline int register_events(struct bt_poll_event *events, int num_events,
				  struct bt_poller *poller, bool just_check)
{
	int events_registered = 0;

	for (int ii = 0; ii < num_events; ii++) {
		uint32_t state;

		os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);
		if (is_condition_met(&events[ii], &state)) {
			set_event_ready(&events[ii], state);
			poller->is_polling = false;
		} else if (!just_check && poller->is_polling) {
			register_event(&events[ii], poller);
			events_registered += 1;
		} else {
			/* Event is not one of those identified in is_condition_met()
			 * catching non-polling events, or is marked for just check,
			 * or not marked for polling. No action needed.
			 */
			;
		}
		os_mutex_unlock(&lock);
	}

	return events_registered;
}

static int signal_poller(struct bt_poll_event *event, uint32_t state)
{
	struct bt_poller *poller = event->poller;

	if (state == BT_POLL_STATE_CANCELLED || event_match(event, state)) {
		set_event_ready(event, state);
		os_sem_give(&poller->sem);
	}

	return 0;
}

void bt_poll_event_init(struct bt_poll_event *event, uint32_t type, int mode, void *obj)
{
	event->type = type;
	event->state = BT_POLL_STATE_NOT_READY;
	event->obj = obj;
}

int bt_poll(struct bt_poll_event *events, int num_events, os_timeout_t timeout)
{
	int events_registered;
	struct bt_poller poller;

	os_sem_init(&poller.sem, 0, 1);
	poller.is_polling = true;

	__ASSERT_MSG(events != NULL, "NULL events\n");
	__ASSERT_MSG(num_events >= 0, "<0 events\n");

	events_registered = register_events(events, num_events, &poller,
					    TIMEOUT_EQ(timeout, OS_TIMEOUT_NO_WAIT));

	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);

	/*
	 * If we're not polling anymore, it means that at least one event
	 * condition is met, either when looping through the events here or
	 * because one of the events registered has had its state changed.
	 */
	if (!poller.is_polling) {
		clear_event_registrations(events, events_registered);
		os_mutex_unlock(&lock);

		return 0;
	}

	poller.is_polling = false;

	if (TIMEOUT_EQ(timeout, OS_TIMEOUT_NO_WAIT)) {
		os_mutex_unlock(&lock);

		return 0;
	}

	int ret = os_sem_take(&poller.sem, timeout);
	os_mutex_unlock(&lock);

	/*
	 * Clear all event registrations. If events happen while we're in this
	 * loop, and we already had one that triggered, that's OK: they will
	 * end up in the list of events that are ready; if we timed out, and
	 * events happen while we're in this loop, that is OK as well since
	 * we've already know the return code (-EAGAIN), and even if they are
	 * added to the list of events that occurred, the user has to check the
	 * return code first, which invalidates the whole list of event states.
	 */

	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);
	clear_event_registrations(events, events_registered);
	os_mutex_unlock(&lock);

	return ret;
}

/* must be called with interrupts locked */
static int signal_poll_event(struct bt_poll_event *event, uint32_t state)
{
	struct bt_poller *poller = event->poller;
	int retcode = 0;

	if (poller != NULL) {
		retcode = signal_poller(event, state);
		poller->is_polling = false;

		if (retcode < 0) {
			return retcode;
		}
	}

	return retcode;
}

void bt_poll_handle_obj_events(bt_dlist_t *events, uint32_t state)
{
	struct bt_poll_event *poll_event;
	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);

	poll_event = (struct bt_poll_event *)bt_dlist_get(events);
	if (poll_event != NULL) {
		(void)signal_poll_event(poll_event, state);
	}

	os_mutex_unlock(&lock);
}

int bt_poll_signal_raise(struct bt_poll_signal *sig, int result)
{
	os_mutex_lock(&lock, OS_TIMEOUT_FOREVER);
	struct bt_poll_event *poll_event;

	sig->result = result;
	sig->signaled = 1U;

	poll_event = (struct bt_poll_event *)bt_dlist_get(&sig->poll_events);
	if (poll_event == NULL) {
		os_mutex_unlock(&lock);

		return 0;
	}

	int rc = signal_poll_event(poll_event, BT_POLL_STATE_SIGNALED);

	os_mutex_unlock(&lock);
	return rc;
}