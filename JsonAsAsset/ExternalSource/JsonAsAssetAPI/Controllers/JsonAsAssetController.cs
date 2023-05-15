using Microsoft.EntityFrameworkCore;
using Microsoft.AspNetCore.Mvc;
using CUE4Parse.FileProvider;
using CUE4Parse.UE4.Assets.Exports;
using CUE4Parse.Utils;
using CUE4Parse.UE4.Assets.Exports.Texture;
using CUE4Parse_Conversion.Textures;
using CUE4Parse.UE4.Versions;
using CUE4Parse.Encryption.Aes;
using CUE4Parse.UE4.Objects.Core.Misc;
using CUE4Parse.MappingsProvider;
using UE4Config.Parsing;
using Newtonsoft.Json;
using SkiaSharp;

using JsonAsAssetAPI.Endpoints.ContentManager;
using CUE4Parse.FileProvider.Objects;
using CUE4Parse.FileProvider.Vfs;
using RestSharp.Serializers.NewtonsoftJson;
using EpicManifestParser.Objects;

// Global Provider
public class Globals
{
    public static DefaultFileProvider? Provider;

    public static string? MappingFilePath;
    public static string? ArchiveDirectory;
    public static string? ExportDirectory;
    public static EGame UnrealVersion;
    public static string? ArchiveKey;
    public ContentManager Manager = new ContentManager();

    public void WriteLog(string source, ConsoleColor Color, string description)
    {
        Console.ForegroundColor = Color;
        Console.Write('[' + source + "] ");
        Console.ResetColor();
        Console.Write(description + '\n');
    }

    // Get config property (path) by name
    public string GetPathProperty(ConfigIni config, string PropertyName)
    {
        var values = new List<string>();
        config.EvaluatePropertyValues("/Script/JsonAsAsset.JsonAsAssetSettings", PropertyName, values);

        return values[0].SubstringBeforeLast("\"").SubstringAfterLast("\"");
    }

    // Get config property (string) by name
    public string GetStringProperty(ConfigIni config, string PropertyName)
    {
        var values = new List<string>();
        config.EvaluatePropertyValues("/Script/JsonAsAsset.JsonAsAssetSettings", PropertyName, values);

        return values[0];
    }

    // Get config property (array) by name
    public List<string> GetArrayProperty(ConfigIni config, string PropertyName)
    {
        var values = new List<string>();
        config.EvaluatePropertyValues("/Script/JsonAsAsset.JsonAsAssetSettings", PropertyName, values);

        return values;
    }

    // Find config folder & UpdateData
    public ConfigIni GetEditorConfig()
    {
        string config_folder = System.AppDomain.CurrentDomain.BaseDirectory.SubstringBeforeLast("\\Plugins\\") + "\\Config\\";
        ConfigIni config = new ConfigIni("DefaultEditorPerProjectUserSettings");
        config.Read(File.OpenText(config_folder + "DefaultEditorPerProjectUserSettings.ini"));

        // Set Config Data to class
        MappingFilePath = GetPathProperty(config, "MappingFilePath");
        ExportDirectory = GetPathProperty(config, "ExportDirectory");
        ArchiveDirectory = GetPathProperty(config, "ArchiveDirectory");
        UnrealVersion = (EGame)Enum.Parse(typeof(EGame), GetStringProperty(config, "UnrealVersion"), true);
        ArchiveKey = GetStringProperty(config, "ArchiveKey");

        WriteLog("UserSettings", ConsoleColor.Blue, $"Mappings: {MappingFilePath.SubstringBeforeLast("\\")}");
        WriteLog("UserSettings", ConsoleColor.Blue, $"Archive Directory: {ArchiveDirectory.SubstringBeforeLast("\\")}");
        WriteLog("UserSettings", ConsoleColor.Blue, $"Unreal Versioning: {UnrealVersion.ToString()}");

        return config;
    }

    private async Task<bool> InitializeContentBuilds()
    {
        // Find BuildInfo.ini by archive directory
        string BuildInfo = Path.Combine(ArchiveDirectory, "../../../Cloud/BuildInfo.ini");
        ConfigIni BuildConfig = new ConfigIni();
        BuildConfig.Read(File.OpenText(BuildInfo));

        string Label; {
            var InitEntries = new List<string>();
            BuildConfig.EvaluatePropertyValues("Content", "Label", InitEntries);
            Label = InitEntries[0];
        }

        ContentBuildResponse ContentBuilds = await Manager.GetContentBuilds(label: Label);
        if (ContentBuilds == null) 
            return false;

        // Construct Manifest
        ContentBuildResponse.ContentItem _Manifest = ContentBuilds.Items.Manifest;
        Manifest LocalManifest = await Manager.LocalManifestManager.GetManifest(url: (_Manifest.Distribution + _Manifest.Path));

        var ContentFiles = new Dictionary<string, GameFile>();
        foreach (var FileManifest in LocalManifest.FileManifests)
        {
            if (Provider.Files.ContainsKey(FileManifest.Name)) continue;

            StreamedGameFile StreamedFile = new StreamedGameFile(FileManifest.Name, FileManifest.GetStream(), Provider.Versions);
            ContentFiles[StreamedFile.Path.ToLowerInvariant()] = StreamedFile;
        }

        FileProviderDictionary FileDirectory = (FileProviderDictionary)Provider.Files;
        FileDirectory.AddFiles(ContentFiles);

        return true;
    }

    public async Task Initialize()
    {
        // Find config folder
        string config_folder = System.AppDomain.CurrentDomain.BaseDirectory.SubstringBeforeLast("\\Plugins\\") + "\\Config\\";
        WriteLog("Provider", ConsoleColor.Red, $"Found config folder: {config_folder.SubstringBeforeLast("\\")}");

        // DefaultEditorPerProjectUserSettings
        ConfigIni config = GetEditorConfig();

        // Create new file provider
        Provider = new DefaultFileProvider(ArchiveDirectory, SearchOption.TopDirectoryOnly, true, new VersionContainer(UnrealVersion));
        Provider.Initialize();

        // Submit Main AES Key
        await Provider.SubmitKeyAsync(new FGuid(), new FAesKey(ArchiveKey));
        WriteLog("Provider", ConsoleColor.Red, $"Submitted Archive Key: {ArchiveKey}");

        var DynamicKeys = GetArrayProperty(config, "DynamicKeys");
        if (DynamicKeys.Count() != 0)
            WriteLog("Provider", ConsoleColor.Red, "Reading {" + DynamicKeys.Count() + "} Dynamic Keys -------------------------------------------");

        // Submit each dynamic key
        foreach (string key in DynamicKeys)
        {
            var _key = key; var ReAssignedKey = _key.SubstringAfterLast("(").SubstringBeforeLast(")");
            string[] entries = ReAssignedKey.Split(",");

            // Key & Guid
            var Key = entries[0].SubstringBeforeLast("\"").SubstringAfterLast("\"");
            var Guid = entries[1].SubstringBeforeLast("\"").SubstringAfterLast("\"");

            await Provider.SubmitKeyAsync(new FGuid(Guid), new FAesKey(Key));
            WriteLog("Provider", ConsoleColor.Red, $"Submitted Key: {Key}");
        }

        Provider.MappingsContainer = new FileUsmapTypeMappingsProvider(MappingFilePath);
        Provider.LoadLocalization(ELanguage.English);
        Provider.LoadVirtualPaths();

        // bool bInitializedContentBuilds = await InitializeContentBuilds();
    }
}

// Handles API Requests
namespace JsonAsAssetAPI.Controllers
{
    [Route("api/[controller]")]
    [ApiController]
    public class JsonAsAssetController : ControllerBase
    {
        private readonly DbContext _context;
        private DefaultFileProvider Provider;

        public JsonAsAssetController(DbContext context)
        {
            _context = context;
            #pragma warning disable CS8601 // Possible null reference assignment.
                Provider = Globals.Provider;
            #pragma warning restore CS8601 // Possible null reference assignment.
        }

        [HttpGet("/api/v1/export")]
        public ActionResult Get(bool raw, string path)
        {
            // Try to load object, if failed, return message
            try
            {
                var LocalObject = Provider.LoadObject(path);

                switch (LocalObject)
                {
                    case UTexture2D texture:
                        // ignore
                    break;
                   
                    default:
                        raw = true;
                    break;
                }

                switch (raw)
                {
                    // Raw exports
                    case true:
                        path = path.SubstringBeforeLast('.');

                        // Add .uasset to path
                        if (!path.EndsWith(".uasset") && !path.Contains("."))
                            path = path + ".uasset";

                        // Credit to MoutainFlash:
                        //  - https://gist.github.com/MinshuG/55f0da93fb839d41050e634b288e81b1
                        //    : (merging editor only data)
                        var objectPath = path.SubstringBeforeLast('.') + ".o." + path.SubstringAfterLast('.');
                        var exports = Provider.LoadAllObjects(path);
                        var finalExports = new List<UObject>();
                        finalExports.AddRange(exports);

                        var mergedExports = new List<UObject>();
                        if (Provider.TryLoadPackage(objectPath, out var editorAsset))
                        {
                            foreach (var export in exports)
                            {
                                var editorData = editorAsset.GetExportOrNull(export.Name + "EditorOnlyData");
                                if (editorData != null)
                                {
                                    export.Properties.AddRange(editorData.Properties);
                                    mergedExports.Add(export);
                                }
                            }

                            foreach (var editorExport in editorAsset.GetExports())
                            {
                                if (!mergedExports.Contains(editorExport))
                                {
                                    finalExports.Add(editorExport);
                                }
                            }
                        }
                        mergedExports.Clear();

                        // Serialize object, and return it indented
                        return new OkObjectResult(JsonConvert.SerializeObject(finalExports, Formatting.Indented));

                    case false:
                        UTexture2D TextureObject = (UTexture2D)Provider.LoadObject(path);
                        
                        // Texture data
                        SKBitmap TextureData = TextureObject.Decode();
                        if (TextureData == null)
                            return StatusCode(500, value: new
                            {
                                errored = true,
                                exceptionstring = "Invalid texture data, exported as json",
                                jsonoutput = new { TextureObject }
                            });

                        return File(TextureData.Encode(SKEncodedImageFormat.Png, quality: 100).AsStream(), "image/png");
                }
            }
            catch (Exception exception)
            {
                if (exception.Message.StartsWith("There is no game file"))
                    return new NotFoundObjectResult(
                            new
                            {
                                errored = true,
                                note = "Unable to find package"
                            }
                        );
                else return new ConflictObjectResult(
                    new
                    {
                        errored = true,
                        note = exception.Message
                    }
                );
            }
        }
    }
}
