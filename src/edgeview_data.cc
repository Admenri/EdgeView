#include "edgeview_data.h"

#include "ev_network.h"

namespace edgeview {

ResourceRequestCallback::~ResourceRequestCallback() { ContinueRequest(this, nullptr); }

}  // namespace edgeview
