#pragma once

#include "co/common.h"

namespace co {

template <class T>
class FilterChainLink : public T
{
public:
  virtual ~FilterChainLink() = default;

  using UnderlyingType = /*typename*/ T; // linux doesn't like typename

  void SetNext(FilterChainLink<T>* t)
  {
    next_ = t;
  }
  inline FilterChainLink<T>* GetNext()
  {
    return next_;
  }
private:
  FilterChainLink<T>* next_{ nullptr };
};

template <class LinkType>
class FilterChainHead : public LinkType
{
public:
  virtual ~FilterChainHead() = default;

  FilterChainHead()
  {
    LinkType::SetNext(&stub_link_);
  }
  void AttachTop(LinkType* link)
  {
    DCHECK(!frozen_);
    link->SetNext(LinkType::GetNext());
    LinkType::SetNext(link);
  }
  void Freeze() {
    DCHECK(!frozen_);
    frozen_ = true;
  }

private:
  LinkType stub_link_;
  bool frozen_{ false };
};


} // namespace co


