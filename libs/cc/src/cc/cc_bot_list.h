#pragma once

#include "cc/cc_bot_data.h"
#include "cc/cc_bot.h"
#include "cc/bot_id.h"

#include "co/async/session.h"

namespace cc {

struct CcBotRecordIterator;

class ICcBotList {
public:
  virtual ~ICcBotList() = default;
  virtual CcBotRecordIterator begin() = 0;
  virtual CcBotRecordIterator end() = 0;
  virtual size_t GetCount() const = 0;
};

class CcServer;

// `reference` is just a pointer to iterator's internal value
// Changing iterator's value (Shptr<ICcBot>) through reference operator*()
// will make no changes except in iterator itself.
struct CcBotRecordIterator
{
  //CcBotRecordIterator(const CcBotRecordIterator& r)
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = Shptr<ICcBot>;
  using pointer = Shptr<ICcBot>*;
  using reference = Shptr<ICcBot>&;

  using SessionListContainer = co::async::SessionListContainer;

  reference operator*() const;
  pointer operator->();
  CcBotRecordIterator& operator++();
  CcBotRecordIterator operator++(int);
  bool operator== (const CcBotRecordIterator& a);
  bool operator!= (const CcBotRecordIterator& a);

private:
  friend class CcServer;
  CcBotRecordIterator(SessionListContainer::Iterator list_it);

  void UpdateCurrentRecord() const;

private:
  SessionListContainer::Iterator list_it_;
  mutable Shptr<ICcBot> cur_record_;
}; 


}



