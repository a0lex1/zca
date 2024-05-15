#pragma once

#include "netshell/status_descriptor_table.h"

static const int kNsCmdNoResult = 0;
static const int kNsCmdNotFound = 150;
static const int kNsCmdExecuted = 200;
static const int kNsBackNotConnected = 300;

// Netshell customization for zca project.
// See .cpp file.
extern netshell::NsStatusDescriptorTable gZcaNsStatusDescriptorTable;
