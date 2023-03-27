// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

class FRemoteUtilities {
public:
	static TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> ExecuteRequestSync(TSharedRef<IHttpRequest> HttpRequest, float LoopDelay = 0.1);
};
