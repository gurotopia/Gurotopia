#pragma once

extern void queue_add_drop(const std::string &world, u_short id, u_short count, ::pos pos);
extern void queue_remove_drop(const std::string &world, int uid, const std::string &by_growid);
extern void dropqueue_cancel_adds(const std::string &world);
extern void dropqueue_pump();
