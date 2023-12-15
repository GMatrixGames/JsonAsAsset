using Microsoft.EntityFrameworkCore;
using Microsoft.AspNetCore.Mvc;
using CUE4Parse.Utils;
using CUE4Parse.UE4.Assets.Exports;
using CUE4Parse.UE4.Assets.Exports.Texture;
using CUE4Parse.UE4.Assets.Exports.Sound;
using CUE4Parse.UE4.Objects.Core.Misc;
using CUE4Parse.UE4.Versions;
using CUE4Parse_Conversion.Textures;
using CUE4Parse_Conversion.Sounds;
using CUE4Parse.Encryption.Aes;
using CUE4Parse.MappingsProvider;
using UE4Config.Parsing;
using Newtonsoft.Json;
using SkiaSharp;
using CUE4Parse.FileProvider;
using System.Runtime.InteropServices;
using System.IO;

// Global Provider
public class Globals
{
    public static DefaultFileProvider? Provider;

    public static string? MappingFilePath;
    public static string? ArchiveDirectory;
    public static string? ExportDirectory;
    public static EGame UnrealVersion;
    public static string? ArchiveKey;
    public static bool bHideConsole;

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

        if (values.Count == 0)
            return "";

        return values[0].SubstringBeforeLast("\"").SubstringAfterLast("\"");
    }

    // Get config property (string) by name
    public string GetStringProperty(ConfigIni config, string PropertyName)
    {
        var values = new List<string>();
        config.EvaluatePropertyValues("/Script/JsonAsAsset.JsonAsAssetSettings", PropertyName, values);

        return values.Count == 1 ? values[0] : "";
    }

    // Get config property (array) by name
    public List<string> GetArrayProperty(ConfigIni config, string PropertyName)
    {
        var values = new List<string>();
        config.EvaluatePropertyValues("/Script/JsonAsAsset.JsonAsAssetSettings", PropertyName, values);

        return values;
    }

    // Get config property (bool) by name
    public bool GetBoolProperty(ConfigIni config, string PropertyName, bool Default = false)
    {
        var values = new List<string>();
        config.EvaluatePropertyValues("/Script/JsonAsAsset.JsonAsAssetSettings", PropertyName, values);

        if (values.Count == 0)
            return Default;

        return values[0] == "True" ? true : false;
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
        bHideConsole = GetBoolProperty(config, "bHideConsole");

        [DllImport("kernel32.dll")]
        static extern IntPtr GetConsoleWindow();
        [DllImport("user32.dll")]
        static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

        ShowWindow(GetConsoleWindow(), bHideConsole ? 0 : 5);

        if (MappingFilePath != "") WriteLog("UserSettings", ConsoleColor.Blue, $"Mappings: {MappingFilePath.SubstringBeforeLast("\\")}");
        WriteLog("UserSettings", ConsoleColor.Blue, $"Archive Directory: {ArchiveDirectory.SubstringBeforeLast("\\")}");
        WriteLog("UserSettings", ConsoleColor.Blue, $"Unreal Versioning: {UnrealVersion.ToString()}");

        return config;
    }

    public async Task Initialize()
    {
        WriteLog("CORE", ConsoleColor.Green, "Initializing globals, and provider..");

        // Find config folder
        string config_folder = System.AppDomain.CurrentDomain.BaseDirectory.SubstringBeforeLast("\\Plugins\\") + "\\Config\\";
        WriteLog("Provider", ConsoleColor.Red, $"Found config folder: {config_folder.SubstringBeforeLast("\\")}");

        // DefaultEditorPerProjectUserSettings
        ConfigIni config = GetEditorConfig();

        // Create new file provider
        Provider = new DefaultFileProvider(ArchiveDirectory, SearchOption.TopDirectoryOnly, true, new VersionContainer(UnrealVersion));
        Provider.Initialize();

        if (ArchiveKey != "")
        {
            // Submit Main AES Key
            await Provider.SubmitKeyAsync(new FGuid(), new FAesKey(ArchiveKey));
            WriteLog("Provider", ConsoleColor.Red, $"Submitted Archive Key: {ArchiveKey}");
        }

        var DynamicKeys = GetArrayProperty(config, "DynamicKeys");
        if (DynamicKeys.Count() != 0)
            WriteLog("Provider", ConsoleColor.Red, "Reading " + DynamicKeys.Count() + " Dynamic Keys -------------------------------------------");

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

        if (MappingFilePath != "") Provider.MappingsContainer = new FileUsmapTypeMappingsProvider(MappingFilePath);
        Provider.LoadLocalization(ELanguage.English);
        Provider.LoadVirtualPaths();
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
            var type = Request.Headers.ContentType;

            // Try to load object, if failed, return message
            try
            {
                path = path.SubstringBefore('.');
                var LocalObject = Provider.LoadObject(path);

                if (!raw)
                {
                    switch (LocalObject)
                    {
                        case UTexture texture:
                            UTexture TextureObject = (UTexture)Provider.LoadObject(path);

                            // .bin support
                            if (type == "application/octet-stream" && TextureObject.GetFirstMip().BulkData.Data is { } mipData)
                                return File(mipData, type);

                            // Texture data
                            SKBitmap TextureData = TextureObject.Decode();
                            if (TextureData == null)
                                return StatusCode(500, value: new
                                {
                                    errored = true,
                                    exceptionstring = "Invalid texture data, exported as json",
                                    jsonOutput = new { TextureObject }
                                });

                            return File(TextureData.Encode(SKEncodedImageFormat.Png, quality: 100).AsStream(), type);

                        case USoundWave wave:
                            wave.Decode(true, out var audioFormat, out var data);
                            if (data == null || string.IsNullOrEmpty(audioFormat))
                                return new ConflictObjectResult(new 
                                {
                                    errored = true,
                                    exceptionstring = "Invalid audio data, exported as json",
                                    jsonOutput = new[] { wave }
                                });

                            var mimeType = audioFormat.ToLower() switch
                            {
                                "wem" => "application/vnd.wwise.wem",
                                "wav" => "audio/wav",
                                "adpcm" => "audio/adpcm",
                                "opus" => "audio/opus",
                                _ => "audio/ogg"
                            };

                            return File(data, mimeType);
                    }
                }

                // Credit to MoutainFlash:
                //  - https://gist.github.com/MinshuG/55f0da93fb839d41050e634b288e81b1
                //    : (merging editor only data)
                var objectPath = path.SubstringBefore('.') + ".o.uasset";
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
                return new OkObjectResult(JsonConvert.SerializeObject(new {
                    jsonOutput = finalExports
                }, Formatting.Indented));
            }
            catch (Exception exception)
            {
                return new ConflictObjectResult(
                    new
                    {
                        errored = true,
                        note =
                            exception.Message.StartsWith("One or more errors occurred. (There is no game file with the path ") ?
                                "Unable to find package" :
                            exception.Message
                    }
                );
            }
        }
    }
}
