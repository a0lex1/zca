#pragma once

#include "co/common.h"

template <class BuildTypes, class TConfig, class TSepConfig>
class BuildDirector;

template <class BuildTypes>
class Buildable {
public:
  virtual ~Buildable() = default;
private:
  template <class B, class C, class S> 
  friend class BuildDirector;

  typedef typename BuildTypes::SeparatableObjects TSepObjects;
  typedef typename BuildTypes::Objects TObjects;
  typedef typename BuildTypes::Params TParams;
  typedef typename BuildTypes::SharedData TSharedData;

  void SetSeparatableObjects(Uptr<TSepObjects> sep_objs) { sep_objs_ = std::move(sep_objs); }
  void SetObjects(Uptr<TObjects> objs) { objs_ = std::move(objs); }
  void SetParams(Uptr<TParams> params) { params_ = std::move(params); }
  void SetSharedData(TSharedData shar_data) { shar_data_ = shar_data; }

  // Complete build process after all objects are set
  virtual void CompleteBuild() = 0;

protected:
  TSepObjects& SeparatableObjects() { return *sep_objs_.get(); }
  TObjects& Objects() { return *objs_.get(); }
  TParams& Params() { return *params_.get(); }
  TSharedData& SharedData() { return shar_data_; }

private:
  Uptr<TSepObjects> sep_objs_;
  Uptr<TObjects> objs_;
  Uptr<TParams> params_;
  TSharedData shar_data_;
};



