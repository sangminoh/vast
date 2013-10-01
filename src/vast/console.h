#ifndef VAST_CONSOLE_H
#define VAST_CONSOLE_H

#include <deque>
#include "vast/actor.h"
#include "vast/cow.h"
#include "vast/util/editline.h"

namespace vast {

class event;

/// A console-based, interactive query client.
class console : public actor<console>
{
public:
  /// Spawns the console client.
  console() = default;

  void act();
  char const* description() const;

private:
  std::deque<cow<event>> results_;
  cppa::actor_ptr query_;
  util::editline editline_;
};

} // namespace vast

#endif
