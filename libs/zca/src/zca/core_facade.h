#pragma once

#include "zca/buildable.h"
#include "co/async/thread_model.h"

template <class BuildTypes, class TConfig, class TSepConfig>
class Builder {
public:
  virtual ~Builder() = default;

  using TSepObjects = typename BuildTypes::SeparatableObjects;
  using TObjects = typename BuildTypes::Objects ;
  using TParams = typename BuildTypes::Params;
  using ThreadModel = co::async::ThreadModel;

  virtual Uptr<TSepObjects> BuildSeparatableObjects(ThreadModel&, const TConfig&, const TSepConfig&) = 0;
  virtual Uptr<TObjects> BuildObjects(ThreadModel&, const TConfig&) = 0;
  virtual Uptr<TParams> BuildParams(const TConfig&) = 0;
};

template <class BuildTypes, class TConfig, class TSepConfig>
class BuildDirector {
public:
  using SharedData = typename BuildTypes::SharedData;
  using ThreadModel = co::async::ThreadModel;

  BuildDirector(
    Buildable<BuildTypes>& target,
    Builder<BuildTypes, TConfig, TSepConfig>& builder,
    ThreadModel& thread_model,
    TConfig conf,
    TSepConfig sep_conf,
    SharedData shar_data)
    :
    target_(target), builder_(builder), thread_model_(thread_model),
    conf_(conf), sep_conf_(sep_conf), shar_data_(shar_data)
  {
  }
  void Build() {
    // All objects may need it
    target_.SetSharedData(shar_data_);

    target_.SetSeparatableObjects(builder_.BuildSeparatableObjects(thread_model_, conf_, sep_conf_));
    target_.SetObjects(builder_.BuildObjects(thread_model_, conf_));
    target_.SetParams(builder_.BuildParams(conf_));

    target_.CompleteBuild();
  }
private:
  Buildable<BuildTypes>& target_;
  Builder<BuildTypes, TConfig, TSepConfig>& builder_;
  ThreadModel& thread_model_;
  TConfig conf_;
  TSepConfig sep_conf_;
  SharedData shar_data_;
};

template <class Core, class BuildTypes, class Builder, class TConfig, class TSepConfig>
class CoreFacade : public Core {
public:
  using SharedData = typename BuildTypes::SharedData;
  using ThreadModel = co::async::ThreadModel;

  virtual ~CoreFacade() = default;

  CoreFacade(ThreadModel& thread_model,
    TConfig conf,
    TSepConfig sep_conf,
    SharedData shar_data = SharedData())
    :
    build_director_(*this, builder_, thread_model, conf, sep_conf, shar_data)
  {
    build_director_.Build();
  }
private:
  Builder builder_;
  BuildDirector<BuildTypes, TConfig, TSepConfig> build_director_;
};




