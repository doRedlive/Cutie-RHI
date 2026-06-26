// do@Redlive

#include "opengl-backend.h"

namespace cutie::opengl
{

Heap::Heap(const GLContext& context) : m_Context(context) {}
Heap::~Heap() {}

CommandListLifetimeTracker::CommandListLifetimeTracker(const GLContext& context) : m_Context(context) {}
CommandListLifetimeTracker::~CommandListLifetimeTracker() {}
void CommandListLifetimeTracker::runGarbageCollection() { trackedCommandLists.clear(); }

} // namespace cutie::opengl
