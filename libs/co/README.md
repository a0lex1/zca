Important moments

#SharedFromThisRefTrackerIssue
14 Feb 2024.
The order of DTORing object and args of bind() is differ for std:: and boost:: binds.
In std::bind, the object is DTORed before the args, so we get UAF.
This is why we invented RefTracker::SetReferencedObject(shared_from_this()).
We need to add it for every object's BeginIo() if that object uses enable_shared_from_this.

#StopOnError
void HandleSomething(Errcode err, RefTracker rt) {
  if (err) {
    return; // we can just return, because we used SetReferencedObject
    // but if we are multifibiered (another i/o fiber is running), we need to StopThreadsafe()
  }
}

#UafRecreateThreadModel
Restarter's recreate_thread_model should be true because, for example, deadline_timer's async_wait
always fire (even if cancel()ed). We can DisableOnReleaseCalls(), but we can't do the same
thing about, for example, deadline_timer.

#NewFiberShouldCheckForStopping
TODO


