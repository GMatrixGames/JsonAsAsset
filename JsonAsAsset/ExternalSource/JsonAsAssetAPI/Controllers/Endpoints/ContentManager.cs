using System.Text.Json.Serialization;
using Newtonsoft.Json;
using System.Net;
using RestSharp;

using EpicManifestParser.Objects;
using System;
using System.IO;

namespace JsonAsAssetAPI.Endpoints.ContentManager;

public class AuthorizationManager
{
    public string AccessToken;
    public readonly RestClient Client;

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
    public class BuildItem
    {
        [JsonPropertyAttribute] public string Distribution;
        [JsonPropertyAttribute] public string Path;
    }

    public class BuildItems
    {
        [JsonPropertyAttribute] public BuildItem Manifest;
    }

    [JsonPropertyAttribute] public BuildItems Items;
}

// [FORTNITE] Content Manager
public class ContentManager
{
    public readonly RestClient Client;
    public AuthorizationManager AuthManager;

    public async Task<ContentBuildResponse?> GetContentBuilds(string label)
    {
        await AuthManager.VerifyAuthorization();

        RestRequest Request = new RestRequest(
            "https://account-public-service-prod03.ol.epicgames.com/account/api/oauth/token",
            Method.Post // Post creates ouath token
        ).AddHeader(
            "Authorization",
            // Add client token
            $"bearer {AuthManager.AccessToken}"
        ).AddQueryParameter("label", label);

        RestResponse<ContentBuildResponse> Response = await Client.ExecuteAsync<ContentBuildResponse>(Request);
        return Response.Data;
    }

    public async Task<Manifest> GetManifest(string url)
    {
        RestRequest Request = new RestRequest(url);
        RestResponse Response = await Client.ExecuteAsync(Request);

        return new Manifest(Response.RawBytes, new ManifestOptions
        {
            ChunkBaseUri = new Uri("https://epicgames-download1.akamaized.net/Builds/Fortnite/Content/CloudDir/ChunksV4/", UriKind.Absolute),
            ChunkCacheDirectory = new DirectoryInfo(Globals.ExportDirectory)
        });
    }
}