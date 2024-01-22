// Copyright JAA Contributors 2023-2024

#include "Utilities/RemoteUtilities.h"

#include "HttpManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"

TSharedPtr<IHttpResponse> FRemoteUtilities::ExecuteRequestSync(TSharedRef<IHttpRequest> HttpRequest, float LoopDelay)
{
	const bool bStartedRequest = HttpRequest->ProcessRequest();
	if (!bStartedRequest)
	{
		UE_LOG(LogJson, Error, TEXT("Failed to start HTTP Request."));
		return nullptr;
	}

	double LastTime = FPlatformTime::Seconds();
	while (EHttpRequestStatus::Processing == HttpRequest->GetStatus())
	{
		const double AppTime = FPlatformTime::Seconds();
		FHttpModule::Get().GetHttpManager().Tick(AppTime - LastTime);
		LastTime = AppTime;
		FPlatformProcess::Sleep(LoopDelay);
	}

	return HttpRequest->GetResponse();
}
