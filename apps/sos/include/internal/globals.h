#pragma once

#include "internal/Capability.h"

const Capability<seL4_EndpointObject, seL4_EndpointBits>& getIpcEndpoint() noexcept;
