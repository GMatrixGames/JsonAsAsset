// Copyright JAA Contributors 2024-2025

#pragma once
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

class FRemoteUtilities {
public:
	static TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> ExecuteRequestSync(TSharedRef<IHttpRequest> HttpRequest, float LoopDelay = 0.1);
};
