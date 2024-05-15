#include "cc/cc_bot_list.h"

#include "cc/cc_server_session.h"

using namespace std;
using namespace co;
using namespace co::async;

namespace cc {

CcBotRecordIterator::CcBotRecordIterator(SessionListContainer::Iterator list_it)
  : list_it_(list_it)
{
  // |cur_record_| is UpdateCurrentRecord()ed on read access, not on iterator
  // change
}

void CcBotRecordIterator::UpdateCurrentRecord() const
{
  Shptr<Session> sess = *list_it_;
  Shptr<CcServerSession> cc_server_sess(static_pointer_cast<CcServerSession>(sess));
  Shptr<ICcBot> cc_record(static_pointer_cast<ICcBot>(cc_server_sess));
  cur_record_ = cc_record;
}

CcBotRecordIterator::reference CcBotRecordIterator::operator*() const
{
  UpdateCurrentRecord();
  return cur_record_;
}

CcBotRecordIterator::pointer CcBotRecordIterator::operator->()
{
  UpdateCurrentRecord();
  return &cur_record_;
}

CcBotRecordIterator& CcBotRecordIterator::operator++()
{
  list_it_++;
  return *this;
}

CcBotRecordIterator CcBotRecordIterator::operator++(int)
{
  CcBotRecordIterator tmp = *this;
  ++(*this);
  return tmp;
}

bool CcBotRecordIterator::operator!=(const CcBotRecordIterator& a)
{
  return a.list_it_ != this->list_it_;
}

bool CcBotRecordIterator::operator==(const CcBotRecordIterator& a)
{
  return !operator!=(a);
}

}


