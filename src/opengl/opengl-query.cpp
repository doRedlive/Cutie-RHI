// do@Redlive

#include "opengl-backend.h"

namespace cutie::opengl
{

EventQuery::EventQuery(const GLContext& context) : m_Context(context) {}
EventQuery::~EventQuery()
{
    if (fence) glDeleteSync(fence);
}

TimerQuery::TimerQuery(const GLContext& context) : m_Context(context) {}
TimerQuery::~TimerQuery()
{
    if (queryBegin) glDeleteQueries(1, &queryBegin);
    if (queryEnd) glDeleteQueries(1, &queryEnd);
}

} // namespace cutie::opengl
