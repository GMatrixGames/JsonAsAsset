using RestSharp.Serializers.NewtonsoftJson;
using System.Text.Json.Serialization;
using EpicManifestParser.Objects;
using System.Net;
using RestSharp;

using J = Newtonsoft.Json.JsonPropertyAttribute;
namespace JsonAsAssetAPI.Endpoints.ContentManager;

public class AuthorizationManager
{
    public string AccessToken;
    public readonly RestClient Client = new RestClient();

    public async Task<bool> VerifyAuthorization()
    {
        bool bIsExpired = IsExpired();
        if (bIsExpired)
        {
            AuthorizationResponse Authorization = await GetAuthorization();
            AccessToken = Authorization.AccessToken;
        }

        return bIsExpired;
    }

    // Creates Authorization Request
    async Task<AuthorizationResponse?> GetAuthorization()
    {
        RestRequest Request = new RestRequest(
            "https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token",
            Method.Post // Post creates ouath token
        ).AddHeader(
            "Authorization",
            // Add client token
            "basic MzQ0NmNkNzI2OTRjNGE0NDg1ZDgxYjc3YWRiYjIxNDE6OTIwOWQ0YTVlMjVhNDU3ZmI5YjA3NDg5ZDMxM2I0MWE="
        ).AddParameter(
            "grant_type",
            "client_credentials"
        );

        // Execute Request (ASYNC)
        RestResponse<AuthorizationResponse> Response = await Client.ExecuteAsync<AuthorizationResponse>(Request);
        return Response.Data;
    }

    public bool IsExpired()
    {
        RestRequest Request = new RestRequest(
            "https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token",
            Method.Post // Post creates ouath token
        ).AddHeader(
            "Authorization",
            // Add client token
            $"bearer {AccessToken}"
        );

        RestResponse Response = Client.Execute(Request);
        return Response.StatusCode != HttpStatusCode.OK;
    }
}

// Response when authorized
public class AuthorizationResponse
{
    [JsonPropertyName("access_token")]
        public string AccessToken { get; init; }

    [JsonPropertyName("expires_at")] 
        public DateTime ExpiresAt { get; init; }
}

public class ContentBuildResponse
{
    [J] public ContentItems Items;

    public class ContentItems
    {
        [J] public ContentItem Manifest;
    }

    public class ContentItem
    {
        [J] public string Distribution;
        [J] public string Path;
    }
}

public class SimpleJsonSerializer : JsonNetSerializer
{
    public string[] SupportedContentTypes { get; } = {
        "application/json", "text/json", "text/x-json", "text/javascript", "*+json", "text/plain"
    };

}

public class ManifestManager 
{
    public readonly RestClient Client = new RestClient();

    public async Task<Manifest> GetManifest(string url)
    {
        RestResponse Response = await Client.ExecuteAsync(new RestRequest(url));

        return new Manifest(Response.RawBytes, new ManifestOptions
        {
            ChunkBaseUri = new Uri("https://epicgames-download1.akamaized.net/Builds/Fortnite/Content/CloudDir/ChunksV4/", UriKind.Absolute),
            ChunkCacheDirectory = new DirectoryInfo(Globals.ExportDirectory + "/Chunks")
        });
    }
}

// [FORTNITE] Content Manager
public class ContentManager
{
    public readonly RestClient Client = new RestClient(
        configureSerialization: s => s.UseSerializer(() => new SimpleJsonSerializer())
    );

    public AuthorizationManager AuthManager = new AuthorizationManager();
    public ManifestManager LocalManifestManager = new ManifestManager();

    public async Task<ContentBuildResponse?> GetContentBuilds(string label)
    {
        await AuthManager.VerifyAuthorization();

        RestRequest Request = new RestRequest(
            "https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/public/assets/Windows/5cb97847cee34581afdbc445400e2f77/FortniteContentBuilds"
        ).AddHeader(
            "Authorization",
            // Add client token
            $"bearer {AuthManager.AccessToken}"
        ).AddQueryParameter("label", label);

        RestResponse<ContentBuildResponse> Response = await Client.ExecuteAsync<ContentBuildResponse>(Request);
        return Response.Data;
    }
}